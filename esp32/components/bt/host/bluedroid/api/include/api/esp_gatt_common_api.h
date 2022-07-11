// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __ESP_GATT_COMMON_API_H__
#define __ESP_GATT_COMMON_API_H__

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum Transmission Unit used in GATT
#define ESP_GATT_DEF_BLE_MTU_SIZE   50   /* relate to GATT_DEF_BLE_MTU_SIZE in stack/gatt_api.h */

// Maximum Transmission Unit allowed in GATT
#define ESP_GATT_MAX_MTU_SIZE       517  /* relate to GATT_MAX_MTU_SIZE in stack/gatt_api.h */

/**
 * @brief           This function is called to set local MTU,
 *                  the function is called before BLE connection.
 *
 * @param[in]       mtu: the size of MTU.
 *
 * @return
 *                  - ESP_OK: success
 *                  - other: failed
 *
 */
extern esp_err_t esp_ble_gatt_set_local_mtu (uint16_t mtu);

#if (BLE_INCLUDED == 1)
extern uint16_t esp_ble_get_sendable_packets_num (void);
extern uint16_t esp_ble_get_cur_sendable_packets_num (uint16_t connid);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ESP_GATT_COMMON_API_H__ */
