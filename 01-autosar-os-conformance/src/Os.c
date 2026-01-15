#include "Os.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/* Task prototypes (AUTOSAR-style naming) */
static void InitTask(void *arg);
static void HighPrioPeriodicTask_200ms(void *arg);
static void LowPrioPeriodicTask_1000ms(void *arg);

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
        9,
        NULL
    );

    /* Periodic Task – High priority */
    xTaskCreate(
        HighPrioPeriodicTask_200ms,
        "Cyclic10ms",
        2048,
        NULL,
        8,
        NULL
    );
    /* Periodic Task – Low priority */
    xTaskCreate(
        LowPrioPeriodicTask_1000ms,
        "Cyclic10ms",
        2048,
        NULL,
        6,
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
 * @brief Periodic cyclic task with 200 ms period.
 *
 * @param[in]  arg   Task argument (unused).
 * @param[out] None
 *
 * @return void
 *
 * @note
 * Execution period is enforced using vTaskDelayUntil().
 */
static void HighPrioPeriodicTask_200ms(void *arg)
{
    (void)arg;

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        printf("High Prio 200ms Cyclic\n");
 

        /* Application logic here */

        /* Alarm-driven activation (200 ms) */
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(200));
    }
}
/**
 * @brief Periodic cyclic task with 1000 ms period.
 *
 * @param[in]  arg   Task argument (unused).
 * @param[out] None
 *
 * @return void
 *
 * @note
 * Execution period is enforced using vTaskDelayUntil().
 */
static void LowPrioPeriodicTask_1000ms(void *arg)
{
    (void)arg;

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
       printf("Low Prio 1000ms Cyclic\n");

        /* Application logic here */

        /* Alarm-driven activation (1000 ms) */
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}