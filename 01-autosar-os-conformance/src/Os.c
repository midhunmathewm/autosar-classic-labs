#include "Os.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/* Task prototypes (AUTOSAR-style naming) */
static void InitTask(void *arg);
static void PeriodicTask_10ms(void *arg);

/**
 * @brief Starts the OS abstraction and creates configured tasks.
 *
 * @param[in]  mode  Application startup mode.
 * @param[out] None
 *
 * @return void
 *
 * @note
 * On ESP-IDF, this function only creates tasks.
 * The FreeRTOS scheduler is already running.
 */
void StartOS(AppModeType mode)
{
    (void)mode;

    /* Init Task – highest priority */
    xTaskCreate(
        InitTask,
        "InitTask",
        2048,
        NULL,
        5,
        NULL
    );

    /* Periodic Task – medium priority */
    xTaskCreate(
        PeriodicTask_10ms,
        "Cyclic10ms",
        2048,
        NULL,
        3,
        NULL
    );
}

/**
 * @brief One-shot initialization task.
 *
 * @param[in]  arg   Task argument (unused).
 * @param[out] None
 *
 * @return void
 *
 * @note
 * Executes system initialization and terminates itself.
 */
static void InitTask(void *arg)
{
    (void)arg;

    printf("[InitTask] System initialization start\n");

    /* Init logic here:
       - drivers
       - state manager
       - safety checks
    */

    printf("[InitTask] Initialization complete\n");

    /* AUTOSAR TerminateTask() equivalent */
    vTaskDelete(NULL);
}

/**
 * @brief Periodic cyclic task with 10 ms period.
 *
 * @param[in]  arg   Task argument (unused).
 * @param[out] None
 *
 * @return void
 *
 * @note
 * Execution period is enforced using vTaskDelayUntil().
 */
static void PeriodicTask_10ms(void *arg)
{
    (void)arg;

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        printf("[CyclicTask] 10ms execution\n");

        /* Application logic here */

        /* Alarm-driven activation (10 ms) */
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10));
    }
}
