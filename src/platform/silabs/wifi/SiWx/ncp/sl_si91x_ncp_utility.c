/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os2.h"

#include "dmadrv.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_core.h"
#include "em_device.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_usart.h"
#include "gpiointerrupt.h"
#include "sl_si91x_host_interface.h"
#include "sl_spidrv_exp_config.h"
#include "spidrv.h"

#ifdef SL_BOARD_NAME
#include "sl_board_control.h"
#endif // SL_BOARD_NAME
#include "sl_board_configuration.h"

#include "sl_device_init_clocks.h"
#include "sl_device_init_hfxo.h"

#include "silabs_utils.h"

#include <platform/silabs/wifi/ncp/spi_multiplex.h>
#include <platform/silabs/wifi/SiWx/ncp/sl_si91x_ncp_utility.h>

#if SL_BTLCTRL_MUX
#include "btl_interface.h"
#endif // SL_BTLCTRL_MUX

#if SL_LCDCTRL_MUX
#include "sl_memlcd.h"
#include "sl_memlcd_display.h"

#define SL_SPIDRV_LCD_BITRATE SL_MEMLCD_SCLK_FREQ
#endif // SL_LCDCTRL_MUX

#if SL_MX25CTRL_MUX
#include "sl_mx25_flash_shutdown_usart_config.h"
#endif // SL_MX25CTRL_MUX

#if SL_SPICTRL_MUX
osMutexId_t spi_peripheral_mutex = 0;
#endif // SL_SPICTRL_MUX

#if SL_LCDCTRL_MUX
sl_status_t sl_wfx_host_pre_lcd_spi_transfer(void)
{
#if SL_SPICTRL_MUX
    osMutexAcquire(spi_peripheral_mutex, osWaitForever);
#endif // SL_SPICTRL_MUX
    sl_status_t status = sl_board_enable_display();
    if (SL_STATUS_OK == status)
    {
        SPIDRV_SetBaudrate(SL_SPIDRV_LCD_BITRATE);
    }
    return status;
}

sl_status_t sl_wfx_host_post_lcd_spi_transfer(void)
{
    sl_status_t status = sl_board_disable_display();
#if SL_SPICTRL_MUX
    osMutexRelease(spi_peripheral_mutex);
#endif // SL_SPICTRL_MUX
    return status;
}
#endif // SL_LCDCTRL_MUX

#if SL_SPICTRL_MUX
void SPIDRV_SetBaudrate(uint32_t baudrate)
{
    if (USART_BaudrateGet(SPI_USART) != baudrate)
    {
        USART_InitSync_TypeDef usartInit = USART_INITSYNC_DEFAULT;
        usartInit.msbf                   = true;
        usartInit.baudrate               = baudrate;
        USART_InitSync(SPI_USART, &usartInit);
    }
}

sl_status_t sl_si91x_host_spi_multiplex_init(void)
{
#if SL_SPICTRL_MUX
    if (spi_peripheral_mutex == NULL)
    {
        spi_peripheral_mutex = osMutexNew(NULL);
    }
#endif /* SL_SPICTRL_MUX */
    return SL_STATUS_OK;
}

void sl_si91x_host_spi_cs_assert(void)
{
#if SL_SPICTRL_MUX
    osMutexAcquire(spi_peripheral_mutex, osWaitForever);
#endif /* SL_SPICTRL_MUX */
    SPIDRV_SetBaudrate(USART_INITSYNC_BAUDRATE);
    GPIO_PinOutClear(SL_SPIDRV_EXP_CS_PORT, SL_SPIDRV_EXP_CS_PIN);
}

void sl_si91x_host_spi_cs_deassert(void)
{
    GPIO_PinOutSet(SL_SPIDRV_EXP_CS_PORT, SL_SPIDRV_EXP_CS_PIN);
#if SL_SPICTRL_MUX
    osMutexRelease(spi_peripheral_mutex);
#endif
}
#endif // SL_SPICTRL_MUX

#if SL_BTLCTRL_MUX
sl_status_t sl_wfx_host_pre_bootloader_spi_transfer(void)
{
#if SL_SPICTRL_MUX
    sl_si91x_host_spi_cs_deassert();
    osMutexAcquire(spi_peripheral_mutex, osWaitForever);
#endif // SL_SPICTRL_MUX
    int32_t status = BOOTLOADER_OK;
#if defined(CHIP_9117)
    LDMA_Init_t ldma_init = LDMA_INIT_DEFAULT;
    LDMA_Init(&ldma_init);
#else
    // bootloader_init takes care of SPIDRV_Init()
    status = bootloader_init();
#endif
    if (status != BOOTLOADER_OK)
    {
        SILABS_LOG("bootloader_init error: %x", status);
#if SL_SPICTRL_MUX
        osMutexRelease(spi_peripheral_mutex);
#endif // SL_SPICTRL_MUX
        return SL_STATUS_FAIL;
    }
#if SL_MX25CTRL_MUX
    sl_wfx_host_spiflash_cs_assert();
#endif // SL_MX25CTRL_MUX
    return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_post_bootloader_spi_transfer(void)
{
    // bootloader_deinit will do USART disable
    int32_t status = bootloader_deinit();
    if (status != BOOTLOADER_OK)
    {
        SILABS_LOG("bootloader_deinit error: %x", status);
#if SL_SPICTRL_MUX
        osMutexRelease(spi_peripheral_mutex);
#endif // SL_SPICTRL_MUX
        return SL_STATUS_FAIL;
    }
    GPIO->USARTROUTE[SL_MX25_FLASH_SHUTDOWN_PERIPHERAL_NO].ROUTEEN = 0;
#if SL_MX25CTRL_MUX
    sl_wfx_host_spiflash_cs_deassert();
#endif // SL_MX25CTRL_MUX
#if SL_SPICTRL_MUX
    osMutexRelease(spi_peripheral_mutex);
#endif // SL_SPICTRL_MUX
    return SL_STATUS_OK;
}
#endif // SL_BTLCTRL_MUX

#if SL_MX25CTRL_MUX
sl_status_t sl_wfx_host_spiflash_cs_assert(void)
{
    GPIO_PinOutClear(SL_MX25_FLASH_SHUTDOWN_CS_PORT, SL_MX25_FLASH_SHUTDOWN_CS_PIN);
    return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_spiflash_cs_deassert(void)
{
    GPIO_PinOutSet(SL_MX25_FLASH_SHUTDOWN_CS_PORT, SL_MX25_FLASH_SHUTDOWN_CS_PIN);
    return SL_STATUS_OK;
}
#endif // SL_MX25CTRL_MUX
