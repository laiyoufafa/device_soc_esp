// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
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

#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <reent.h>
#include <sys/fcntl.h>
#include "sdkconfig.h"
#include "esp_rom_uart.h"

static int syscall_not_implemented(void)
{
    errno = ENOSYS;
    return -1;
}

static int syscall_not_implemented_aborts(void)
{
    abort();
}

ssize_t _write_r_console(struct _reent *r, int fd, const void * data, size_t size)
{
    const char* cdata = (const char*) data;
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        for (size_t i = 0; i < size; ++i) {
            esp_rom_uart_tx_one_char(cdata[i]);
        }
        return size;
    }
    errno = EBADF;
    return -1;
}

ssize_t _read_r_console(struct _reent *r, int fd, void * data, size_t size)
{
    char* cdata = (char*) data;
    if (fd == STDIN_FILENO) {
        size_t received;
        for (received = 0; received < size; ++received) {
            int status = esp_rom_uart_rx_one_char((uint8_t*) &cdata[received]);
            if (status != 0) {
                break;
            }
        }
        return received;
    }
    errno = EBADF;
    return -1;
}


/* The following weak definitions of syscalls will be used unless
 * another definition is provided. That definition may come from
 * VFS, LWIP, or the application.
 */
ssize_t _read_r(struct _reent *r, int fd, void * dst, size_t size)
    __attribute__((weak,alias("_read_r_console")));
ssize_t _write_r(struct _reent *r, int fd, const void * data, size_t size)
    __attribute__((weak,alias("_write_r_console")));


/* The aliases below are to "syscall_not_implemented", which
 * doesn't have the same signature as the original function.
 * Disable type mismatch warnings for this reason.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattribute-alias"


#pragma GCC diagnostic pop


/* No-op function, used to force linking this file,
   instead of the syscalls implementation from libgloss.
 */
void newlib_include_syscalls_impl(void)
{
}
