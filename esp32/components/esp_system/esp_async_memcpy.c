// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "esp_osal/esp_osal.h"
#include "esp_osal/semphr.h"
#include "hal/dma_types.h"
#include "esp_compiler.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_async_memcpy.h"
#include "esp_async_memcpy_impl.h"

static const char *TAG = "async_memcpy";

#define ASMCP_CHECK(a, msg, tag, ret, ...)                                        \
    do                                                                            \
    {                                                                             \
        if (unlikely(!(a)))                                                       \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " msg, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret_code = ret;                                                       \
            goto tag;                                                             \
        }                                                                         \
    } while (0)

/**
 * @brief Type of async mcp stream
 *        mcp stream inherits DMA descriptor, besides that, it has a callback function member
 */
typedef struct {
    dma_descriptor_t desc;
    async_memcpy_isr_cb_t cb;
    void *cb_args;
} async_memcpy_stream_t;

/**
 * @brief Type of async mcp driver context
 */
typedef struct async_memcpy_context_t {
    async_memcpy_impl_t mcp_impl; // implementation layer
    portMUX_TYPE spinlock;     // spinlock, prevent operating descriptors concurrently
    intr_handle_t intr_hdl;    // interrupt handle
    uint32_t flags;            // extra driver flags
    dma_descriptor_t *tx_desc; // pointer to the next free TX descriptor
    dma_descriptor_t *rx_desc; // pointer to the next free RX descriptor
    dma_descriptor_t *next_rx_desc_to_check; // pointer to the next RX descriptor to recycle
    uint32_t max_stream_num;            // maximum number of streams
    async_memcpy_stream_t *out_streams;    // pointer to the first TX stream
    async_memcpy_stream_t *in_streams;     // pointer to the first RX stream
    async_memcpy_stream_t streams_pool[0]; // stream pool (TX + RX), the size is configured during driver installation
} async_memcpy_context_t;

esp_err_t esp_async_memcpy_install(const async_memcpy_config_t *config, async_memcpy_t *asmcp)
{
    esp_err_t ret_code = ESP_OK;
    async_memcpy_context_t *mcp_hdl = NULL;

    ASMCP_CHECK(config, "configuration can't be null", err, ESP_ERR_INVALID_ARG);
    ASMCP_CHECK(asmcp, "can't assign mcp handle to null", err, ESP_ERR_INVALID_ARG);

    // context memory size + stream pool size
    size_t total_malloc_size = sizeof(async_memcpy_context_t) + sizeof(async_memcpy_stream_t) * config->backlog * 2;
    // to work when cache is disabled, the driver handle should located in SRAM
    mcp_hdl = heap_caps_calloc(1, total_malloc_size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ASMCP_CHECK(mcp_hdl, "allocate context memory failed", err, ESP_ERR_NO_MEM);

    mcp_hdl->flags = config->flags;
    mcp_hdl->out_streams = mcp_hdl->streams_pool;
    mcp_hdl->in_streams = mcp_hdl->streams_pool + config->backlog;
    mcp_hdl->max_stream_num = config->backlog;

    // circle TX/RX descriptors
    for (size_t i = 0; i < mcp_hdl->max_stream_num; i++) {
        mcp_hdl->out_streams[i].desc.dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
        mcp_hdl->out_streams[i].desc.next = &mcp_hdl->out_streams[i + 1].desc;
        mcp_hdl->in_streams[i].desc.dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
        mcp_hdl->in_streams[i].desc.next = &mcp_hdl->in_streams[i + 1].desc;
    }
    mcp_hdl->out_streams[mcp_hdl->max_stream_num - 1].desc.next = &mcp_hdl->out_streams[0].desc;
    mcp_hdl->in_streams[mcp_hdl->max_stream_num - 1].desc.next = &mcp_hdl->in_streams[0].desc;

    mcp_hdl->tx_desc = &mcp_hdl->out_streams[0].desc;
    mcp_hdl->rx_desc = &mcp_hdl->in_streams[0].desc;
    mcp_hdl->next_rx_desc_to_check = &mcp_hdl->in_streams[0].desc;
    mcp_hdl->spinlock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    // initialize implementation layer
    async_memcpy_impl_init(&mcp_hdl->mcp_impl);

    *asmcp = mcp_hdl;

    async_memcpy_impl_start(&mcp_hdl->mcp_impl, (intptr_t)&mcp_hdl->out_streams[0].desc, (intptr_t)&mcp_hdl->in_streams[0].desc);

    return ESP_OK;
err:
    if (mcp_hdl) {
        free(mcp_hdl);
    }
    if (asmcp) {
        *asmcp = NULL;
    }
    return ret_code;
}

esp_err_t esp_async_memcpy_uninstall(async_memcpy_t asmcp)
{
    esp_err_t ret_code = ESP_OK;
    ASMCP_CHECK(asmcp, "mcp handle can't be null", err, ESP_ERR_INVALID_ARG);

    async_memcpy_impl_stop(&asmcp->mcp_impl);
    async_memcpy_impl_deinit(&asmcp->mcp_impl);
    free(asmcp);
    return ESP_OK;
err:
    return ret_code;
}

static int async_memcpy_prepare_receive(async_memcpy_t asmcp, void *buffer, size_t size, dma_descriptor_t **start_desc, dma_descriptor_t **end_desc)
{
    uint32_t prepared_length = 0;
    uint8_t *buf = (uint8_t *)buffer;
    dma_descriptor_t *desc = asmcp->rx_desc; // descriptor iterator
    dma_descriptor_t *start = desc;
    dma_descriptor_t *end = desc;

    while (size > DMA_DESCRIPTOR_BUFFER_MAX_SIZE) {
        if (desc->dw0.owner != DMA_DESCRIPTOR_BUFFER_OWNER_DMA) {
            desc->dw0.suc_eof = 0;
            desc->dw0.size = DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
            desc->buffer = &buf[prepared_length];
            desc = desc->next; // move to next descriptor
            prepared_length += DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
            size -= DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
        } else {
            // out of RX descriptors
            goto _exit;
        }
    }
    if (size) {
        if (desc->dw0.owner != DMA_DESCRIPTOR_BUFFER_OWNER_DMA) {
            end = desc; // the last descriptor used
            desc->dw0.suc_eof = 0;
            desc->dw0.size = size;
            desc->buffer = &buf[prepared_length];
            desc = desc->next; // move to next descriptor
            prepared_length += size;
        } else {
            // out of RX descriptors
            goto _exit;
        }
    }

_exit:
    *start_desc = start;
    *end_desc = end;
    return prepared_length;
}

static int async_memcpy_prepare_transmit(async_memcpy_t asmcp, void *buffer, size_t len, dma_descriptor_t **start_desc, dma_descriptor_t **end_desc)
{
    uint32_t prepared_length = 0;
    uint8_t *buf = (uint8_t *)buffer;
    dma_descriptor_t *desc = asmcp->tx_desc; // descriptor iterator
    dma_descriptor_t *start = desc;
    dma_descriptor_t *end = desc;

    while (len > DMA_DESCRIPTOR_BUFFER_MAX_SIZE) {
        if (desc->dw0.owner != DMA_DESCRIPTOR_BUFFER_OWNER_DMA) {
            desc->dw0.suc_eof = 0; // not the end of the transaction
            desc->dw0.size = DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
            desc->dw0.length = DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
            desc->buffer = &buf[prepared_length];
            desc = desc->next; // move to next descriptor
            prepared_length += DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
            len -= DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
        } else {
            // out of TX descriptors
            goto _exit;
        }
    }
    if (len) {
        if (desc->dw0.owner != DMA_DESCRIPTOR_BUFFER_OWNER_DMA) {
            end = desc;            // the last descriptor used
            desc->dw0.suc_eof = 1; // end of the transaction
            desc->dw0.size = len;
            desc->dw0.length = len;
            desc->buffer = &buf[prepared_length];
            desc = desc->next; // move to next descriptor
            prepared_length += len;
        } else {
            // out of TX descriptors
            goto _exit;
        }
    }

    *start_desc = start;
    *end_desc = end;
_exit:
    return prepared_length;
}

static bool async_memcpy_get_next_rx_descriptor(async_memcpy_t asmcp, dma_descriptor_t *eof_desc, dma_descriptor_t **next_desc)
{
    dma_descriptor_t *next = asmcp->next_rx_desc_to_check;
    // additional check, to avoid potential interrupt got triggered by mistake
    if (next->dw0.owner == DMA_DESCRIPTOR_BUFFER_OWNER_CPU) {
        asmcp->next_rx_desc_to_check = asmcp->next_rx_desc_to_check->next;
        *next_desc = next;
        // return if we need to continue
        return eof_desc == next ? false : true;
    }

    *next_desc = NULL;
    return false;
}

esp_err_t esp_async_memcpy(async_memcpy_t asmcp, void *dst, void *src, size_t n, async_memcpy_isr_cb_t cb_isr, void *cb_args)
{
    esp_err_t ret_code = ESP_OK;
    dma_descriptor_t *rx_start_desc = NULL;
    dma_descriptor_t *rx_end_desc = NULL;
    dma_descriptor_t *tx_start_desc = NULL;
    dma_descriptor_t *tx_end_desc = NULL;
    size_t rx_prepared_size = 0;
    size_t tx_prepared_size = 0;
    ASMCP_CHECK(asmcp, "mcp handle can't be null", err, ESP_ERR_INVALID_ARG);
    ASMCP_CHECK(async_memcpy_impl_is_buffer_address_valid(&asmcp->mcp_impl, src, dst), "buffer address not valid", err, ESP_ERR_INVALID_ARG);
    ASMCP_CHECK(n <= DMA_DESCRIPTOR_BUFFER_MAX_SIZE * asmcp->max_stream_num, "buffer size too large", err, ESP_ERR_INVALID_ARG);

    // Prepare TX and RX descriptor
    portENTER_CRITICAL_SAFE(&asmcp->spinlock);
    rx_prepared_size = async_memcpy_prepare_receive(asmcp, dst, n, &rx_start_desc, &rx_end_desc);
    tx_prepared_size = async_memcpy_prepare_transmit(asmcp, src, n, &tx_start_desc, &tx_end_desc);
    if ((rx_prepared_size == n) && (tx_prepared_size == n)) {
        // register user callback to the last descriptor
        async_memcpy_stream_t *mcp_stream = __containerof(rx_end_desc, async_memcpy_stream_t, desc);
        mcp_stream->cb = cb_isr;
        mcp_stream->cb_args = cb_args;
        // restart RX firstly
        dma_descriptor_t *desc = rx_start_desc;
        while (desc != rx_end_desc) {
            desc->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
            desc = desc->next;
        }
        desc->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
        asmcp->rx_desc = desc->next;
        // restart TX secondly
        desc = tx_start_desc;
        while (desc != tx_end_desc) {
            desc->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
            desc = desc->next;
        }
        desc->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
        asmcp->tx_desc = desc->next;
        async_memcpy_impl_restart(&asmcp->mcp_impl);
    }
    portEXIT_CRITICAL_SAFE(&asmcp->spinlock);

    // It's unlikely that we have space for rx descriptor but no space for tx descriptor
    // Both tx and rx descriptor should move in the same pace
    ASMCP_CHECK(rx_prepared_size == n, "out of rx descriptor", err, ESP_FAIL);
    ASMCP_CHECK(tx_prepared_size == n, "out of tx descriptor", err, ESP_FAIL);

    return ESP_OK;
err:
    return ret_code;
}

IRAM_ATTR void async_memcpy_isr_on_rx_done_event(async_memcpy_impl_t *impl)
{
    bool to_continue = false;
    async_memcpy_stream_t *in_stream = NULL;
    dma_descriptor_t *next_desc = NULL;
    async_memcpy_context_t *asmcp = __containerof(impl, async_memcpy_context_t, mcp_impl);

    // get the RX eof descriptor address
    dma_descriptor_t *eof = (dma_descriptor_t *)impl->rx_eof_addr;
    // traversal all unchecked descriptors
    do {
        portENTER_CRITICAL_ISR(&asmcp->spinlock);
        // There is an assumption that the usage of rx descriptors are in the same pace as tx descriptors (this is determined by M2M DMA working mechanism)
        // And once the rx descriptor is recycled, the corresponding tx desc is guaranteed to be returned by DMA
        to_continue = async_memcpy_get_next_rx_descriptor(asmcp, eof, &next_desc);
        portEXIT_CRITICAL_ISR(&asmcp->spinlock);
        if (next_desc) {
            in_stream = __containerof(next_desc, async_memcpy_stream_t, desc);
            // invoke user registered callback if available
            if (in_stream->cb) {
                async_memcpy_event_t e = {0};
                if (in_stream->cb(asmcp, &e, in_stream->cb_args)) {
                    impl->isr_need_yield = true;
                }
                in_stream->cb = NULL;
                in_stream->cb_args = NULL;
            }
        }
    } while (to_continue);
}
