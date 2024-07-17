/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#include "AppConfig.h"
#include "USART.h"
#include "matter_shell.h"
#include "rsi_rom_egpio.h"
#include "sl_si91x_usart.h"
#include "cmsis_os2.h"
#include <sl_cmsis_os2_common.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "assert.h"
#include "rsi_board.h"
#include "rsi_debug.h"
#include "uart.h"
#include <stddef.h>
#include <string.h>

#include "silabs_utils.h"

#define USART_BAUDRATE 115200 // Baud rate <9600-7372800>
#define UART_CONSOLE_ERR -1   // Negative value in case of UART Console action failed. Triggers a failure for PW_RPC
#define MAX_BUFFER_SIZE 256
#define MAX_DMA_BUFFER_SIZE (MAX_BUFFER_SIZE / 2)

// uart transmit
#if SILABS_LOG_OUT_UART
#define UART_MAX_QUEUE_SIZE 125
#else
#define UART_MAX_QUEUE_SIZE 25
#endif

#ifdef CHIP_CONFIG_LOG_MESSAGE_MAX_SIZE
#define UART_TX_MAX_BUF_LEN (CHIP_CONFIG_LOG_MESSAGE_MAX_SIZE + 2) // \r\n
#else
#define UART_TX_MAX_BUF_LEN (258)
#endif

typedef struct
{
    // The data buffer
    uint8_t * pBuffer;
    // The offset of the first item written to the list.
    volatile uint16_t Head;
    // The offset of the next item to be written to the list.
    volatile uint16_t Tail;
    // Maxium size of data that can be hold in buffer before overwriting
    uint16_t MaxSize;
} Fifo_t;

typedef struct
{
    uint8_t data[UART_TX_MAX_BUF_LEN];
    uint16_t length = 0;
} UartTxStruct_t;

static uint8_t sRxFifoBuffer[MAX_BUFFER_SIZE];
static Fifo_t sReceiveFifo;

sl_usart_handle_t usart_handle;

static constexpr uint32_t kUartTxCompleteFlag = 1;
static osThreadId_t sUartTaskHandle;
constexpr uint32_t kUartTaskSize = 1024;
static uint8_t uartStack[kUartTaskSize];
static osThread_t sUartTaskControlBlock;
constexpr osThreadAttr_t kUartTaskAttr = { .name       = "UART",
                                           .attr_bits  = osThreadDetached,
                                           .cb_mem     = &sUartTaskControlBlock,
                                           .cb_size    = osThreadCbSize,
                                           .stack_mem  = uartStack,
                                           .stack_size = kUartTaskSize,
                                           .priority   = osPriorityRealtime };

static osMessageQueueId_t sUartTxQueue;
static osMessageQueue_t sUartTxQueueStruct;
uint8_t sUartTxQueueBuffer[UART_MAX_QUEUE_SIZE * sizeof(UartTxStruct_t)];
constexpr osMessageQueueAttr_t kUartTxQueueAttr = { .cb_mem  = &sUartTxQueueStruct,
                                                    .cb_size = osMessageQueueCbSize,
                                                    .mq_mem  = sUartTxQueueBuffer,
                                                    .mq_size = sizeof(sUartTxQueueBuffer) };


// void callback_event(uint32_t event);
static void uartSendBytes(uint8_t * buffer, uint16_t nbOfBytes);


static uint16_t AvailableDataCount(Fifo_t * fifo)
{
    uint16_t size = 0;

    // if equal there is no data return 0 directly
    if (fifo->Tail != fifo->Head)
    {
        // determine if a wrap around occurred to get the right data size avalaible.
        size = (fifo->Tail < fifo->Head) ? (fifo->MaxSize - fifo->Head + fifo->Tail) : (fifo->Tail - fifo->Head);
    }

    return size;
}

static uint16_t RetrieveFromFifo(Fifo_t * fifo, uint8_t * pData, uint16_t SizeToRead)
{
    assert(fifo);
    assert(pData);
    assert(SizeToRead <= fifo->MaxSize);

    uint16_t ReadSize        = MIN(SizeToRead, AvailableDataCount(fifo));
    uint16_t nBytesBeforWrap = (fifo->MaxSize - fifo->Head);
    if (ReadSize > nBytesBeforWrap)
    {
        memcpy(pData, fifo->pBuffer + fifo->Head, nBytesBeforWrap);
        memcpy(pData + nBytesBeforWrap, fifo->pBuffer, ReadSize - nBytesBeforWrap);
    }
    else
    {
        memcpy(pData, (fifo->pBuffer + fifo->Head), ReadSize);
    }

    fifo->Head = (fifo->Head + ReadSize) % fifo->MaxSize; // increment tail with wraparound

    return ReadSize;
}

static bool InitFifo(Fifo_t * fifo, uint8_t * pDataBuffer, uint16_t bufferSize)
{
    if (fifo == NULL || pDataBuffer == NULL)
    {
        return false;
    }

    fifo->pBuffer = pDataBuffer;
    fifo->MaxSize = bufferSize;
    fifo->Tail = fifo->Head = 0;

    return true;
}

static uint16_t RemainingSpace(Fifo_t * fifo)
{
    return fifo->MaxSize - AvailableDataCount(fifo);
}

static void WriteToFifo(Fifo_t * fifo, uint8_t * pDataToWrite, uint16_t SizeToWrite)
{
    assert(fifo);
    assert(pDataToWrite);
    assert(SizeToWrite <= fifo->MaxSize);

    // Overwrite is not allowed
    if (RemainingSpace(fifo) >= SizeToWrite)
    {
        uint16_t nBytesBeforWrap = (fifo->MaxSize - fifo->Tail);
        if (SizeToWrite > nBytesBeforWrap)
        {
            // The number of bytes to write is bigger than the remaining bytes
            // in the buffer, we have to wrap around
            memcpy(fifo->pBuffer + fifo->Tail, pDataToWrite, nBytesBeforWrap);
            memcpy(fifo->pBuffer, pDataToWrite + nBytesBeforWrap, SizeToWrite - nBytesBeforWrap);
        }
        else
        {
            memcpy(fifo->pBuffer + fifo->Tail, pDataToWrite, SizeToWrite);
        }

        fifo->Tail = (fifo->Tail + SizeToWrite) % fifo->MaxSize; // increment tail with wraparound
    }
}
/*******************************************************************************
 * Callback function triggered on data Transfer and reception
 ******************************************************************************/
// void callback_event(uint32_t event)
// {
//     switch (event)
//     {
//     case SL_USART_EVENT_SEND_COMPLETE:
//         osThreadFlagsSet(sUartTaskHandle, kUartTxCompleteFlag);
//         break;
//     case SL_USART_EVENT_RECEIVE_COMPLETE:
// #ifdef ENABLE_CHIP_SHELL
//         chip::NotifyShellProcess();
// #endif
//     case SL_USART_EVENT_TRANSFER_COMPLETE:
//         break;
//     }
// }

void cache_uart_rx_data(char character) {
    if (RemainingSpace(&sReceiveFifo) >= 1)
    {
        WriteToFifo(&sReceiveFifo,(uint8_t *) &character, 1);
    }
#ifdef ENABLE_CHIP_SHELL
    chip::NotifyShellProcess();
#endif // ENABLE_CHIP_SHELL
}

void uartConsoleInit(void)
{
    if (sUartTaskHandle != NULL)
    {
        // Init was already done
        return;
    }

    sUartTxQueue    = osMessageQueueNew(UART_MAX_QUEUE_SIZE, sizeof(UartTxStruct_t), &kUartTxQueueAttr);
    sUartTaskHandle = osThreadNew(uartMainLoop, nullptr, &kUartTaskAttr);

    // Init a fifo for the data received on the uart
    InitFifo(&sReceiveFifo, sRxFifoBuffer, MAX_BUFFER_SIZE);
    assert(sUartTaskHandle);
    assert(sUartTxQueue);
}

/*
 *   @brief Read the data available from the console Uart
 *   @param Buffer that contains the data to write, number bytes to write.
 *   @return Amount of bytes written or ERROR (-1)
 */
int16_t uartConsoleWrite(const char * Buf, uint16_t BufLength)
{
    if (Buf == NULL || BufLength < 1 || BufLength > UART_TX_MAX_BUF_LEN)
    {
        return UART_CONSOLE_ERR;
    }

    UartTxStruct_t workBuffer;
    memcpy(workBuffer.data, Buf, BufLength);
    workBuffer.length = BufLength;

    if (osMessageQueuePut(sUartTxQueue, &workBuffer, osPriorityNormal, 0) == osOK)
    {
        return BufLength;
    }

    return UART_CONSOLE_ERR;
}

/**
 * @brief Write Logs to the Uart. Appends a return character
 *
 * @param log pointer to the logs
 * @param length number of bytes to write
 * @return int16_t Amount of bytes written or ERROR (-1)
 */
int16_t uartLogWrite(const char * log, uint16_t length)
{
    if (log == NULL || length < 1 || (length + 2) > UART_TX_MAX_BUF_LEN)
    {
        return UART_CONSOLE_ERR;
    }

    UartTxStruct_t workBuffer;
    memcpy(workBuffer.data, log, length);
    memcpy(workBuffer.data + length, "\r\n", 2);
    workBuffer.length = length + 2;

    if (osMessageQueuePut(sUartTxQueue, &workBuffer, osPriorityNormal, 0) == osOK)
    {
        return length;
    }

    return UART_CONSOLE_ERR;
}

/*
 *   @brief Read the data available from the console Uart
 *   @param Buffer for the data to be read, number bytes to read.
 *   @return Amount of bytes that was read from the rx fifo or ERROR (-1)
 */
int16_t uartConsoleRead(char * Buf, uint16_t NbBytesToRead)
{
    uint8_t * data;

    if (Buf == NULL || NbBytesToRead < 1) {
        return UART_CONSOLE_ERR;
    }

    return (int16_t) RetrieveFromFifo(&sReceiveFifo, (uint8_t *) Buf, NbBytesToRead);
}

void uartMainLoop(void * args)
{
    UartTxStruct_t workBuffer;

    while (1)
    {

        osStatus_t eventReceived = osMessageQueueGet(sUartTxQueue, &workBuffer, nullptr, osWaitForever);
        while (eventReceived == osOK)
        {
            uartSendBytes(workBuffer.data, workBuffer.length);
            eventReceived = osMessageQueueGet(sUartTxQueue, &workBuffer, nullptr, 0);
        }
    }
}

/**
 * @brief Send Bytes to UART. This blocks the UART task.
 *
 * @param buffer pointer to the buffer containing the data
 * @param nbOfBytes number of bytes to send
 */
void uartSendBytes(uint8_t * buffer, uint16_t nbOfBytes)
{
    buffer[nbOfBytes] = '\0';
    Board_UARTPutSTR((uint8_t *)buffer);
}

#ifdef __cplusplus
}
#endif