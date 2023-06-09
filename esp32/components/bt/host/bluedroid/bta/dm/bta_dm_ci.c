/******************************************************************************
 *
 *  Copyright (C) 2003-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This is the API implementation file for the BTA device manager.
 *
 ******************************************************************************/

#include "bta/bta_sys.h"
#include "bta/bta_api.h"
#include "bta_dm_int.h"
#include <string.h>
#include "bta/bta_dm_ci.h"
#include "osi/allocator.h"


#if (BTM_OOB_INCLUDED == 1 && SMP_INCLUDED == 1)
/*******************************************************************************
**
** Function         bta_dm_ci_io_req
**
** Description      This function must be called in response to function
**                  bta_dm_co_io_req(), if *p_oob_data to BTA_OOB_UNKNOWN
**                  by bta_dm_co_io_req().
**
** Returns          void
**
*******************************************************************************/
void bta_dm_ci_io_req(BD_ADDR bd_addr, tBTA_IO_CAP io_cap, tBTA_OOB_DATA oob_data,
                      tBTA_AUTH_REQ auth_req)

{
    tBTA_DM_CI_IO_REQ    *p_msg;

    if ((p_msg = (tBTA_DM_CI_IO_REQ *) osi_malloc(sizeof(tBTA_DM_CI_IO_REQ))) != NULL) {
        p_msg->hdr.event = BTA_DM_CI_IO_REQ_EVT;
        bdcpy(p_msg->bd_addr, bd_addr);
        p_msg->io_cap   = io_cap;
        p_msg->oob_data = oob_data;
        p_msg->auth_req = auth_req;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         bta_dm_ci_rmt_oob
**
** Description      This function must be called in response to function
**                  bta_dm_co_rmt_oob() to provide the OOB data associated
**                  with the remote device.
**
** Returns          void
**
*******************************************************************************/
void bta_dm_ci_rmt_oob(BOOLEAN accept, BD_ADDR bd_addr, BT_OCTET16 c, BT_OCTET16 r)
{
    tBTA_DM_CI_RMT_OOB    *p_msg;

    if ((p_msg = (tBTA_DM_CI_RMT_OOB *) osi_malloc(sizeof(tBTA_DM_CI_RMT_OOB))) != NULL) {
        p_msg->hdr.event = BTA_DM_CI_RMT_OOB_EVT;
        bdcpy(p_msg->bd_addr, bd_addr);
        p_msg->accept    = accept;
        memcpy(p_msg->c, c, BT_OCTET16_LEN);
        memcpy(p_msg->r, r, BT_OCTET16_LEN);
        bta_sys_sendmsg(p_msg);
    }
}
#endif /* BTM_OOB_INCLUDED */
