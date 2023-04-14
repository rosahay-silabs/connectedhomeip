/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "sl_wfx_configuration_defaults.h"

#include "sl_wfx.h"
#include "sl_wfx_board.h"
#include "sl_wfx_host_api.h"

#include "dmadrv.h"
#include "em_bus.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "spidrv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AppConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "gpiointerrupt.h"
#include "sl_wfx_board.h"
#include "sl_wfx_host.h"
#include "sl_wfx_task.h"
#include "wfx_host_events.h"


#if SL_WIFI
#include "spi_multiplex.h"
StaticSemaphore_t spi_sem_peripharal;
SemaphoreHandle_t spi_sem_sync_hdl;
#endif /* SL_WIFI */

#define USART SL_WFX_HOST_PINOUT_SPI_PERIPHERAL

StaticSemaphore_t xEfrSpiSemaBuffer;
static SemaphoreHandle_t spiTransferLock;
static TaskHandle_t spiInitiatorTaskHandle = NULL;

static bool spi_enabled = false;

#if defined(EFR32MG12)
uint8_t wirq_irq_nb = SL_WFX_HOST_PINOUT_SPI_IRQ;
#elif defined(EFR32MG24)
uint8_t wirq_irq_nb = SL_WFX_HOST_PINOUT_SPI_WIRQ_PIN; // SL_WFX_HOST_PINOUT_SPI_WIRQ_PIN;
#endif

#define PIN_OUT_SET 1
#define PIN_OUT_CLEAR 0

/****************************************************************************
 * @fn  sl_status_t sl_wfx_host_init_bus(void)
 * @brief
 *  Initialize SPI peripheral
 * @param[in] None
 * @return returns SL_STATUS_OK
 *****************************************************************************/
sl_status_t sl_wfx_host_init_bus(void)
{
    spi_enabled     = true;
    spiTransferLock = xSemaphoreCreateBinaryStatic(&xEfrSpiSemaBuffer);
    xSemaphoreGive(spiTransferLock);

    spi_sem_sync_hdl = xSemaphoreCreateBinaryStatic(&spi_sem_peripharal);
    xSemaphoreGive(spi_sem_sync_hdl);
    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn  sl_status_t sl_wfx_host_deinit_bus(void)
 * @brief
 *     De-initialize SPI peripheral and DMAs
 * @param[in] None
 * @return returns SL_STATUS_OK
 *****************************************************************************/
sl_status_t sl_wfx_host_deinit_bus(void)
{
    vSemaphoreDelete(spiTransferLock);
    vSemaphoreDelete(spi_sem_sync_hdl);
    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn  sl_status_t sl_wfx_host_spi_cs_assert()
 * @brief
 *     Assert chip select.
 * @param[in] None
 * @return returns SL_STATUS_OK
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_cs_assert()
{
    configASSERT(spi_sem_sync_hdl);
    if (xSemaphoreTake(spi_sem_sync_hdl, portMAX_DELAY) != pdTRUE)
    {
        return SL_STATUS_TIMEOUT;
    }
    spi_drv_reinit(SL_BIT_RATE_EXP_HDR);
    GPIO_PinOutClear(SL_SPIDRV_EXP_CS_PORT, SL_SPIDRV_EXP_CS_PIN);
    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn  sl_status_t sl_wfx_host_spi_cs_deassert()
 * @brief
 *     De-Assert chip select.
 * @param[in] None
 * @return returns SL_STATUS_OK
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_cs_deassert()
{
    GPIO_PinOutSet(SL_SPIDRV_EXP_CS_PORT, SL_SPIDRV_EXP_CS_PIN);
    xSemaphoreGive(spi_sem_sync_hdl);
    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn  static void spi_dmaTransferComplete(SPIDRV_HandleData_t * pxHandle, Ecode_t transferStatus, int itemsTransferred)
 * @brief
 *     The callback for SPIDRV_Callback_t function
 * @param[in]  pxHandle: spidrv instance handle
 * @param[in]  transferStatus: status of the SPI transfer
 * @param[in]  itemsTransferred: number of bytes transferred
 * @return None
 *****************************************************************************/
static void spi_dmaTransferComplete(SPIDRV_HandleData_t * pxHandle, Ecode_t transferStatus, int itemsTransferred)
{
    configASSERT(spiInitiatorTaskHandle != NULL);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(spiInitiatorTaskHandle, &xHigherPriorityTaskWoken);
    spiInitiatorTaskHandle = NULL;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/****************************************************************************
 * @fn  sl_status_t sl_wfx_host_spi_transfer_no_cs_assert(sl_wfx_host_bus_transfer_type_t type,
                                                  uint8_t *header,
                                                  uint16_t header_length,
                                                  uint8_t *buffer,
                                                  uint16_t buffer_length)
 * @brief
 * WFX SPI transfer implementation
 * @param[in] type:
 * @param[in] header:
 * @param[in] header_length:
 * @param[in] buffer:
 * @param[in] buffer_length:
 * @return  returns SL_STATUS_OK if successful,
 *          SL_STATUS_FAIL otherwise
 *****************************************************************************/
sl_status_t sl_wfx_host_spi_transfer_no_cs_assert(sl_wfx_host_bus_transfer_type_t type, uint8_t * header, uint16_t header_length,
                                                  uint8_t * buffer, uint16_t buffer_length)
{
    if (header_length <= 0 || buffer_length <= 0)
    {

        return SL_STATUS_INVALID_PARAMETER;
    }
    if (xSemaphoreTake(spiTransferLock, portMAX_DELAY) != pdTRUE)
    {

        return SL_STATUS_BUSY;
    }
    // no other task should be waiting for dma completion
    configASSERT(spiInitiatorTaskHandle == NULL);
    spiInitiatorTaskHandle = xTaskGetCurrentTaskHandle();

    Ecode_t spiError;
    const bool is_read = (type == SL_WFX_BUS_READ);
    if (is_read)
    {
        spiError = SPIDRV_MReceive(SL_SPIDRV_HANDLE, buffer, buffer_length, spi_dmaTransferComplete);
    }
    else
    {
        spiError = SPIDRV_MTransmit(SL_SPIDRV_HANDLE, buffer, buffer_length, spi_dmaTransferComplete);
    }

    if (ECODE_EMDRV_SPIDRV_OK != spiError)
    {
        spiInitiatorTaskHandle = NULL;

        return SL_STATUS_FAIL;
    }
    // slave (WF200) expects a synchronous operation
    // wait for notification from dma completition block
    if (ulTaskNotifyTake(pdTRUE, SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS) != pdPASS)
    {
        int itemsTransferred = 0;
        int itemsRemaining   = 0;
        SPIDRV_GetTransferStatus(SL_SPIDRV_HANDLE, &itemsTransferred, &itemsRemaining);
        SPIDRV_AbortTransfer(SL_SPIDRV_HANDLE);

        return SL_STATUS_TIMEOUT;
    }
    xSemaphoreGive(spiTransferLock);

    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn   void sl_wfx_host_start_platform_interrupt(void)
 * @brief
 * Enable WFX interrupt
 * @param[in]  none
 * @return None
 *****************************************************************************/
void sl_wfx_host_start_platform_interrupt(void)
{
    // Enable (and clear) the bus interrupt
    GPIO_ExtIntConfig(SL_WFX_HOST_PINOUT_SPI_WIRQ_PORT, SL_WFX_HOST_PINOUT_SPI_WIRQ_PIN, wirq_irq_nb, true, false, true);
}

/****************************************************************************
 * @fn   sl_status_t sl_wfx_host_disable_platform_interrupt(void)
 * @brief
 * Disable WFX interrupt
 * @param[in]  None
 * @return  returns SL_STATUS_OK if successful,
 *          SL_STATUS_FAIL otherwise
 *****************************************************************************/
sl_status_t sl_wfx_host_disable_platform_interrupt(void)
{
    GPIO_IntDisable(1 << wirq_irq_nb);
    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn   sl_status_t sl_wfx_host_enable_platform_interrupt(void)
 * @brief
 *      enable the platform interrupt
 * @param[in]  None
 * @return  returns SL_STATUS_OK if successful,
 *          SL_STATUS_FAIL otherwise
 *****************************************************************************/
sl_status_t sl_wfx_host_enable_platform_interrupt(void)
{
    GPIO_IntEnable(1 << wirq_irq_nb);
    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn   sl_status_t sl_wfx_host_enable_spi(void)
 * @brief
 *       enable spi
 * @param[in]  None
 * @return  returns SL_STATUS_OK if successful,
 *          SL_STATUS_FAIL otherwise
 *****************************************************************************/
sl_status_t sl_wfx_host_enable_spi(void)
{
    if (spi_enabled == false)
    {
        spi_enabled = true;
    }
    return SL_STATUS_OK;
}

/****************************************************************************
 * @fn   sl_status_t sl_wfx_host_disable_spi(void)
 * @brief
 *       disable spi
 * @param[in]  None
 * @return  returns SL_STATUS_OK if successful,
 *          SL_STATUS_FAIL otherwise
 *****************************************************************************/
sl_status_t sl_wfx_host_disable_spi(void)
{
    if (spi_enabled == true)
    {
        spi_enabled = false;
    }
    return SL_STATUS_OK;
}

/*
 * IRQ for SPI callback
 * Clear the Interrupt and wake up the task that
 * handles the actions of the interrupt (typically - wfx_bus_task ())
 */
static void sl_wfx_spi_wakeup_irq_callback(uint8_t irqNumber)
{
    BaseType_t bus_task_woken;
    uint32_t interrupt_mask;

    if (irqNumber != wirq_irq_nb)
        return;
    // Get and clear all pending GPIO interrupts
    interrupt_mask = GPIO_IntGet();
    GPIO_IntClear(interrupt_mask);
    bus_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(wfx_wakeup_sem, &bus_task_woken);
    vTaskNotifyGiveFromISR(wfx_bus_task_handle, &bus_task_woken);
    portYIELD_FROM_ISR(bus_task_woken);
}

/****************************************************************************
 * Init some actions pins to the WF-200 expansion board
 *****************************************************************************/
void sl_wfx_host_gpio_init(void)
{
    // Enable GPIO clock.
    CMU_ClockEnable(cmuClock_GPIO, true);

#if defined(EFR32MG24)
    // configure WF200 CS pin.
    GPIO_PinModeSet(SL_SPIDRV_EXP_CS_PORT, SL_SPIDRV_EXP_CS_PIN, gpioModePushPull, 1);
#endif
    // Configure WF200 reset pin.
    GPIO_PinModeSet(SL_WFX_HOST_PINOUT_RESET_PORT, SL_WFX_HOST_PINOUT_RESET_PIN, gpioModePushPull, 0);
    // Configure WF200 WUP pin.
    GPIO_PinModeSet(SL_WFX_HOST_PINOUT_WUP_PORT, SL_WFX_HOST_PINOUT_WUP_PIN, gpioModePushPull, 0);

    // GPIO used as IRQ.
    GPIO_PinModeSet(SL_WFX_HOST_PINOUT_SPI_WIRQ_PORT, SL_WFX_HOST_PINOUT_SPI_WIRQ_PIN, gpioModeInputPull, 0);
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

    // Set up interrupt based callback function - trigger on both edges.
    GPIOINT_Init();
    GPIO_ExtIntConfig(SL_WFX_HOST_PINOUT_SPI_WIRQ_PORT, SL_WFX_HOST_PINOUT_SPI_WIRQ_PIN, wirq_irq_nb, true, false,
                      false); /* Don't enable it */

    GPIOINT_CallbackRegister(wirq_irq_nb, sl_wfx_spi_wakeup_irq_callback);

    // Change GPIO interrupt priority (FreeRTOS asserts unless this is done here!)
    NVIC_ClearPendingIRQ(1 << wirq_irq_nb);
    NVIC_SetPriority(GPIO_EVEN_IRQn, 5);
    NVIC_SetPriority(GPIO_ODD_IRQn, 5);
}
