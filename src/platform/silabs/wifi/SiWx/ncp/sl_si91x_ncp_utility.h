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

/*
 * This file contains all the functions specific to the MG24 family for
 * multiplexing the SPI port with WiFi NCP and other WSTK
 * devices such as External Flash and LCD.
 * That can be extended to other families as well.
 */
#pragma once

#include <platform/silabs/wifi/ncp/spi_multiplex.h>

#include "silabs_utils.h"
#include "sl_status.h"

#define USART_INITSYNC_BAUDRATE 12500000

#if SL_SPICTRL_MUX
/**
 * @fn   sl_si91x_host_spi_multiplex_init(void)
 * @brief Initialize the SPI multiplexing for the WiFi NCP and other devices.
 *        This function is called during the initialization of the WiFi NCP.
 * @return None
 */
sl_status_t sl_si91x_host_spi_multiplex_init(void);
#endif // SL_SPICTRL_MUX
