/*
 * Copyright (c) 2022 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __OSAL_CONFIG_H__
#define __OSAL_CONFIG_H__

#include "sdkconfig.h"

/* enable use of optimized task selection by the scheduler */
#ifdef CONFIG_FREERTOS_OPTIMIZED_SCHEDULER
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif

/* ESP31 and ESP32 are dualcore processors. */
#ifndef CONFIG_FREERTOS_UNICORE
#define portNUM_PROCESSORS 2
#else
#define portNUM_PROCESSORS 1
#endif

#define XT_USE_THREAD_SAFE_CLIB		0
#define configASSERT_2	            0
#define portUSING_MPU_WRAPPERS      0
#define configUSE_MUTEX             1
#undef XT_USE_SWPRI

#if CONFIG_FREERTOS_CORETIMER_0
#define XT_TIMER_INDEX 0
#elif CONFIG_FREERTOS_CORETIMER_1
#define XT_TIMER_INDEX 1
#endif

#define configNUM_THREAD_LOCAL_STORAGE_POINTERS  CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS
#define configTHREAD_LOCAL_STORAGE_DELETE_CALLBACKS 1

#ifndef __ASSEMBLER__

/**
 * This function is defined to provide a deprecation warning whenever
 * XT_CLOCK_FREQ macro is used.
 * Update the code to use esp_clk_cpu_freq function instead.
 * @return current CPU clock frequency, in Hz
 */
int xt_clock_freq(void) __attribute__((deprecated));

#define XT_CLOCK_FREQ   (xt_clock_freq())

#endif // __ASSEMBLER__


/* Required for configuration-dependent settings */
#include <esp_osal/xtensa_config.h>

/* configASSERT behaviour */
#ifndef __ASSEMBLER__
#include <assert.h>
#include "esp_rom_sys.h"
#if CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/ets_sys.h"  // will be removed in idf v5.0
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/ets_sys.h"
#endif

#if defined(CONFIG_FREERTOS_ASSERT_DISABLE)
#define configASSERT(a) /* assertions disabled */
#elif defined(CONFIG_FREERTOS_ASSERT_FAIL_PRINT_CONTINUE)
#define configASSERT(a) if (unlikely(!(a))) {                               \
        esp_rom_printf("%s:%d (%s)- assert failed!\n", __FILE__, __LINE__,  \
                   __FUNCTION__);                                           \
    }
#elif defined(CONFIG_FREERTOS_ASSERT_FAIL_ABORT)
#define configASSERT(a) assert(a)
#endif

#if CONFIG_FREERTOS_ASSERT_ON_UNTESTED_FUNCTION
#define UNTESTED_FUNCTION() { esp_rom_printf("Untested FreeRTOS function %s\r\n", __FUNCTION__); configASSERT(false); } while(0)
#else
#define UNTESTED_FUNCTION()
#endif


#endif /* def __ASSEMBLER__ */


/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * Note that the default heap size is deliberately kept small so that
 * the build is more likely to succeed for configurations with limited
 * memory.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION			0
#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				0
#define configRECORD_STACK_HIGH_ADDRESS 1
#define configTICK_RATE_HZ				( CONFIG_FREERTOS_HZ )

/* Default clock rate for simulator */
//#define configCPU_CLOCK_HZ				80000000

/* This has impact on speed of search for highest priority */
#ifdef SMALL_TEST
#define configMAX_PRIORITIES			( 7 )
#else
#define configMAX_PRIORITIES			( 25 )
#endif

/* Various things that impact minimum stack sizes */

/* Higher stack checker modes cause overhead on each function call */
#if CONFIG_STACK_CHECK_ALL || CONFIG_STACK_CHECK_STRONG
#define configSTACK_OVERHEAD_CHECKER 256
#else
#define configSTACK_OVERHEAD_CHECKER 0
#endif

/* with optimizations disabled, scheduler uses additional stack */
#if CONFIG_COMPILER_OPTIMIZATION_NONE
#define configSTACK_OVERHEAD_OPTIMIZATION 320
#else
#define configSTACK_OVERHEAD_OPTIMIZATION 0
#endif

/* apptrace mdule increases minimum stack usage */
#if CONFIG_APPTRACE_ENABLE
#define configSTACK_OVERHEAD_APPTRACE 1280
#else
#define configSTACK_OVERHEAD_APPTRACE 0
#endif

#define configSTACK_OVERHEAD_TOTAL (                                    \
                                    configSTACK_OVERHEAD_CHECKER +      \
                                    configSTACK_OVERHEAD_OPTIMIZATION + \
                                    configSTACK_OVERHEAD_APPTRACE       \
                                                                        )

#define configMINIMAL_STACK_SIZE  (768 + configSTACK_OVERHEAD_TOTAL)

#ifndef configIDLE_TASK_STACK_SIZE
#define configIDLE_TASK_STACK_SIZE CONFIG_FREERTOS_IDLE_TASK_STACKSIZE
#endif

/* Stack alignment, architecture specifc. Must be a power of two. */
#define configSTACK_ALIGNMENT			16

/* The Xtensa port uses a separate interrupt stack. Adjust the stack size
 * to suit the needs of your specific application.
 * Size needs to be aligned to the stack increment, since the location of
 * the stack for the 2nd CPU will be calculated using configISR_STACK_SIZE.
 */
#ifndef configISR_STACK_SIZE
#define configISR_STACK_SIZE			((CONFIG_FREERTOS_ISR_STACKSIZE + configSTACK_ALIGNMENT - 1) & (~(configSTACK_ALIGNMENT - 1)))
#endif

/* Minimal heap size to make sure examples can run on memory limited
   configs. Adjust this to suit your system. */


//We define the heap to span all of the non-statically-allocated shared RAM. ToDo: Make sure there
//is some space left for the app and main cpu when running outside of a thread.
#define configAPPLICATION_ALLOCATED_HEAP 1
#define configTOTAL_HEAP_SIZE			(&_heap_end - &_heap_start)//( ( size_t ) (64 * 1024) )

#define configMAX_TASK_NAME_LEN			( CONFIG_FREERTOS_MAX_TASK_NAME_LEN )

#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY        0       /* Used by uxTaskGetSystemState(), and other trace facility functions */
#endif

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define configUSE_STATS_FORMATTING_FUNCTIONS    1   /* Used by vTaskList() */
#endif

#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
#define configTASKLIST_INCLUDE_COREID   1
#endif

#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
#define configGENERATE_RUN_TIME_STATS   1       /* Used by vTaskGetRunTimeStats() */
#endif

#define configUSE_TRACE_FACILITY_2      0		/* Provided by Xtensa port patch */
#define configBENCHMARK					0		/* Provided by Xtensa port patch */
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			0
#define configQUEUE_REGISTRY_SIZE		CONFIG_FREERTOS_QUEUE_REGISTRY_SIZE

#define configUSE_MUTEXES				1
#define configUSE_RECURSIVE_MUTEXES		1
#define configUSE_COUNTING_SEMAPHORES	1

#if CONFIG_FREERTOS_CHECK_STACKOVERFLOW_NONE
#define configCHECK_FOR_STACK_OVERFLOW	0
#elif CONFIG_FREERTOS_CHECK_STACKOVERFLOW_PTRVAL
#define configCHECK_FOR_STACK_OVERFLOW	1
#elif CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY
#define configCHECK_FOR_STACK_OVERFLOW	2
#endif


/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 			0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
   to exclude the API function. */

#define INCLUDE_vTaskPrioritySet			0
#define INCLUDE_uxTaskPriorityGet			0
#define INCLUDE_vTaskDelete					1
#define INCLUDE_vTaskCleanUpResources		0
#define INCLUDE_vTaskSuspend				0
#define INCLUDE_vTaskDelayUntil				0
#define INCLUDE_vTaskDelay					1
#define INCLUDE_uxTaskGetStackHighWaterMark	0
#define INCLUDE_pcTaskGetTaskName			0
#define INCLUDE_xTaskGetIdleTaskHandle      0
#define INCLUDE_pxTaskGetStackStart			0

#define INCLUDE_xSemaphoreGetMutexHolder    0

/* The priority at which the tick interrupt runs.  This should probably be
   kept at 1. */
#define configKERNEL_INTERRUPT_PRIORITY		1

/* The maximum interrupt priority from which FreeRTOS.org API functions can
   be called.  Only API functions that end in ...FromISR() can be used within
   interrupts. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY	XCHAL_EXCM_LEVEL

#define configUSE_NEWLIB_REENTRANT		0

#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configSUPPORT_STATIC_ALLOCATION     1

#ifndef __ASSEMBLER__
#if CONFIG_FREERTOS_ENABLE_STATIC_TASK_CLEAN_UP
extern void vPortCleanUpTCB ( void *pxTCB );
#define portCLEAN_UP_TCB( pxTCB )           vPortCleanUpTCB( pxTCB )
#endif
#endif

/* Test FreeRTOS timers (with timer task) and more. */
/* Some files don't compile if this flag is disabled */
#define configUSE_TIMERS                    0
#define configTIMER_TASK_PRIORITY           CONFIG_FREERTOS_TIMER_TASK_PRIORITY
#define configTIMER_QUEUE_LENGTH            CONFIG_FREERTOS_TIMER_QUEUE_LENGTH
#define configTIMER_TASK_STACK_DEPTH        CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH

#define INCLUDE_xTimerPendFunctionCall      1
#define INCLUDE_eTaskGetState               1
#define configUSE_QUEUE_SETS                0

#define configUSE_TICKLESS_IDLE             0 // CONFIG_FREERTOS_USE_TICKLESS_IDLE
#if configUSE_TICKLESS_IDLE
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP
#endif //configUSE_TICKLESS_IDLE

#define configXT_BOARD                      1   /* Board mode */
#define configXT_SIMULATOR					0

#if CONFIG_ESP_COREDUMP_ENABLE
#define configENABLE_TASK_SNAPSHOT          1
#endif
#ifndef configENABLE_TASK_SNAPSHOT
#define configENABLE_TASK_SNAPSHOT          1
#endif

#if CONFIG_SYSVIEW_ENABLE
#ifndef __ASSEMBLER__
#include "SEGGER_SYSVIEW_esp_osal.h"
#undef INLINE // to avoid redefinition
#endif /* def __ASSEMBLER__ */
#endif

#if CONFIG_FREERTOS_CHECK_MUTEX_GIVEN_BY_OWNER
#define configCHECK_MUTEX_GIVEN_BY_OWNER    1
#else
#define configCHECK_MUTEX_GIVEN_BY_OWNER    0
#endif

#endif /* __OSAL_CONFIG_H__ */
