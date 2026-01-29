#include "Os.h"
#include "OsTimer_Cfg.h"
#include "OsCounter_Cfg.h"
#include "OsAlarm_Cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/**
 * @file    Os.c
 * @brief   AUTOSAR OS Implementation for ESP32
 */

/* Task prototypes */
static void Os_InitTask(void *arg);
static void Os_HighPrioPeriodicTask_200ms(void *arg);
static void Os_LowPrioPeriodicTask_1000ms(void *arg);

/* Timer callback */
static void Os_TimerCallback(void);

/**
 * @brief Starts the OS abstraction layer
 *
 * @param[in]  mode  Application startup mode
 *
 * @return void
 *
 * @note
 * Initialization sequence:
 * 1. Initialize counters
 * 2. Initialize alarms
 * 3. Initialize and start timer
 * 4. Create application tasks
 */
void StartOS(AppModeType mode)
{
    (void)mode;

    printf("\n");
    printf("========================================\n");
    printf("  AUTOSAR OS Layer for ESP32\n");
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
    
    /* Step 5: Start timer */
    OsTimer_Start();

    printf("[StartOS] Creating application tasks...\n");

    /* Create Init Task - highest priority */
    xTaskCreate(
        Os_InitTask,
        "InitTask",
        2048,
        NULL,
        9,  /* Highest priority */
        NULL
    );

    /* Create High Priority Periodic Task (200ms cycle) */
    xTaskCreate(
        Os_HighPrioPeriodicTask_200ms,
        "HighPrio200ms",
        2048,
        NULL,
        8,
        NULL
    );

    /* Create Low Priority Periodic Task (1000ms cycle) */
    xTaskCreate(
        Os_LowPrioPeriodicTask_1000ms,
        "LowPrio1000ms",
        2048,
        NULL,
        6,
        NULL
    );

    printf("[StartOS] OS initialization complete\n");
    printf("========================================\n\n");
}

/**
 * @brief Timer callback - called every 1ms from ISR
 * 
 * @note This is called from ISR context by the hardware timer
 */
static void Os_TimerCallback(void)
{
    /* Increment system counter - this will trigger alarm processing */
    OsCounter_Increment(OsCounter_System);
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

    /* Wait for other tasks to be created */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 
     * Application-specific initialization
     * - Initialize drivers
     * - Initialize state machines
     * - Perform safety checks
     * - Configure alarms (if needed)
     */

    uint32_t systemTick = OsCounter_GetValue(OsCounter_System);
    printf("[InitTask] Initialization complete (system tick: %lu)\n", systemTick);
    printf("[InitTask] Application tasks starting...\n\n");

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
 * Execution period is enforced using vTaskDelayUntil().
 * AUTOSAR equivalent: Cyclic task with 200ms alarm
 */
static void Os_HighPrioPeriodicTask_200ms(void *arg)
{
    (void)arg;

    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t executionCount = 0;

    for (;;)
    {
        uint32_t systemTick = OsCounter_GetValue(OsCounter_System);
        uint32_t freeRtosTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        printf("[%lu ms / Tick %lu] HighPrio Task #%lu\n",
               freeRtosTick,
               systemTick,
               ++executionCount);

        /* 
         * Application logic here:
         * - Fast control loops
         * - Sensor reading
         * - Real-time processing
         */

        /* Wait for next period (200 ms) */
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(200));
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
 * Execution period is enforced using vTaskDelayUntil().
 * AUTOSAR equivalent: Cyclic task with 1000ms alarm
 */
static void Os_LowPrioPeriodicTask_1000ms(void *arg)
{
    (void)arg;

    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t executionCount = 0;

    for (;;)
    {
        uint32_t systemTick = OsCounter_GetValue(OsCounter_System);
        uint32_t freeRtosTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        printf("[%lu ms / Tick %lu] LowPrio Task #%lu\n",
               freeRtosTick,
               systemTick,
               ++executionCount);

        /* 
         * Application logic here:
         * - Diagnostics
         * - Logging
         * - Housekeeping
         * - Non-critical monitoring
         */

        /* Wait for next period (1000 ms) */
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Get OS tick count (system counter value)
 * 
 * @return Current system tick count (1 tick = 1 ms)
 */
uint32_t Os_GetTickCount(void)
{
    return OsCounter_GetValue(OsCounter_System);
}

/**
 * @brief Delay execution for specified ticks
 * 
 * @param[in] ticks Number of ticks to delay (1 tick = 1 ms)
 */
void Os_Delay(uint32_t ticks)
{
    vTaskDelay(pdMS_TO_TICKS(ticks));
}
