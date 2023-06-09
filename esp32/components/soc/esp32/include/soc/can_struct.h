// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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
#ifndef __CAN_STRUCT_H__
#define __CAN_STRUCT_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#warning soc/can_struct.h is deprecated, please use soc/twai_struct.h instead

#include "soc/twai_struct.h"

typedef twai_dev_t can_dev_t;
extern can_dev_t CAN;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __CAN_STRUCT_H__ */
