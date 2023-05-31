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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sl_status.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"

#include "wfx_host_events.h"
/*  wifi-sdk
#include "rsi_driver.h"
#include "rsi_wlan_non_rom.h"

#include "rsi_common_apis.h"
#include "rsi_data_types.h"
#include "rsi_nwk.h"
#include "rsi_socket.h"
#include "rsi_utils.h"
#include "rsi_wlan.h"
#include "rsi_wlan_apis.h"
#include "rsi_wlan_config.h"
//#include "rsi_wlan_non_rom.h"
#include "rsi_bootup_config.h"
#include "rsi_error.h"
*/
//#include "cmsis_os2.h"
#include "sl_net.h"
//#include "sl_uart.h"
#include "sl_board_configuration.h"
#include "sl_wifi_types.h"
#include "sl_wifi_callback_framework.h"
#include "sl_wifi_constants.h"
#include "sl_si91x_types.h"
//#include "sl_net_default_values.h"
#include "sl_si91x_host_interface.h"
#include "rsi_ccp_common.h"

#include "dhcp_client.h"
#include "wfx_host_events.h"
#include "wfx_rsi.h"
#include "sl_wifi.h"

struct wfx_rsi wfx_rsi;

/* Rsi driver Task will use as its stack */
//StackType_t driverRsiTaskStack[WFX_RSI_WLAN_TASK_SZ] = { 0 };

/* Structure that will hold the TCB of the wfxRsi Task being created. */
//StaticTask_t driverRsiTaskBuffer;

/* Declare a variable to hold the data associated with the created event group. */
StaticEventGroup_t rsiDriverEventGroup;

bool hasNotifiedIPV6 = false;
#if (CHIP_DEVICE_CONFIG_ENABLE_IPV4)
bool hasNotifiedIPV4 = false;
#endif /* CHIP_DEVICE_CONFIG_ENABLE_IPV4 */
bool hasNotifiedWifiConnectivity = false;

/* Declare a flag to differentiate between after boot-up first IP connection or reconnection */
bool is_wifi_disconnection_event = false;

/* Declare a variable to hold connection time intervals */
uint32_t retryInterval = WLAN_MIN_RETRY_TIMER_MS;
/* wifi-sdk
#if (RSI_BLE_ENABLE)
extern rsi_semaphore_handle_t sl_rs_ble_init_sem;
#endif
*/
/*
 * This file implements the interface to the RSI SAPIs
 */

//#if (BLE_ENABLE)
//extern osSemaphoreId_t sl_rs_ble_init_sem;
//#endif

static sl_wifi_device_configuration_t config;

static uint8_t wfx_rsi_drv_buf[WFX_RSI_BUF_SZ];
wfx_wifi_scan_ext_t * temp_reset;

//wifi-sdk
volatile sl_status_t callback_status = SL_STATUS_OK;
//sl_wifi_interface_t interface     = SL_WIFI_CLIENT_2_4GHZ_INTERFACE;

/******************************************************************
 * @fn   int32_t wfx_rsi_get_ap_info(wfx_wifi_scan_result_t *ap)
 * @brief
 *       Getting the AP details
 * @param[in] ap: access point
 * @return
 *        status
 *********************************************************************/
int32_t wfx_rsi_get_ap_info(wfx_wifi_scan_result_t * ap)
{
/* wifi-sdk
    int32_t status;
    uint8_t rssi;
    ap->security = wfx_rsi.sec.security;
    ap->chan     = wfx_rsi.ap_chan;
    memcpy(&ap->bssid[0], &wfx_rsi.ap_mac.octet[0], BSSID_MAX_STR_LEN);
    status = rsi_wlan_get(RSI_RSSI, &rssi, sizeof(rssi));
    if (status == RSI_SUCCESS)
    {
        ap->rssi = (-1) * rssi;
    }
    return status;
*/
}

/******************************************************************
 * @fn   int32_t wfx_rsi_get_ap_ext(wfx_wifi_scan_ext_t *extra_info)
 * @brief
 *       Getting the AP extra details
 * @param[in] extra info: access point extra information
 * @return
 *        status
 *********************************************************************/
int32_t wfx_rsi_get_ap_ext(wfx_wifi_scan_ext_t * extra_info)
{
#ifdef SiWx917_WIFI
    // TODO: for wisemcu
    return 0;
#else
    int32_t status;
/* wifi-sdk
    uint8_t buff[RSI_RESPONSE_MAX_SIZE] = { 0 };
    status                              = rsi_wlan_get(RSI_WLAN_EXT_STATS, buff, sizeof(buff));
    if (status != RSI_SUCCESS)
    {
        SILABS_LOG("\r\n Failed, Error Code : 0x%lX\r\n", status);
    }
    else
    {
        rsi_wlan_ext_stats_t * test   = (rsi_wlan_ext_stats_t *) buff;
        extra_info->beacon_lost_count = test->beacon_lost_count - temp_reset->beacon_lost_count;
        extra_info->beacon_rx_count   = test->beacon_rx_count - temp_reset->beacon_rx_count;
        extra_info->mcast_rx_count    = test->mcast_rx_count - temp_reset->mcast_rx_count;
        extra_info->mcast_tx_count    = test->mcast_tx_count - temp_reset->mcast_tx_count;
        extra_info->ucast_rx_count    = test->ucast_rx_count - temp_reset->ucast_rx_count;
        extra_info->ucast_tx_count    = test->ucast_tx_count - temp_reset->ucast_tx_count;
        extra_info->overrun_count     = test->overrun_count - temp_reset->overrun_count;
    }
*/
    return status;
#endif
}

/******************************************************************
 * @fn   int32_t wfx_rsi_reset_count()
 * @brief
 *       Getting the driver reset count
 * @param[in] None
 * @return
 *        status
 *********************************************************************/
int32_t wfx_rsi_reset_count()
{
#ifdef SiWx917_WIFI
    // TODO: for wisemcu
    return 0;
#else
/*
    int32_t status;
    uint8_t buff[RSI_RESPONSE_MAX_SIZE] = { 0 };
    status                              = rsi_wlan_get(RSI_WLAN_EXT_STATS, buff, sizeof(buff));
    if (status != RSI_SUCCESS)
    {
        SILABS_LOG("\r\n Failed, Error Code : 0x%lX\r\n", status);
    }
    else
    {
        rsi_wlan_ext_stats_t * test   = (rsi_wlan_ext_stats_t *) buff;
        temp_reset->beacon_lost_count = test->beacon_lost_count;
        temp_reset->beacon_rx_count   = test->beacon_rx_count;
        temp_reset->mcast_rx_count    = test->mcast_rx_count;
        temp_reset->mcast_tx_count    = test->mcast_tx_count;
        temp_reset->ucast_rx_count    = test->ucast_rx_count;
        temp_reset->ucast_tx_count    = test->ucast_tx_count;
        temp_reset->overrun_count     = test->overrun_count;
    }
    return status;
*/
#endif

return RSI_SUCCESS;
}

/******************************************************************
 * @fn   wfx_rsi_disconnect()
 * @brief
 *       Getting the driver disconnect status
 * @param[in] None
 * @return
 *        status
 *********************************************************************/
int32_t wfx_rsi_disconnect()
{
    int32_t status;
/* wifi-sdk
    status = rsi_wlan_disconnect();
*/
    return status;
}

/******************************************************************
 * @fn   wfx_rsi_power_save()
 * @brief
 *       Setting the RS911x in DTIM sleep based mode
 *
 * @param[in] None
 * @return
 *        None
 *********************************************************************/
void wfx_rsi_power_save()
{
/*
    int32_t status = rsi_wlan_power_save_profile(RSI_SLEEP_MODE_2, RSI_MAX_PSP);
    if (status != RSI_SUCCESS)
    {
        SILABS_LOG("Powersave Config Failed, Error Code : 0x%lX", status);
        return;
    }
    SILABS_LOG("Powersave Config Success");
*/
}

/******************************************************************
 * @fn   wfx_rsi_join_cb(uint16_t status, const uint8_t *buf, const uint16_t len)
 * @brief
 *       called when driver join with cb
 * @param[in] status:
 * @param[in] buf:
 * @param[in] len:
 * @return
 *        None
 *********************************************************************/
sl_status_t join_callback_handler(sl_wifi_event_t event, char *result, uint32_t result_length, void *arg)
{
  if (CHECK_IF_EVENT_FAILED(event)) {
    printf("F: Join Event received with %u bytes payload\n", result_length);
    callback_status = *(sl_status_t *)result;
    return SL_STATUS_FAIL;
  }
   /*
         * Join was complete - Do the DHCP
         */
        SILABS_LOG("%s: join completed.", __func__);
  printf("%c: Join Event received with %u bytes payload\n", *result, result_length);
#ifdef RS911X_SOCKETS
        xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_DO_DHCP);
#else
        xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_CONN);
#endif
  callback_status = SL_STATUS_OK;
  return SL_STATUS_OK;
}

/******************************************************************
 * @fn   wfx_rsi_join_cb(uint16_t status, const uint8_t *buf, const uint16_t len)
 * @brief
 *       called when driver join with cb
 * @param[in] status:
 * @param[in] buf:
 * @param[in] len:
 * @return
 *        None
 *********************************************************************/
static void wfx_rsi_join_cb(uint16_t status, const uint8_t * buf, const uint16_t len)
{
    SILABS_LOG("%s: status: %02x", __func__, status);
    wfx_rsi.dev_state &= ~WFX_RSI_ST_STA_CONNECTING;
    temp_reset = (wfx_wifi_scan_ext_t *) malloc(sizeof(wfx_wifi_scan_ext_t));
    memset(temp_reset, 0, sizeof(wfx_wifi_scan_ext_t));
    if (status != RSI_SUCCESS)
    {
        /*
         * We should enable retry.. (Need config variable for this)
         */
        SILABS_LOG("%s: failed. retry: %d", __func__, wfx_rsi.join_retries);
        wfx_retry_interval_handler(is_wifi_disconnection_event, wfx_rsi.join_retries++);
        if (is_wifi_disconnection_event || wfx_rsi.join_retries <= WFX_RSI_CONFIG_MAX_JOIN)
            xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_START_JOIN);
    }
    else
    {
        /*
         * Join was complete - Do the DHCP
         */
        SILABS_LOG("%s: join completed.", __func__);
#ifdef RS911X_SOCKETS
        xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_DO_DHCP);
#else
        xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_CONN);
#endif
        wfx_rsi.join_retries = 0;
        retryInterval        = WLAN_MIN_RETRY_TIMER_MS;
    }
}

/******************************************************************
 * @fn  wfx_rsi_join_fail_cb(uint16_t status, uint8_t *buf, uint32_t len)
 * @brief
 *       called when driver fail to join with cb
 * @param[in] status:
 * @param[in] buf:
 * @param[in] len:
 * @return
 *        None
 *********************************************************************/
static void wfx_rsi_join_fail_cb(uint16_t status, uint8_t * buf, uint32_t len)
{
    SILABS_LOG("%s: error: failed status: %02x", __func__, status);
    wfx_rsi.join_retries += 1;
    wfx_rsi.dev_state &= ~(WFX_RSI_ST_STA_CONNECTING | WFX_RSI_ST_STA_CONNECTED);
    is_wifi_disconnection_event = true;
    xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_START_JOIN);
}
#ifdef RS911X_SOCKETS

/******************************************************************
 * @fn  wfx_rsi_ipchange_cb(uint16_t status, uint8_t *buf, uint32_t len)
 * @brief
 *       DHCP should end up here
 * @param[in] status:
 * @param[in] buf:
 * @param[in] len:
 * @return
 *        None
 *********************************************************************/
static void wfx_rsi_ipchange_cb(uint16_t status, uint8_t * buf, uint32_t len)
{
    SILABS_LOG("%s: status: %02x", __func__, status);
    if (status != RSI_SUCCESS)
    {
        /* Restart DHCP? */
        xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_DO_DHCP);
    }
    else
    {
        wfx_rsi.dev_state |= WFX_RSI_ST_STA_DHCP_DONE;
        xEventGroupSetBits(wfx_rsi.events, WFX_EVT_STA_DHCP_DONE);
    }
}

#else
/*************************************************************************************
 * @fn  wfx_rsi_wlan_pkt_cb(uint16_t status, uint8_t *buf, uint32_t len)
 * @brief
 *      Got RAW WLAN data pkt
 * @param[in]  status:
 * @param[in]  buf:
 * @param[in]  len:
 * @return
 *        None
 *****************************************************************************************/
static void wfx_rsi_wlan_pkt_cb(uint16_t status, uint8_t * buf, uint32_t len)
{
    // SILABS_LOG("%s: status=%d, len=%d", __func__, status, len);
    if (status != RSI_SUCCESS)
    {
        return;
    }
    wfx_host_received_sta_frame_cb(buf, len);
}
#endif /* !Socket support */
#if 0
#if BLE_ENABLE
/*==============================================*/
/**
 * @fn         initialize_device_configuration
 * @brief      fetches the init configuration
 * @param[in]  none.
 * @return     none.
 * @section description
 * This function fetches init configuration for wifi device
 */
void initialize_device_configuration(void)
{
  si91x_boot_configuration_t *boot_config = (si91x_boot_configuration_t *)&(config.boot_config);
  config.boot_option                      = LOAD_NWP_FW;
  config.band                             = SL_SI91X_WIFI_BAND_2_4GHZ;

  boot_config->oper_mode = SL_SI91X_CLIENT_MODE;
  boot_config->coex_mode = SL_SI91X_WLAN_BLE_MODE;

  // fill other parameters from configuration file
#ifdef RSI_M4_INTERFACE
  boot_config->feature_bit_map = (SL_SI91X_FEAT_WPS_DISABLE | RSI_FEATURE_BIT_MAP);
#else
  boot_config->feature_bit_map            = RSI_FEATURE_BIT_MAP;
#endif

#if RSI_TCP_IP_BYPASS
  boot_config->tcp_ip_feature_bit_map = (SL_SI91X_TCP_IP_FEAT_BYPASS | SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT | SL_SI91X_TCP_IP_FEAT_DNS_CLIENT | SL_SI91X_TCP_IP_FEAT_SSL
                      | SL_SI91X_TCP_IP_FEAT_ICMP | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID);
#else
  boot_config->tcp_ip_feature_bit_map     = (RSI_TCP_IP_FEATURE_BIT_MAP | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID);
#endif
  boot_config->custom_feature_bit_map = (SL_SI91X_FEAT_CUSTOM_FEAT_EXTENTION_VALID | RSI_CUSTOM_FEATURE_BIT_MAP);

#ifdef CHIP_9117
  boot_config->ext_custom_feature_bit_map = RSI_EXT_CUSTOM_FEATURE_BIT_MAP;
#else //defaults
#ifdef RSI_M4_INTERFACE
  boot_config->ext_custom_feature_bit_map = (SL_SI91X_EXT_FEAT_256K_MODE | RSI_EXT_CUSTOM_FEATURE_BIT_MAP);
#else
  boot_config->ext_custom_feature_bit_map = (SL_SI91X_EXT_FEAT_384K_MODE | RSI_EXT_CUSTOM_FEATURE_BIT_MAP);
#endif
#endif

#ifdef RSI_PROCESS_MAX_RX_DATA
  boot_config->ext_tcp_ip_feature_bit_map =
    (RSI_EXT_TCPIP_FEATURE_BITMAP | SL_SI91X_CONFIG_FEAT_EXTENTION_VALID | SL_SI91X_EXT_TCP_MAX_RECV_LENGTH);
#else
  boot_config->ext_tcp_ip_feature_bit_map = (RSI_EXT_TCPIP_FEATURE_BITMAP | SL_SI91X_CONFIG_FEAT_EXTENTION_VALID);
#endif
  boot_config->config_feature_bit_map = (SL_SI91X_FEAT_SLEEP_GPIO_SEL_BITMAP | RSI_CONFIG_FEATURE_BITMAP);

  boot_config->bt_feature_bit_map = RSI_BT_FEATURE_BITMAP;
  boot_config->bt_feature_bit_map |= SL_SI91X_ENABLE_BLE_PROTOCOL;
#if (RSI_BT_GATT_ON_CLASSIC)
  boot_config->bt_feature_bit_map |= SL_SI91X_BT_ATT_OVER_CLASSIC_ACL; /* to support att over classic acl link */
#endif
  if (boot_config->coex_mode == SL_SI91X_WLAN_BLE_MODE) {
    boot_config->custom_feature_bit_map |= SL_SI91X_FEAT_CUSTOM_FEAT_EXTENTION_VALID;
    boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_BT_CUSTOM_FEAT_ENABLE;
#if (defined A2DP_POWER_SAVE_ENABLE)
    boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_XTAL_CLK_ENABLE(2);
#endif
    //!ENABLE_BLE_PROTOCOL in bt_feature_bit_map
    boot_config->ble_feature_bit_map =
      (SL_SI91X_BLE_MAX_NBR_SLAVES(RSI_BLE_MAX_NBR_SLAVES) | SL_SI91X_BLE_MAX_NBR_MASTERS(RSI_BLE_MAX_NBR_MASTERS)
       | SL_SI91X_BLE_MAX_NBR_ATT_SERV(RSI_BLE_MAX_NBR_ATT_SERV)
       | SL_SI91X_BLE_MAX_NBR_ATT_REC(RSI_BLE_MAX_NBR_ATT_REC));

    /*Enable BLE custom feature bitmap*/
    boot_config->ble_feature_bit_map |= SL_SI91X_FEAT_BLE_CUSTOM_FEAT_EXTENTION_VALID;
    boot_config->ble_feature_bit_map |= SL_SI91X_BLE_PWR_INX(RSI_BLE_PWR_INX);
    boot_config->ble_feature_bit_map |= SL_SI91X_BLE_PWR_SAVE_OPTIONS(RSI_BLE_PWR_SAVE_OPTIONS);
    boot_config->ble_feature_bit_map |= SL_SI91X_916_BLE_COMPATIBLE_FEAT_ENABLE;

#if RSI_BLE_GATT_ASYNC_ENABLE
    boot_config->ble_feature_bit_map |= SL_SI91X_BLE_GATT_ASYNC_ENABLE;
#endif

    boot_config->ble_ext_feature_bit_map =
      (SL_SI91X_BLE_NUM_CONN_EVENTS(RSI_BLE_NUM_CONN_EVENTS) | SL_SI91X_BLE_NUM_REC_BYTES(RSI_BLE_NUM_REC_BYTES));
#if RSI_BLE_INDICATE_CONFIRMATION_FROM_HOST
    boot_config->ble_ext_feature_bit_map |= SL_SI91X_BLE_INDICATE_CONFIRMATION_FROM_HOST; //indication response from app
#endif
#if RSI_BLE_MTU_EXCHANGE_FROM_HOST
    boot_config->ble_ext_feature_bit_map |=
      SL_SI91X_BLE_MTU_EXCHANGE_FROM_HOST; //MTU Exchange request initiation from app
#endif
#if RSI_BLE_SET_SCAN_RESP_DATA_FROM_HOST
    boot_config->ble_ext_feature_bit_map |= (SL_SI91X_BLE_SET_SCAN_RESP_DATA_FROM_HOST); //Set SCAN Resp Data from app
#endif
#if RSI_BLE_DISABLE_CODED_PHY_FROM_HOST
    boot_config->ble_ext_feature_bit_map |= (SL_SI91X_BLE_DISABLE_CODED_PHY_FROM_HOST); //Disable Coded PHY from app
#endif
#if BLE_SIMPLE_GATT
    boot_config->ble_ext_feature_bit_map |= SL_SI91X_BLE_GATT_INIT;
#endif
  }
  return;
}

#endif
#endif
/*************************************************************************************
 * @fn  static int32_t wfx_rsi_init(void)
 * @brief
 *      driver initialization
 * @param[in]  None
 * @return
 *        None
 *****************************************************************************************/
static int32_t wfx_rsi_init(void)
{
  sl_status_t status;
//#if BLE_ENABLE
  //initialize_device_configuration();
 // status = sl_wifi_init(&config  , default_wifi_event_handler);
//#else
    status = sl_wifi_init(&sl_wifi_default_client_configuration , default_wifi_event_handler);
    if(status != 0){
        SILABS_LOG("init failed **************** %x", status);
    }
//#endif
//  }
  //sl_mac_address_t mac_addr     = { 0 };

  status = sl_wifi_get_mac_address(SL_WIFI_CLIENT_INTERFACE, (sl_mac_address_t *)&wfx_rsi.sta_mac.octet[0]);
//  VERIFY_STATUS_AND_RETURN(status);

//  SILABS_LOG("%x:%x:%x:%x:%x:%x",
//         wfx_rsi.sta_mac.octet[0],
//         wfx_rsi.sta_mac.octet[1],
//         wfx_rsi.sta_mac.octet[2],
//         wfx_rsi.sta_mac.octet[3],
//         wfx_rsi.sta_mac.octet[4],
//         wfx_rsi.sta_mac.octet[5]);
  wfx_rsi.events = xEventGroupCreateStatic(&rsiDriverEventGroup);
    wfx_rsi.dev_state |= WFX_RSI_ST_DEV_READY;
//    SILABS_LOG("%s: RSI: OK", __func__);
    return 0;
}

/*************************************************************************************
 * @fn  void wfx_show_err(char *msg)
 * @brief
 *      driver shows error message
 * @param[in]  msg
 * @return
 *        None
 *****************************************************************************************/
void wfx_show_err(char * msg)
{
    SILABS_LOG("%s: message: %d", __func__, msg);
}

/***************************************************************************************
 * @fn   static void wfx_rsi_save_ap_info()
 * @brief
 *       Saving the details of the AP
 * @param[in]  None
 * @return
 *       None
 *******************************************************************************************/
static void wfx_rsi_save_ap_info() // translation
{
#if 0
//wifi-sdk
    int32_t status;
    rsi_rsp_scan_t rsp;

    status =
        rsi_wlan_scan_with_bitmap_options((int8_t *) &wfx_rsi.sec.ssid[0], AP_CHANNEL_NO_0, &rsp, sizeof(rsp), SCAN_BITMAP_OPTN_1);
    if (status)
    {
        /*
         * Scan is done - failed
         */
    }
    else
    {
        wfx_rsi.sec.security = WFX_SEC_UNSPECIFIED;
        wfx_rsi.ap_chan      = rsp.scan_info->rf_channel;
        memcpy(&wfx_rsi.ap_mac.octet[0], &rsp.scan_info->bssid[0], BSSID_MAX_STR_LEN);
    }

    switch (rsp.scan_info->security_mode)
    {
    case SME_OPEN:
        wfx_rsi.sec.security = WFX_SEC_NONE;
        break;
    case SME_WPA:
    case SME_WPA_ENTERPRISE:
        wfx_rsi.sec.security = WFX_SEC_WPA;
        break;
    case SME_WPA2:
    case SME_WPA2_ENTERPRISE:
        wfx_rsi.sec.security = WFX_SEC_WPA2;
        break;
    case SME_WEP:
        wfx_rsi.sec.security = WFX_SEC_WEP;
        break;
    case SME_WPA3:
    case SME_WPA3_TRANSITION:
        wfx_rsi.sec.security = WFX_SEC_WPA3;
        break;
    default:
        wfx_rsi.sec.security = WFX_SEC_UNSPECIFIED;
        break;
    }

    SILABS_LOG("%s: WLAN: connecting to %s==%s, sec=%d, status=%02x", __func__, &wfx_rsi.sec.ssid[0], &wfx_rsi.sec.passkey[0],
                wfx_rsi.sec.security, status);
#endif
}

/********************************************************************************************
 * @fn   static void wfx_rsi_do_join(void)
 * @brief
 *        Start an async Join command
 * @return
 *        None
 **********************************************************************************************/
static void wfx_rsi_do_join(void)
{
    int32_t status;
  //  rsi_security_mode_t connect_security_mode;

    if (wfx_rsi.dev_state & (WFX_RSI_ST_STA_CONNECTING | WFX_RSI_ST_STA_CONNECTED))
    {
        SILABS_LOG("%s: not joining - already in progress", __func__);
    }
    else
    {
#if 0
//wifi-sdk
        switch (wfx_rsi.sec.security)
        {
        case WFX_SEC_WEP:
            connect_security_mode = SL_WIFI_WEP;
            break;
        case WFX_SEC_WPA:
        case WFX_SEC_WPA2:
            connect_security_mode = SL_WIFI_WPA_WPA2_MIXED;
            break;
        case WFX_SEC_WPA3:
            connect_security_mode = SL_WIFI_WPA3;
            break;
        case WFX_SEC_NONE:
            connect_security_mode = SL_WIFI_OPEN;
            break;
        default:
            SILABS_LOG("%s: error: unknown security type.");
            return;
        }
#endif

        SILABS_LOG("%s: WLAN: connecting to %s==%s, sec=%d", __func__, &wfx_rsi.sec.ssid[0], &wfx_rsi.sec.passkey[0],
                    wfx_rsi.sec.security);

       SILABS_LOG("%s: in joinnnnnnnnnnnnnnnnnnnnnnnn", __func__);
    int32_t status;

    if (wfx_rsi.dev_state & (WFX_RSI_ST_STA_CONNECTING | WFX_RSI_ST_STA_CONNECTED))
    {
        SILABS_LOG("%s: not joining - already in progress", __func__);
    }
    else
    {
        SILABS_LOG("%s: WLAN: connecting to %s==%s, sec=%d", __func__, &wfx_rsi.sec.ssid[0], &wfx_rsi.sec.passkey[0],
                    wfx_rsi.sec.security);

        /*
         * Join the network
         */
        /* TODO - make the WFX_SECURITY_xxx - same as RSI_xxx
         * Right now it's done by hand - we need something better
         */
        wfx_rsi.dev_state |= WFX_RSI_ST_STA_CONNECTING;

        sl_wifi_set_join_callback(join_callback_handler, NULL);
        SILABS_LOG("sl_wifi_set_join_callback  afetr");
/* commented wiseconnect*/
//        if ((status = rsi_wlan_register_callbacks(RSI_JOIN_FAIL_CB, wfx_rsi_join_fail_cb)) != RSI_SUCCESS)
//        {
//            SILABS_LOG("%s: RSI callback register join failed with status: %02x", __func__, status);
//        }

        /* Try to connect Wifi with given Credentials
         * untill there is a success or maximum number of tries allowed
         */

            /* Call rsi connect call with given ssid and password
             * And check there is a success
             */
            sl_wifi_credential_t cred                           = { 0 };
            cred.type = SL_WIFI_CRED_PSK;
            memcpy(cred.psk.value, &wfx_rsi.sec.passkey[0], strlen(wfx_rsi.sec.passkey));
            sl_wifi_credential_id_t id        = SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID;
            SILABS_LOG("sl_net_set_credential  before");
            status = sl_net_set_credential(id, SL_NET_WIFI_PSK, &wfx_rsi.sec.passkey[0], strlen(wfx_rsi.sec.passkey));
            if (SL_STATUS_OK != status)
            {
                return status;
            }

           sl_wifi_client_configuration_t ap = { 0 };
           uint32_t timeout_ms               = 0;

           ap.ssid.length                    = strlen(wfx_rsi.sec.ssid);
           memcpy(ap.ssid.value, (int8_t *) &wfx_rsi.sec.ssid[0], ap.ssid.length);
           ap.security      = 2;//( sl_wifi_security_t) wfx_rsi.sec.security;
           ap.encryption    = SL_WIFI_NO_ENCRYPTION;
           ap.credential_id = id;
            SILABS_LOG("sl_net_set_credential  after %%%%%%%%%%");
//              while (is_wifi_disconnection_event || wfx_rsi.join_retries <= WFX_RSI_CONFIG_MAX_JOIN)
//        {
           if ((status =  sl_wifi_connect(SL_WIFI_CLIENT_INTERFACE, &ap, timeout_ms)) == SL_STATUS_IN_PROGRESS)
           {

                 callback_status = SL_STATUS_IN_PROGRESS;
                 while (callback_status == SL_STATUS_IN_PROGRESS)
                 {
                    osThreadYield();
                 }
                 status = callback_status;

         //     SILABS_LOG("sl_wifi_connect  before");
       //    wfx_rsi.dev_state &= ~WFX_RSI_ST_STA_CONNECTING;
     //      wfx_retry_interval_handler(is_wifi_disconnection_event, wfx_rsi.join_retries);
    //       wfx_rsi.join_retries++;
/*
            if ((status = rsi_wlan_connect_async((int8_t *) &wfx_rsi.sec.ssid[0], (rsi_security_mode_t) wfx_rsi.sec.security,
                                                 &wfx_rsi.sec.passkey[0], wfx_rsi_join_cb)) != RSI_SUCCESS)
            {

                wfx_rsi.dev_state &= ~WFX_RSI_ST_STA_CONNECTING;
                SILABS_LOG("%s: rsi_wlan_connect_async failed with status: %02x on try %d", __func__, status,
                            wfx_rsi.join_retries);

                wfx_retry_interval_handler(is_wifi_disconnection_event, wfx_rsi.join_retries);
                wfx_rsi.join_retries++;*/
         }
         else
         {
                SILABS_LOG("%s: starting JOIN to %s after %d tries\n", __func__, (char *) &wfx_rsi.sec.ssid[0],
                            wfx_rsi.join_retries);
               // break; // exit while loop
            }
        //}
    }
}
}

/*********************************************************************************
 * @fn  void wfx_rsi_task(void *arg)
 * @brief
 * The main WLAN task - started by wfx_wifi_start () that interfaces with RSI.
 * The rest of RSI stuff come in call-backs.
 * The initialization has been already done.
 * @param[in] arg:
 * @return
 *       None
 **********************************************************************************/
/* ARGSUSED */
void wfx_rsi_task(void * arg)
{
    EventBits_t flags;
#ifndef RS911X_SOCKETS
    TickType_t last_dhcp_poll, now;
    struct netif * sta_netif;
#endif
    (void) arg;
    uint32_t rsi_status = wfx_rsi_init();
    if (rsi_status != RSI_SUCCESS)
    {
        SILABS_LOG("%s: error: wfx_rsi_init with status: %02x", __func__, rsi_status);
        return;
    }

#ifndef RS911X_SOCKETS
    wfx_lwip_start();
    last_dhcp_poll = xTaskGetTickCount();
    sta_netif      = wfx_get_netif(SL_WFX_STA_INTERFACE);
#endif
    wfx_started_notify();

    SILABS_LOG("%s: starting event wait", __func__);
    for (;;)
    {
        /*
         * This is the main job of this task.
         * Wait for commands from the ConnectivityManager
         * Make state changes (based on call backs)
         */
        flags = xEventGroupWaitBits(wfx_rsi.events,
                                    WFX_EVT_STA_CONN | WFX_EVT_STA_DISCONN | WFX_EVT_STA_START_JOIN
#ifdef RS911X_SOCKETS
                                        | WFX_EVT_STA_DO_DHCP | WFX_EVT_STA_DHCP_DONE
#endif /* RS911X_SOCKETS */
#ifdef SL_WFX_CONFIG_SOFTAP
                                        | WFX_EVT_AP_START | WFX_EVT_AP_STOP
#endif /* SL_WFX_CONFIG_SOFTAP */
#ifdef SL_WFX_CONFIG_SCAN
                                        | WFX_EVT_SCAN
#endif /* SL_WFX_CONFIG_SCAN */
                                        | 0,
                                    pdTRUE,              /* Clear the bits */
                                    pdFALSE,             /* Wait for any bit */
                                    pdMS_TO_TICKS(250)); /* 250 mSec */

        if (flags)
        {
            SILABS_LOG("%s: wait event encountered: %x", __func__, flags);
        }
#ifdef RS911X_SOCKETS
        if (flags & WFX_EVT_STA_DO_DHCP)
        {
            /*
             * Do DHCP -
             */
            if ((status = rsi_config_ipaddress(RSI_IP_VERSION_4, RSI_DHCP | RSI_DHCP_UNICAST_OFFER, NULL, NULL, NULL,
                                               &wfx_rsi.ip4_addr[0], IP_CONF_RSP_BUFF_LENGTH_4, STATION)) != RSI_SUCCESS)
            {
                /* We should try this again.. (perhaps sleep) */
                /* TODO - Figure out what to do here */
            }
        }
#else /* !RS911X_SOCKET - using LWIP */
        /*
         * Let's handle DHCP polling here
         */
        if (wfx_rsi.dev_state & WFX_RSI_ST_STA_CONNECTED)
        {
            if ((now = xTaskGetTickCount()) > (last_dhcp_poll + pdMS_TO_TICKS(250)))
            {
#if (CHIP_DEVICE_CONFIG_ENABLE_IPV4)
                uint8_t dhcp_state = dhcpclient_poll(sta_netif);
                if (dhcp_state == DHCP_ADDRESS_ASSIGNED && !hasNotifiedIPV4)
                {
                    wfx_dhcp_got_ipv4((uint32_t) sta_netif->ip_addr.u_addr.ip4.addr);
                    hasNotifiedIPV4 = true;
#if CHIP_DEVICE_CONFIG_ENABLE_SED
#ifndef RSI_BLE_ENABLE
                    // enabling the power save mode for RS9116 if sleepy device is enabled
                    // if BLE is used on the rs9116 then powersave config is done after ble disconnect event
                    wfx_rsi_power_save();
#endif /* RSI_BLE_ENABLE */
#endif /* CHIP_DEVICE_CONFIG_ENABLE_SED */
                    if (!hasNotifiedWifiConnectivity)
                    {
                        wfx_connected_notify(CONNECTION_STATUS_SUCCESS, &wfx_rsi.ap_mac);
                        hasNotifiedWifiConnectivity = true;
                    }
                }
                else if (dhcp_state == DHCP_OFF)
                {
                    wfx_ip_changed_notify(IP_STATUS_FAIL);
                    hasNotifiedIPV4 = false;
                }
#endif /* CHIP_DEVICE_CONFIG_ENABLE_IPV4 */
                /* Checks if the assigned IPv6 address is preferred by evaluating
                 * the first block of IPv6 address ( block 0)
                 */
                if ((ip6_addr_ispreferred(netif_ip6_addr_state(sta_netif, 0))) && !hasNotifiedIPV6)
                {
                    wfx_ipv6_notify(GET_IPV6_SUCCESS);
                    hasNotifiedIPV6 = true;
#if CHIP_DEVICE_CONFIG_ENABLE_SED
#ifndef RSI_BLE_ENABLE
                    // enabling the power save mode for RS9116 if sleepy device is enabled
                    // if BLE is used on the rs9116 then powersave config is done after ble disconnect event
                    wfx_rsi_power_save();
#endif /* RSI_BLE_ENABLE */
#endif /* CHIP_DEVICE_CONFIG_ENABLE_SED */
                    if (!hasNotifiedWifiConnectivity)
                    {
                        wfx_connected_notify(CONNECTION_STATUS_SUCCESS, &wfx_rsi.ap_mac);
                        hasNotifiedWifiConnectivity = true;
                    }
                }
                last_dhcp_poll = now;
            }
        }
#endif /* RS911X_SOCKETS */
        if (flags & WFX_EVT_STA_START_JOIN)
        {
            // saving the AP related info
            wfx_rsi_save_ap_info();
            // Joining to the network
            wfx_rsi_do_join();
        }
        if (flags & WFX_EVT_STA_CONN)
        {
        #if 0
        uint32_t sampledata[100];
         uint32_t count =0;
/* make buffer point to useful data, and then: */
    for (i=0; i < 100; ++i)
    sampledata[i] = i*10;
    for(count=0;count<10;count++)
    {
    /*
    * Initiate the Join command (assuming we have been provisioned)
    */
        sl_wifi_buffer_t *buffer;
        sl_si91x_packet_t *packet;
        sl_status_t status = SL_STATUS_OK;
        uint32_t data_length = 100;
        /* Confirm if packet is allocated */

        status = sl_si91x_allocate_command_buffer(&buffer,
                                            (void **)&packet,
                                            sizeof(sl_si91x_packet_t) + 100,
                                            SL_WIFI_ALLOCATE_COMMAND_BUFFER_WAIT_TIME);
//        VERIFY_STATUS_AND_RETURN(status);
        if (packet == NULL) {
           SILABS_LOG("EN-RSI:No buf");
           return SL_STATUS_ALLOCATION_FAILED;
        }
        memset(packet->desc, 0, sizeof(packet->desc));
        if (sampledata != NULL) {
           memcpy(packet->data, sampledata, data_length);
        }

        // Fill frame type
        packet->length  = data_length & 0xFFF;
        packet->command =  0x1;
        if (sl_si91x_driver_send_data_packet(SI91X_WLAN_CMD_QUEUE,buffer, 1000))
        {
            SILABS_LOG("Wifi send fail");
            return ;
        }
    }
#endif
            SILABS_LOG("%s: starting LwIP STA", __func__);
            wfx_rsi.dev_state |= WFX_RSI_ST_STA_CONNECTED;
#ifndef RS911X_SOCKETS
            hasNotifiedWifiConnectivity = false;
#if (CHIP_DEVICE_CONFIG_ENABLE_IPV4)
            hasNotifiedIPV4 = false;
#endif // CHIP_DEVICE_CONFIG_ENABLE_IPV4
            hasNotifiedIPV6 = false;
            wfx_lwip_set_sta_link_up();
#endif /* !RS911X_SOCKETS */
            /* We need to get AP Mac - TODO */
            // Uncomment once the hook into MATTER is moved to IP connectivty instead
            // of AP connectivity. wfx_connected_notify(0, &wfx_rsi.ap_mac); // This
            // is independant of IP connectivity.
        }
        if (flags & WFX_EVT_STA_DISCONN)
        {
            wfx_rsi.dev_state &=
                ~(WFX_RSI_ST_STA_READY | WFX_RSI_ST_STA_CONNECTING | WFX_RSI_ST_STA_CONNECTED | WFX_RSI_ST_STA_DHCP_DONE);
            SILABS_LOG("%s: disconnect notify", __func__);
            /* TODO: Implement disconnect notify */
#ifndef RS911X_SOCKETS
            wfx_lwip_set_sta_link_down(); // Internally dhcpclient_poll(netif) ->
                                          // wfx_ip_changed_notify(0) for IPV4
#if (CHIP_DEVICE_CONFIG_ENABLE_IPV4)
            wfx_ip_changed_notify(IP_STATUS_FAIL);
            hasNotifiedIPV4 = false;
#endif /* CHIP_DEVICE_CONFIG_ENABLE_IPV4 */
            wfx_ipv6_notify(GET_IPV6_FAIL);
            hasNotifiedIPV6             = false;
            hasNotifiedWifiConnectivity = false;
#endif /* !RS911X_SOCKETS */
        }
#ifdef SL_WFX_CONFIG_SCAN
        if (flags & WFX_EVT_SCAN)
        {
            if (!(wfx_rsi.dev_state & WFX_RSI_ST_SCANSTARTED))
            {
                SILABS_LOG("%s: start SSID scan", __func__);
                int x;
                wfx_wifi_scan_result_t ap;
#if 0
//wifi-sdk
                rsi_scan_info_t * scan;
                int32_t status;
                uint8_t bgscan_results[BG_SCAN_RES_SIZE] = { 0 };
                status = rsi_wlan_bgscan_profile(1, (rsi_rsp_scan_t *) bgscan_results, BG_SCAN_RES_SIZE);

                SILABS_LOG("%s: status: %02x size = %d", __func__, status, BG_SCAN_RES_SIZE);
                rsi_rsp_scan_t * rsp = (rsi_rsp_scan_t *) bgscan_results;
                if (status)
                {
                    /*
                     * Scan is done - failed
                     */
                }
                else
                    for (x = 0; x < rsp->scan_count[0]; x++)
                    {
                        scan = &rsp->scan_info[x];
                        strcpy(&ap.ssid[0], (char *) &scan->ssid[0]);
                        if (wfx_rsi.scan_ssid)
                        {
                            SILABS_LOG("Inside scan_ssid");
                            SILABS_LOG("SCAN SSID: %s , ap scan: %s", wfx_rsi.scan_ssid, ap.ssid);
                            if (strcmp(wfx_rsi.scan_ssid, ap.ssid) == CMP_SUCCESS)
                            {
                                SILABS_LOG("Inside ap details");
                                ap.security = scan->security_mode;
                                ap.rssi     = (-1) * scan->rssi_val;
                                memcpy(&ap.bssid[0], &scan->bssid[0], BSSID_MAX_STR_LEN);
                                (*wfx_rsi.scan_cb)(&ap);
                            }
                        }
                        else
                        {
                            SILABS_LOG("Inside else");
                            ap.security = scan->security_mode;
                            ap.rssi     = (-1) * scan->rssi_val;
                            ap.chan     = scan->rf_channel;
                            memcpy(&ap.bssid[0], &scan->bssid[0], BSSID_MAX_STR_LEN);
                            (*wfx_rsi.scan_cb)(&ap);
                        }
                    }
                wfx_rsi.dev_state &= ~WFX_RSI_ST_SCANSTARTED;
                /* Terminate with end of scan which is no ap sent back */
                (*wfx_rsi.scan_cb)((wfx_wifi_scan_result_t *) 0);
                wfx_rsi.scan_cb = (void (*)(wfx_wifi_scan_result_t *)) 0;

                if (wfx_rsi.scan_ssid)
                {
                    vPortFree(wfx_rsi.scan_ssid);
                    wfx_rsi.scan_ssid = (char *) 0;
                }
#endif
            }
        }
#endif /* SL_WFX_CONFIG_SCAN */
#ifdef SL_WFX_CONFIG_SOFTAP
        /* TODO */
        if (flags & WFX_EVT_AP_START)
        {
        }
        if (flags & WFX_EVT_AP_STOP)
        {
        }
#endif /* SL_WFX_CONFIG_SOFTAP */
    }
}

#if CHIP_DEVICE_CONFIG_ENABLE_IPV4
/********************************************************************************************
 * @fn   void wfx_dhcp_got_ipv4(uint32_t ip)
 * @brief
 *        Acquire the new ip address
 * @param[in] ip: internet protocol
 * @return
 *        None
 **********************************************************************************************/
void wfx_dhcp_got_ipv4(uint32_t ip)
{
    /*
     * Acquire the new IP address
     */
    wfx_rsi.ip4_addr[0] = (ip) &HEX_VALUE_FF;
    wfx_rsi.ip4_addr[1] = (ip >> 8) & HEX_VALUE_FF;
    wfx_rsi.ip4_addr[2] = (ip >> 16) & HEX_VALUE_FF;
    wfx_rsi.ip4_addr[3] = (ip >> 24) & HEX_VALUE_FF;
    SILABS_LOG("%s: DHCP OK: IP=%d.%d.%d.%d", __func__, wfx_rsi.ip4_addr[0], wfx_rsi.ip4_addr[1], wfx_rsi.ip4_addr[2],
                wfx_rsi.ip4_addr[3]);
    /* Notify the Connectivity Manager - via the app */
    wfx_rsi.dev_state |= WFX_RSI_ST_STA_DHCP_DONE;
    wfx_ip_changed_notify(IP_STATUS_SUCCESS);
    wfx_rsi.dev_state |= WFX_RSI_ST_STA_READY;
}
#endif /* CHIP_DEVICE_CONFIG_ENABLE_IPV4 */

/*
 * WARNING - Taken from RSI and broken up
 * This is my own RSI stuff for not copying code and allocating an extra
 * level of indirection - when using LWIP buffers
 * see also: int32_t rsi_wlan_send_data_xx(uint8_t *buffer, uint32_t length)
 */
/********************************************************************************************
 * @fn   void *wfx_rsi_alloc_pkt()
 * @brief
 *       Allocate packet to send data
 * @param[in] None
 * @return
 *        None
 **********************************************************************************************/
void * wfx_rsi_alloc_pkt(uint16_t data_length)
{
   sl_wifi_buffer_t *buffer;
   sl_si91x_packet_t *packet;
   sl_status_t status = SL_STATUS_OK;

   /* Confirm if packet is allocated */

    status = sl_si91x_allocate_command_buffer(&buffer,
                                            (void **)&packet,
                                            sizeof(sl_si91x_packet_t) + data_length,
                                            SL_WIFI_ALLOCATE_COMMAND_BUFFER_WAIT_TIME);
//    VERIFY_STATUS_AND_RETURN(status);
    if (packet == NULL) {
      return SL_STATUS_ALLOCATION_FAILED;
    }
    return (void *) packet;
}

/********************************************************************************************
 * @fn   void wfx_rsi_pkt_add_data(void *p, uint8_t *buf, uint16_t len, uint16_t off)
 * @brief
 *       add the data into packet
 * @param[in]  p:
 * @param[in]  buf:
 * @param[in]  len:
 * @param[in]  off:
 * @return
 *        None
 **********************************************************************************************/
void wfx_rsi_pkt_add_data(void * p, uint8_t * buf, uint16_t len, uint16_t off)
{
/* wifi-sdk
    rsi_pkt_t * pkt;

    pkt = (rsi_pkt_t *) p;
    memcpy(((char *) pkt->data) + off, buf, len);
*/
    sl_si91x_packet_t * pkt;
    pkt = (sl_si91x_packet_t *) p;
    memcpy(((char *) pkt->data) + off, buf, len);
}

/********************************************************************************************
 * @fn   int32_t wfx_rsi_send_data(void *p, uint16_t len)
 * @brief
 *       Driver send a data
 * @param[in]  p:
 * @param[in]  len:
 * @return
 *        None
 **********************************************************************************************/
int32_t wfx_rsi_send_data(void * p, uint16_t len)
{
  int32_t status;
  sl_wifi_buffer_t *buffer;
  buffer = (sl_wifi_buffer_t *) p;
  // Fill frame type
 //packet->length  = data_length & 0xFFF;
 // packet->command = RSI_SEND_RAW_DATA;

  if (sl_si91x_driver_send_data_packet(SI91X_WLAN_CMD_QUEUE,buffer, RSI_SEND_RAW_DATA_RESPONSE_WAIT_TIME))
  {
      SILABS_LOG("*ERR*EN-RSI:Send fail");
      return ERR_IF;
  }
  return status;
}

