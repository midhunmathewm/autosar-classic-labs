#include "Os.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/* Task prototypes (AUTOSAR-style naming) */
static void InitTask(void *arg);
static void HighPrioPeriodicTask_200ms(void *arg);
static void LowPrioPeriodicTask_1000ms(void *arg);
static void SamePrioTask_A(void *arg);
static void SamePrioTask_B(void *arg);
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

    xTaskCreate(
        SamePrioTask_A,
        "SameA",
        2048,
        NULL,
        7,
        NULL
    );
    xTaskCreate(
        SamePrioTask_B,
        "SameB",
        2048,
        NULL,
        7,
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
/**
 * @brief Same-priority task A
 *
 * This task runs at the same priority as Task B. It periodically prints
 * a message and voluntarily blocks for a short duration to allow the
 * scheduler to switch to another READY task of the same priority.
 *
 * Scheduling behavior:
 *  - Priority-based
 *  - Cooperative (blocking-based)
 *  - Time slicing enabled
 *
 * @param[in] arg  Unused task parameter (required by FreeRTOS API)
 */
static void SamePrioTask_A(void *arg)
{
    for (;;)
    {
        printf("[%lu ms] [Task A] RUNNING\n",
       xTaskGetTickCount() * portTICK_PERIOD_MS);

         /*
         * Voluntarily block the task for a short duration.
         * This causes the task to move from RUNNING → BLOCKED.
         * When the delay expires, the task becomes READY again,
         * allowing round-robin scheduling with other same-priority tasks.
         */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
/**
 * @brief Same-priority task B
 *
 * This task runs at the same priority as Task B. It periodically prints
 * a message and voluntarily blocks for a short duration to allow the
 * scheduler to switch to another READY task of the same priority.
 *
 * Scheduling behavior:
 *  - Priority-based
 *  - Cooperative (blocking-based)
 *  - Time slicing enabled
 *
 * @param[in] arg  Unused task parameter (required by FreeRTOS API)
 */
static void SamePrioTask_B(void *arg)
{
    for (;;)
    {
        printf("[Task B] RUNNING\n");

         /*
         * Voluntarily block the task for a short duration.
         * This causes the task to move from RUNNING → BLOCKED.
         * When the delay expires, the task becomes READY again,
         * allowing round-robin scheduling with other same-priority tasks.
         */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
