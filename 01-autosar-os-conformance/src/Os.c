#include "Os.h"
#include "OsTimer_Cfg.h"
#include "OsCounter_Cfg.h"
#include "OsAlarm_Cfg.h"
#include "OsEvent_Cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/**
 * @file    Os.c
 * @brief   AUTOSAR OS Implementation for ESP32 – Event-Based Task Activation
 *
 * @details
 * Implements the full AUTOSAR Classic OS Extended Task (ECC2) pattern:
 *
 *   1. Hardware Timer ISR increments the system counter every 1 ms.
 *   2. Counter increment triggers OsAlarm_ProcessCounter().
 *   3. Expired alarms invoke their registered callbacks.
 *   4. Each callback calls OsEvent_SetEvent() – sets an event flag AND
 *      wakes the owning task via task notification.
 *   5. The task returns from OsEvent_WaitEvent(), polls every event it
 *      owns with OsEvent_IsEventSet(), executes the associated application
 *      code, and clears the event with OsEvent_ClearEvent().
 *
 * Alarm -> Event mapping
 *   OsAlarm_Task50ms   ->  OsEvent_50ms   (HighPrio task)
 *   OsAlarm_Task200ms  ->  OsEvent_200ms  (HighPrio task)
 *   OsAlarm_Task100ms  ->  OsEvent_100ms  (LowPrio  task)
 *   OsAlarm_Task500ms  ->  OsEvent_500ms  (LowPrio  task)
 *
 * This decouples alarm expiry from task activation and gives each task
 * independent, fine-grained event processing – exactly as specified in
 * AUTOSAR Classic OS.
 */

/* --------------------------------------------------------------------------
 * Task handles
 * -------------------------------------------------------------------------- */
static TaskHandle_t Os_InitTaskHandle       = NULL;
static TaskHandle_t Os_HighPrioTaskHandle   = NULL;
static TaskHandle_t Os_LowPrioTaskHandle    = NULL;

/* --------------------------------------------------------------------------
 * Task function prototypes
 * -------------------------------------------------------------------------- */
static void Os_InitTask(void *arg);
static void Os_HighPrioPeriodicTask(void *arg);
static void Os_LowPrioPeriodicTask(void *arg);

/* --------------------------------------------------------------------------
 * Timer callback prototype
 * -------------------------------------------------------------------------- */
static void Os_TimerCallback(void);

/* --------------------------------------------------------------------------
 * Alarm callback prototypes – one per alarm, each calls SetEvent
 * -------------------------------------------------------------------------- */
static void Os_Alarm50msCallback(void);
static void Os_Alarm100msCallback(void);
static void Os_Alarm200msCallback(void);
static void Os_Alarm500msCallback(void);

/* ==========================================================================
 * StartOS
 * ========================================================================== */

/**
 * @brief Starts the OS abstraction layer
 *
 * @param[in]  mode  Application startup mode (unused – always default)
 *
 * @details
 * Initialisation sequence (AUTOSAR-compliant):
 *   1. Initialise counters
 *   2. Initialise alarms
 *   3. Initialise events
 *   4. Initialise and configure hardware timer
 *   5. Create application tasks
 *   6. Bind events to their owning tasks
 *   7. Register alarm callbacks
 *   8. Start hardware timer
 *   9. Activate cyclic alarms (StartupHook equivalent)
 */
void StartOS(AppModeType mode)
{
    (void)mode;

    printf("\n");
    printf("========================================\n");
    printf("  AUTOSAR OS Layer for ESP32\n");
    printf("  Event-Based Task Activation (ECC2)\n");
    printf("========================================\n");
    printf("[StartOS] Initializing BSW layer...\n");

    /* Step 1: Initialise counter subsystem */
    OsCounter_Init();

    /* Step 2: Initialise alarm subsystem */
    OsAlarm_Init();

    /* Step 3: Initialise event subsystem */
    OsEvent_Init();

    /* Step 4: Initialise hardware timer */
    OsTimer_Init();
    OsTimer_RegisterCallback(Os_TimerCallback);

    /* Step 5: Create application tasks */
    printf("[StartOS] Creating application tasks...\n");

    /* Init task – one-shot, highest priority */
    xTaskCreate(
        Os_InitTask,
        "InitTask",
        2048,
        NULL,
        9,  /* Highest priority */
        &Os_InitTaskHandle
    );

    /* High-priority task – owns OsEvent_50ms and OsEvent_200ms */
    xTaskCreate(
        Os_HighPrioPeriodicTask,
        "HighPrioTask",
        2048,
        NULL,
        8,
        &Os_HighPrioTaskHandle
    );

    /* Low-priority task – owns OsEvent_100ms and OsEvent_500ms */
    xTaskCreate(
        Os_LowPrioPeriodicTask,
        "LowPrioTask",
        2048,
        NULL,
        6,
        &Os_LowPrioTaskHandle
    );

    /* Step 6: Bind events to their owning tasks
     * This must happen after xTaskCreate() so the handles are valid.
     * Two events map to the same task – BindEvent stores the handle once
     * per task context.
     */
    printf("[StartOS] Binding events to tasks...\n");
    OsEvent_BindEvent(OsEvent_50ms,  Os_HighPrioTaskHandle);
    OsEvent_BindEvent(OsEvent_200ms, Os_HighPrioTaskHandle);
    OsEvent_BindEvent(OsEvent_100ms, Os_LowPrioTaskHandle);
    OsEvent_BindEvent(OsEvent_500ms, Os_LowPrioTaskHandle);

    /* Step 7: Register alarm callbacks
     * Each callback simply calls OsEvent_SetEvent() for its event.
     */
    printf("[StartOS] Configuring alarms...\n");
    OsAlarm_RegisterCallback(OsAlarm_Task50ms,  Os_Alarm50msCallback);
    OsAlarm_RegisterCallback(OsAlarm_Task100ms, Os_Alarm100msCallback);
    OsAlarm_RegisterCallback(OsAlarm_Task200ms, Os_Alarm200msCallback);
    OsAlarm_RegisterCallback(OsAlarm_Task500ms, Os_Alarm500msCallback);

    /* Step 8: Start hardware timer – counter increments begin */
    OsTimer_Start();

    /* Step 9: Activate cyclic alarms (AUTOSAR StartupHook equivalent)
     * Each alarm is set relative to the current counter value and recurs
     * at the same interval.
     */
    OsAlarm_SetRel(OsAlarm_Task50ms,   50,   50);   /*  50 ms cyclic */
    OsAlarm_SetRel(OsAlarm_Task100ms, 100,  100);   /* 100 ms cyclic */
    OsAlarm_SetRel(OsAlarm_Task200ms, 200,  200);   /* 200 ms cyclic */
    OsAlarm_SetRel(OsAlarm_Task500ms, 500,  500);   /* 500 ms cyclic */

    printf("[StartOS] OS initialization complete\n");
    printf("[StartOS] Alarms & Events activated:\n");
    printf("  - OsAlarm_Task50ms   ->  OsEvent_50ms   (HighPrio)  50 ms cyclic\n");
    printf("  - OsAlarm_Task100ms  ->  OsEvent_100ms  (LowPrio)  100 ms cyclic\n");
    printf("  - OsAlarm_Task200ms  ->  OsEvent_200ms  (HighPrio) 200 ms cyclic\n");
    printf("  - OsAlarm_Task500ms  ->  OsEvent_500ms  (LowPrio)  500 ms cyclic\n");
    printf("========================================\n\n");
}

/* ==========================================================================
 * Timer callback – called every 1 ms from hardware ISR
 * ========================================================================== */

/**
 * @brief Timer callback – increments the system counter
 *
 * @note  Called from ISR context by the ESP32 GP Timer.
 */
static void Os_TimerCallback(void)
{
    OsCounter_Increment(OsCounter_System);
}

/* ==========================================================================
 * Alarm callbacks – one per alarm, each sets its associated event.
 * All execute in ISR context; OsEvent_SetEvent() is ISR-safe.
 * ========================================================================== */

/**
 * @brief Alarm callback for the 50 ms alarm
 * @note  ISR context – sets OsEvent_50ms
 */
static void Os_Alarm50msCallback(void)
{
    OsEvent_SetEvent(OsEvent_50ms);
}

/**
 * @brief Alarm callback for the 100 ms alarm
 * @note  ISR context – sets OsEvent_100ms
 */
static void Os_Alarm100msCallback(void)
{
    OsEvent_SetEvent(OsEvent_100ms);
}

/**
 * @brief Alarm callback for the 200 ms alarm
 * @note  ISR context – sets OsEvent_200ms
 */
static void Os_Alarm200msCallback(void)
{
    OsEvent_SetEvent(OsEvent_200ms);
}

/**
 * @brief Alarm callback for the 500 ms alarm
 * @note  ISR context – sets OsEvent_500ms
 */
static void Os_Alarm500msCallback(void)
{
    OsEvent_SetEvent(OsEvent_500ms);
}

/* ==========================================================================
 * Init Task – one-shot system initialisation
 * ========================================================================== */

/**
 * @brief One-shot initialisation task
 *
 * @param[in] arg  Task argument (unused)
 *
 * @details
 * Executes any startup logic the application requires, waits briefly for
 * the rest of the system to settle, then terminates itself.
 * AUTOSAR equivalent: StartupHook + Init Task -> TerminateTask().
 */
static void Os_InitTask(void *arg)
{
    (void)arg;

    printf("[InitTask] System initialization start\n");

    /* Brief delay to let all tasks reach their first WaitEvent() */
    vTaskDelay(pdMS_TO_TICKS(100));

    /*
     * Application-specific one-shot initialisation can go here:
     *   - Driver initialisation
     *   - State-machine reset
     *   - Safety checks
     *   - Any additional alarm / event reconfiguration
     */

    uint32_t systemTick = OsCounter_GetValue(OsCounter_System);
    printf("[InitTask] Initialization complete (system tick: %lu)\n", systemTick);
    printf("[InitTask] Event-based application tasks ready...\n\n");

    /* AUTOSAR TerminateTask() equivalent – task deletes itself */
    vTaskDelete(NULL);
}

/* ==========================================================================
 * High-Priority Periodic Task
 * Owns: OsEvent_50ms, OsEvent_200ms
 * ========================================================================== */

/**
 * @brief High-priority extended task – event-based (ECC2)
 *
 * @param[in] arg  Task argument (unused)
 *
 * @details
 * AUTOSAR ECC2 loop:
 *   1. WaitEvent()        – block until any owned event is set
 *   2. IsEventSet()       – check each event independently
 *   3. ClearEvent()       – clear the event before executing app code
 *   4. (app code)         – placeholder for the application function
 *   5. Repeat from 1
 *
 * The order of the if-blocks defines the priority among events within this
 * task.  OsEvent_50ms is checked first (faster rate, higher implicit prio).
 */
static void Os_HighPrioPeriodicTask(void *arg)
{
    (void)arg;

    uint32_t exec50ms  = 0;
    uint32_t exec200ms = 0;

    for (;;)
    {
        /* ---------------------------------------------------------------
         * AUTOSAR WaitEvent() – blocks until SetEvent() wakes this task.
         * When we return, at least one of our events is flagged.
         * --------------------------------------------------------------- */
        OsEvent_WaitEvent();

        /* ---------------------------------------------------------------
         * Poll OsEvent_50ms
         * --------------------------------------------------------------- */
        if (OsEvent_IsEventSet(OsEvent_50ms))
        {
            /* AUTOSAR ClearEvent() – clear the flag first */
            OsEvent_ClearEvent(OsEvent_50ms);

            uint32_t tick   = OsCounter_GetValue(OsCounter_System);
            uint32_t rtTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ++exec50ms;

            printf("[%lu ms / Tick %lu] HighPrio - OsEvent_50ms  #%lu\n",
                   rtTick, tick, exec50ms);

            /*
             * *** APPLICATION FUNCTION PLACEHOLDER – 50 ms ***
             *
             * Insert your 50 ms application logic here, e.g.:
             *   App_HighPrio_50ms();
             *
             * Typical use-cases:
             *   - Fast sensor polling
             *   - Inner-loop PID control
             *   - Quick signal pre-processing
             */
        }

        /* ---------------------------------------------------------------
         * Poll OsEvent_200ms
         * --------------------------------------------------------------- */
        if (OsEvent_IsEventSet(OsEvent_200ms))
        {
            /* AUTOSAR ClearEvent() */
            OsEvent_ClearEvent(OsEvent_200ms);

            uint32_t tick   = OsCounter_GetValue(OsCounter_System);
            uint32_t rtTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ++exec200ms;

            printf("[%lu ms / Tick %lu] HighPrio - OsEvent_200ms #%lu\n",
                   rtTick, tick, exec200ms);

            /*
             * *** APPLICATION FUNCTION PLACEHOLDER – 200 ms ***
             *
             * Insert your 200 ms application logic here, e.g.:
             *   App_HighPrio_200ms();
             *
             * Typical use-cases:
             *   - Outer control loop
             *   - Actuator update
             *   - Filtered measurement aggregation
             */
        }

        /* AUTOSAR TerminateTask() is implicit – loop back to WaitEvent() */
    }
}

/* ==========================================================================
 * Low-Priority Periodic Task
 * Owns: OsEvent_100ms, OsEvent_500ms
 * ========================================================================== */

/**
 * @brief Low-priority extended task – event-based (ECC2)
 *
 * @param[in] arg  Task argument (unused)
 *
 * @details
 * Same ECC2 pattern as the high-priority task.
 * OsEvent_100ms is checked first (faster rate).
 */
static void Os_LowPrioPeriodicTask(void *arg)
{
    (void)arg;

    uint32_t exec100ms = 0;
    uint32_t exec500ms = 0;

    for (;;)
    {
        /* ---------------------------------------------------------------
         * AUTOSAR WaitEvent()
         * --------------------------------------------------------------- */
        OsEvent_WaitEvent();

        /* ---------------------------------------------------------------
         * Poll OsEvent_100ms
         * --------------------------------------------------------------- */
        if (OsEvent_IsEventSet(OsEvent_100ms))
        {
            /* AUTOSAR ClearEvent() */
            OsEvent_ClearEvent(OsEvent_100ms);

            uint32_t tick   = OsCounter_GetValue(OsCounter_System);
            uint32_t rtTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ++exec100ms;

            printf("[%lu ms / Tick %lu] LowPrio  - OsEvent_100ms #%lu\n",
                   rtTick, tick, exec100ms);

            /*
             * *** APPLICATION FUNCTION PLACEHOLDER – 100 ms ***
             *
             * Insert your 100 ms application logic here, e.g.:
             *   App_LowPrio_100ms();
             *
             * Typical use-cases:
             *   - Periodic diagnostics
             *   - Communication frame assembly
             *   - Medium-rate status monitoring
             */
        }

        /* ---------------------------------------------------------------
         * Poll OsEvent_500ms
         * --------------------------------------------------------------- */
        if (OsEvent_IsEventSet(OsEvent_500ms))
        {
            /* AUTOSAR ClearEvent() */
            OsEvent_ClearEvent(OsEvent_500ms);

            uint32_t tick   = OsCounter_GetValue(OsCounter_System);
            uint32_t rtTick = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ++exec500ms;

            printf("[%lu ms / Tick %lu] LowPrio  - OsEvent_500ms #%lu\n",
                   rtTick, tick, exec500ms);

            /*
             * *** APPLICATION FUNCTION PLACEHOLDER – 500 ms ***
             *
             * Insert your 500 ms application logic here, e.g.:
             *   App_LowPrio_500ms();
             *
             * Typical use-cases:
             *   - Housekeeping / logging
             *   - Non-critical status updates
             *   - Slow telemetry reporting
             */
        }

        /* AUTOSAR TerminateTask() is implicit – loop back to WaitEvent() */
    }
}

/* ==========================================================================
 * Utility functions
 * ========================================================================== */

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
 * @param[in] ticks  Number of ticks to delay (1 tick = 1 ms)
 *
 * @note Convenience wrapper – not part of AUTOSAR OS.
 */
void Os_Delay(uint32_t ticks)
{
    vTaskDelay(pdMS_TO_TICKS(ticks));
}
