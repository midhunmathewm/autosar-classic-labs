#include "Os.h"
#include "OsTimer_Cfg.h"
#include "OsCounter_Cfg.h"
#include "OsAlarm_Cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>

/**
 * @file    Os.c
 * @brief   AUTOSAR OS Implementation for ESP32
 * 
 * @details
 * Implements AUTOSAR-style OS using alarm-based task activation.
 * Tasks are triggered by alarms instead of using vTaskDelayUntil().
 */

/* Task handles */
static TaskHandle_t Os_InitTaskHandle = NULL;
static TaskHandle_t Os_HighPrioTaskHandle = NULL;
static TaskHandle_t Os_LowPrioTaskHandle = NULL;

/* Task notification indices */
#define TASK_NOTIFY_INDEX_ALARM  (0)

/* Task prototypes */
static void Os_InitTask(void *arg);
static void Os_HighPrioPeriodicTask_200ms(void *arg);
static void Os_LowPrioPeriodicTask_1000ms(void *arg);

/* Timer callback */
static void Os_TimerCallback(void);

/* Alarm callbacks */
static void Os_Alarm200msCallback(void);
static void Os_Alarm1000msCallback(void);

/**
 * @brief Starts the OS abstraction layer
 *
 * @param[in]  mode  Application startup mode
 *
 * @return void
 *
 * @note
 * Initialization sequence (AUTOSAR-compliant):
 * 1. Initialize counters
 * 2. Initialize alarms
 * 3. Initialize and start timer
 * 4. Create application tasks
 * 5. Configure and activate alarms
 */
void StartOS(AppModeType mode)
{
    (void)mode;

    printf("\n");
    printf("========================================\n");
    printf("  AUTOSAR OS Layer for ESP32\n");
    printf("  Alarm-Based Task Activation\n");
    printf("========================================\n");
    printf("[StartOS] Initializing BSW layer...\n");

    /* Step 1: Initialize counter subsystem */
    OsCounter_Init();
    
    /* Step 2: Initialize alarm subsystem */
    OsAlarm_Init();
    
    /* Step 3: Initialize timer */
    OsTimer_Init();
    
    /* Step 4: Register timer callback */
    OsTimer_RegisterCallback(Os_TimerCallback);

    printf("[StartOS] Creating application tasks...\n");

    /* Create Init Task - highest priority */
    xTaskCreate(
        Os_InitTask,
        "InitTask",
        2048,
        NULL,
        9,  /* Highest priority */
        &Os_InitTaskHandle
    );

    /* Create High Priority Periodic Task (activated by 200ms alarm) */
    xTaskCreate(
        Os_HighPrioPeriodicTask_200ms,
        "HighPrio200ms",
        2048,
        NULL,
        8,
        &Os_HighPrioTaskHandle
    );

    /* Create Low Priority Periodic Task (activated by 1000ms alarm) */
    xTaskCreate(
        Os_LowPrioPeriodicTask_1000ms,
        "LowPrio1000ms",
        2048,
        NULL,
        6,
        &Os_LowPrioTaskHandle
    );

    /* Step 5: Configure alarm callbacks */
    printf("[StartOS] Configuring alarms...\n");
    OsAlarm_RegisterCallback(OsAlarm_Task200ms, Os_Alarm200msCallback);
    OsAlarm_RegisterCallback(OsAlarm_Task1000ms, Os_Alarm1000msCallback);
    
    /* Step 6: Start timer (this enables counter increment) */
    OsTimer_Start();
    
    /* Step 7: Activate alarms
     * Note: Alarms are activated after tasks are created and timer is started
     * This follows AUTOSAR pattern where alarms are configured in StartupHook
     */
    OsAlarm_SetRel(OsAlarm_Task200ms, 200, 200);   /* First at 200ms, then every 200ms */
    OsAlarm_SetRel(OsAlarm_Task1000ms, 1000, 1000); /* First at 1000ms, then every 1000ms */

    printf("[StartOS] OS initialization complete\n");
    printf("[StartOS] Alarms activated:\n");
    printf("  - OsAlarm_Task200ms:  200ms cyclic\n");
    printf("  - OsAlarm_Task1000ms: 1000ms cyclic\n");
    printf("========================================\n\n");
}

/**
 * @brief Timer callback - called every 1ms from ISR
 * 
 * @details
 * This is the system tick handler. It increments the system counter,
 * which in turn triggers alarm processing.
 * 
 * @note Called from ISR context by the hardware timer
 */
static void Os_TimerCallback(void)
{
    /* Increment system counter - this will trigger alarm processing */
    OsCounter_Increment(OsCounter_System);
}

/**
 * @brief Alarm callback for 200ms task
 * 
 * @details
 * Called from ISR context when the 200ms alarm expires.
 * Activates (notifies) the high priority task.
 * 
 * @note ISR context - must be short and ISR-safe
 */
static void Os_Alarm200msCallback(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* Activate task using task notification (AUTOSAR ActivateTask equivalent) */
    if (Os_HighPrioTaskHandle != NULL)
    {
        vTaskNotifyGiveIndexedFromISR(
            Os_HighPrioTaskHandle,
            TASK_NOTIFY_INDEX_ALARM,
            &xHigherPriorityTaskWoken
        );
    }
    
    /* Yield if a higher priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Alarm callback for 1000ms task
 * 
 * @details
 * Called from ISR context when the 1000ms alarm expires.
 * Activates (notifies) the low priority task.
 * 
 * @note ISR context - must be short and ISR-safe
 */
static void Os_Alarm1000msCallback(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* Activate task using task notification (AUTOSAR ActivateTask equivalent) */
    if (Os_LowPrioTaskHandle != NULL)
    {
        vTaskNotifyGiveIndexedFromISR(
            Os_LowPrioTaskHandle,
            TASK_NOTIFY_INDEX_ALARM,
            &xHigherPriorityTaskWoken
        );
    }
    
    /* Yield if a higher priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief One-shot initialization task
 *
 * @param[in]  arg   Task argument (unused)
 *
 * @return void
 *
 * @note
 * Executes system initialization and terminates itself.
 * AUTOSAR equivalent: Startup Hook + Init Task
 */
static void Os_InitTask(void *arg)
{
    (void)arg;

    printf("[InitTask] System initialization start\n");

    /* Wait for other tasks to be created and timer to start */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 
     * Application-specific initialization
     * - Initialize drivers
     * - Initialize state machines
     * - Perform safety checks
     * - Additional alarm configuration if needed
     */

    uint32_t systemTick = OsCounter_GetValue(OsCounter_System);
    printf("[InitTask] Initialization complete (system tick: %lu)\n", systemTick);
    printf("[InitTask] Application tasks ready for activation...\n\n");

    /* AUTOSAR TerminateTask() equivalent */
    vTaskDelete(NULL);
}

/**
 * @brief High priority periodic task with 200ms period
 *
 * @param[in]  arg   Task argument (unused)
 *
 * @return void
 *
 * @note
 * Task is activated by OsAlarm_Task200ms alarm.
 * Uses task notification as AUTOSAR ActivateTask() equivalent.
 * Implements AUTOSAR Extended Task pattern (WaitEvent/SetEvent).
 */
static void Os_HighPrioPeriodicTask_200ms(void *arg)
{
    (void)arg;

    uint32_t executionCount = 0;

    for (;;)
    {
        /* Wait for alarm notification (AUTOSAR WaitEvent equivalent) */
        ulTaskNotifyTakeIndexed(
            TASK_NOTIFY_INDEX_ALARM,
            pdTRUE,           /* Clear notification on exit */
            portMAX_DELAY     /* Wait indefinitely */
        );

        /* Task body starts here */
        uint32_t systemTick = OsCounter_GetValue(OsCounter_System);
        uint32_t freeRtosTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        printf("[%lu ms / Tick %lu] HighPrio Task #%lu (alarm-triggered)\n",
               freeRtosTick,
               systemTick,
               ++executionCount);

        /* 
         * Application logic here:
         * - Fast control loops
         * - Sensor reading
         * - Real-time processing
         * - Signal processing
         */

        /* Task body ends - automatically goes back to wait for next alarm */
        /* AUTOSAR TerminateTask() is implicit (task returns to wait state) */
    }
}

/**
 * @brief Low priority periodic task with 1000ms period
 *
 * @param[in]  arg   Task argument (unused)
 *
 * @return void
 *
 * @note
 * Task is activated by OsAlarm_Task1000ms alarm.
 * Uses task notification as AUTOSAR ActivateTask() equivalent.
 * Implements AUTOSAR Extended Task pattern (WaitEvent/SetEvent).
 */
static void Os_LowPrioPeriodicTask_1000ms(void *arg)
{
    (void)arg;

    uint32_t executionCount = 0;

    for (;;)
    {
        /* Wait for alarm notification (AUTOSAR WaitEvent equivalent) */
        ulTaskNotifyTakeIndexed(
            TASK_NOTIFY_INDEX_ALARM,
            pdTRUE,           /* Clear notification on exit */
            portMAX_DELAY     /* Wait indefinitely */
        );

        /* Task body starts here */
        uint32_t systemTick = OsCounter_GetValue(OsCounter_System);
        uint32_t freeRtosTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        printf("[%lu ms / Tick %lu] LowPrio Task #%lu (alarm-triggered)\n",
               freeRtosTick,
               systemTick,
               ++executionCount);

        /* 
         * Application logic here:
         * - Diagnostics
         * - Logging
         * - Housekeeping
         * - Non-critical monitoring
         * - Status updates
         */

        /* Task body ends - automatically goes back to wait for next alarm */
        /* AUTOSAR TerminateTask() is implicit (task returns to wait state) */
    }
}

/**
 * @brief Get OS tick count (system counter value)
 * 
 * @return Current system tick count (1 tick = 1 ms)
 * 
 * @note AUTOSAR equivalent: GetCounterValue()
 */
uint32_t Os_GetTickCount(void)
{
    return OsCounter_GetValue(OsCounter_System);
}

/**
 * @brief Delay execution for specified ticks
 * 
 * @param[in] ticks Number of ticks to delay (1 tick = 1 ms)
 * 
 * @note This is a convenience function, not part of AUTOSAR OS
 */
void Os_Delay(uint32_t ticks)
{
    vTaskDelay(pdMS_TO_TICKS(ticks));
}
