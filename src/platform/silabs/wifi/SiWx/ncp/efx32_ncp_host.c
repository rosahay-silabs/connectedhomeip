/*******************************************************************************
 * @file  efx32_ncp_host.c
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include <platform/silabs/wifi/SiWx/ncp/sl_board_configuration.h>
#include <platform/silabs/wifi/ncp/spi_multiplex.h>

#include <platform/silabs/wifi/SiWx/ncp/sl_si91x_ncp_utility.h>

#include <stdbool.h>
#include <string.h>

#include "cmsis_os2.h"
#include "dmadrv.h"
#include "em_cmu.h"
#include "em_core.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "gpiointerrupt.h"
#include "sl_constants.h"
#include "sl_rsi_utility.h"
#include "sl_si91x_host_interface.h"
#include "sl_si91x_status.h"
#include "sl_spidrv_exp_config.h"
#include "sl_spidrv_instances.h"
#include "sl_status.h"
#include "sl_wifi_constants.h"
#include "spidrv.h"

#if defined(SL_CATLOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif

#ifdef SL_BOARD_NAME
#include "sl_board_control.h"
#endif // SL_BOARD_NAME

#define MAX_DATA_PACKET_SIZE 1800

static const uint16_t ncp_transfer_timeout_ms = 1000;

// use SPI handle for EXP header (configured in project settings)
extern SPIDRV_Handle_t sl_spidrv_exp_handle;
#define SPI_HANDLE sl_spidrv_exp_handle

static osSemaphoreId_t transfer_done_semaphore = NULL;
static osMutexId_t ncp_transfer_mutex          = NULL;

static sl_si91x_host_init_configuration init_config = { 0 };
static uint8_t dummy_buffer[MAX_DATA_PACKET_SIZE]   = { 0 };

static void gpio_interrupt(uint8_t interrupt_number)
{
    UNUSED_PARAMETER(interrupt_number);
    if (NULL != init_config.rx_irq)
    {
        init_config.rx_irq();
    }
    return;
}

static void spi_dma_callback(struct SPIDRV_HandleData * handle, Ecode_t transferStatus, int itemsTransferred)
{
    UNUSED_PARAMETER(handle);
    UNUSED_PARAMETER(transferStatus);
    UNUSED_PARAMETER(itemsTransferred);
#if defined(SL_CATLOG_POWER_MANAGER_PRESENT)
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
#endif
    osSemaphoreRelease(transfer_done_semaphore);
    return;
}

static void efx32_spi_init(void)
{
    SPIDRV_SetBitrate(SPI_HANDLE, USART_INITSYNC_BAUDRATE);

    // Configure SPI bus pins
    GPIO_PinModeSet(SPI_MISO_PIN.port, SPI_MISO_PIN.pin, gpioModeInput, 0);
    GPIO_PinModeSet(SPI_MOSI_PIN.port, SPI_MOSI_PIN.pin, gpioModePushPull, 0);
    GPIO_PinModeSet(SPI_CLOCK_PIN.port, SPI_CLOCK_PIN.pin, gpioModePushPullAlternate, 0);
    GPIO_PinModeSet(SPI_CS_PIN.port, SPI_CS_PIN.pin, gpioModePushPull, 1);

    // configure packet pending interrupt priority
    NVIC_SetPriority(GPIO_ODD_IRQn, PACKET_PENDING_INT_PRI);
    GPIOINT_CallbackRegister(INTERRUPT_PIN.pin, gpio_interrupt);
    GPIO_PinModeSet(INTERRUPT_PIN.port, INTERRUPT_PIN.pin, gpioModeInputPullFilter, 0);

    // Configure the GPIO external interrupt for active high configuration
    GPIO_ExtIntConfig(INTERRUPT_PIN.port, INTERRUPT_PIN.pin, INTERRUPT_PIN.pin, true, false, true);

    return;
}

void sl_si91x_host_set_sleep_indicator(void)
{
    GPIO_PinOutSet(SLEEP_CONFIRM_PIN.port, SLEEP_CONFIRM_PIN.pin);
}

void sl_si91x_host_clear_sleep_indicator(void)
{
    GPIO_PinOutClear(SLEEP_CONFIRM_PIN.port, SLEEP_CONFIRM_PIN.pin);
}

uint32_t sl_si91x_host_get_wake_indicator(void)
{
    return GPIO_PinInGet(WAKE_INDICATOR_PIN.port, WAKE_INDICATOR_PIN.pin);
}

sl_status_t sl_si91x_host_init(const sl_si91x_host_init_configuration * config)
{
#if SL_SPICTRL_MUX
    sl_si91x_host_spi_multiplex_init();
#endif // SL_SPICTRL_MUX

    init_config.rx_irq      = config->rx_irq;
    init_config.rx_done     = config->rx_done;
    init_config.boot_option = config->boot_option;

    // Enable clock (not needed on xG21)
    CMU_ClockEnable(cmuClock_GPIO, true);

    if (transfer_done_semaphore == NULL)
    {
        // initialize and acquire the semaphore
        transfer_done_semaphore = osSemaphoreNew(1, 1, NULL);
    }

    if (ncp_transfer_mutex == NULL)
    {
        ncp_transfer_mutex = osMutexNew(NULL);
    }

    efx32_spi_init();

    // Start reset line low
    GPIO_PinModeSet(RESET_PIN.port, RESET_PIN.pin, gpioModeWiredAnd, 0);

    // Configure interrupt, sleep and wake confirmation pins
    GPIO_PinModeSet(SLEEP_CONFIRM_PIN.port, SLEEP_CONFIRM_PIN.pin, gpioModeWiredOrPullDown, 1);
    GPIO_PinModeSet(WAKE_INDICATOR_PIN.port, WAKE_INDICATOR_PIN.pin, gpioModeWiredOrPullDown, 0);

    return SL_STATUS_OK;
}

sl_status_t sl_si91x_host_deinit(void)
{
    return SL_STATUS_OK;
}

void sl_si91x_host_enable_high_speed_bus() {}

/*==================================================================*/
/**
 * @fn         sl_status_t sl_si91x_host_spi_transfer(const void *tx_buffer, void *rx_buffer, uint16_t buffer_length)
 * @param[in]  uint8_t *tx_buff, pointer to the buffer with the data to be transferred
 * @param[in]  uint8_t *rx_buff, pointer to the buffer to store the data received
 * @param[in]  uint16_t transfer_length, Number of bytes to send and receive
 * @param[in]  uint8_t mode, To indicate mode 8 BIT/32 BIT mode transfers.
 * @param[out] None
 * @return     0, 0=success
 * @section description
 * This API is used to transfer/receive data to the Wi-Fi module through the SPI interface.
 */

sl_status_t sl_si91x_host_spi_cs_assert()
{
#if SL_SPICTRL_MUX
    sl_wfx_host_spi_cs_assert();
#endif // SL_SPICTRL_MUX
    return SL_STATUS_OK;
}

sl_status_t sl_si91x_host_spi_cs_deassert()
{
#if SL_SPICTRL_MUX
    sl_wfx_host_spi_cs_deassert();
#endif // SL_SPICTRL_MUX
    return SL_STATUS_OK;
}

sl_status_t sl_si91x_host_spi_transfer(const void * tx_buffer, void * rx_buffer, uint16_t buffer_length)
{
    osMutexAcquire(ncp_transfer_mutex, osWaitForever);

    uint8_t * tx_buf = (tx_buffer != NULL) ? (uint8_t *) tx_buffer : dummy_buffer;
    uint8_t * rx_buf = (rx_buffer != NULL) ? (uint8_t *) rx_buffer : dummy_buffer;
    Ecode_t status   = ECODE_EMDRV_SPIDRV_OK;

    // If the buffer length is greater than the high speed transfer threshold, use DMA transfer
    status = SPIDRV_MTransfer(SPI_HANDLE, tx_buf, rx_buf, buffer_length, spi_dma_callback);

    if (ECODE_EMDRV_SPIDRV_OK != status)
    {
        SILABS_LOG("ERR: SPI failed with error:%x (tx%x rx%x)", status, (uint32_t) tx_buf, (uint32_t) rx_buf);
        osMutexRelease(ncp_transfer_mutex);

        return SL_STATUS_FAIL;
    }

    if (osSemaphoreAcquire(transfer_done_semaphore, ncp_transfer_timeout_ms) != osOK)
    {
        int itemsTransferred = 0;
        int itemsRemaining   = 0;
        SPIDRV_GetTransferStatus(SPI_HANDLE, &itemsTransferred, &itemsRemaining);
        SILABS_LOG("ERR: SPI timed out %d/%d (rx%x rx%x)", itemsTransferred, itemsRemaining, (uint32_t) tx_buf, (uint32_t) rx_buf);
        SPIDRV_AbortTransfer(SPI_HANDLE);
        osMutexRelease(ncp_transfer_mutex);

        return SL_STATUS_SPI_BUSY;
    }

    osMutexRelease(ncp_transfer_mutex);

    return SL_STATUS_OK;
}

void sl_si91x_host_hold_in_reset(void)
{
    GPIO_PinOutClear(RESET_PIN.port, RESET_PIN.pin);
}

void sl_si91x_host_release_from_reset(void)
{
    GPIO_PinOutSet(RESET_PIN.port, RESET_PIN.pin);
}

void sl_si91x_host_enable_bus_interrupt(void)
{
    NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

void sl_si91x_host_disable_bus_interrupt(void)
{
    NVIC_DisableIRQ(GPIO_ODD_IRQn);
}

bool sl_si91x_host_is_in_irq_context(void)
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0U;
}
