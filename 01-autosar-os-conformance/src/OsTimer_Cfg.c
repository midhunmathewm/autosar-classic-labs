#include "OsTimer_Cfg.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include <stdio.h>
#include "Os.h"
#include "esp_attr.h"

/**
 * @file    OsTimer_Cfg.c
 * @brief   AUTOSAR OS Timer Implementation for ESP32
 */

/* Hardware timer handle */
static gptimer_handle_t osTimerHandle = NULL;

/* Timer callback */
static Os_TimerCallbackType osTimerCallback = NULL;

/* Timer running state */
static bool osTimerRunning = false;

/**
 * @brief Timer ISR callback - called every 1ms
 * 
 * @param[in] timer Timer handle
 * @param[in] edata Event data
 * @param[in] user_ctx User context (unused)
 * 
 * @return false (no higher priority task woken)
 * 
 * @note IRAM_ATTR ensures this function runs from internal RAM for speed
 */
static bool IRAM_ATTR OsTimer_ISR(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    (void)timer;
    (void)edata;
    (void)user_ctx;
    
    /* Call registered callback if available */
    if (osTimerCallback != NULL)
    {
        osTimerCallback();
    }
    
    return false;  /* No higher priority task woken */
}


/**
 * @brief Initialize hardware timer
 * 
 * @details
 * - Configures GP Timer with 1 MHz resolution (1 µs tick)
 * - Sets alarm at 1000 µs (1 ms period)
 * - Enables auto-reload for continuous operation
 * - Registers ISR callback
 */
void OsTimer_Init(void)
{
    esp_err_t ret;
    
    printf("[OsTimer] Initializing hardware timer...\n");
    
    /* Timer configuration */
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = OS_TIMER_RESOLUTION_HZ  /* 1 MHz */
    };
    
    ret = gptimer_new_timer(&timer_config, &osTimerHandle);
    if (ret != ESP_OK)
    {
        printf("[OsTimer] ERROR: Failed to create timer (0x%x)\n", ret);
        return;
    }
    
    /* Alarm configuration - trigger every 1ms */
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = OS_TIMER_PERIOD_US,  /* 1000 µs = 1 ms */
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true
    };
    
    ret = gptimer_set_alarm_action(osTimerHandle, &alarm_config);
    if (ret != ESP_OK)
    {
        printf("[OsTimer] ERROR: Failed to set alarm action (0x%x)\n", ret);
        return;
    }
    
    /* Register ISR callback */
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = OsTimer_ISR
    };
    
    ret = gptimer_register_event_callbacks(osTimerHandle, &callbacks, NULL);
    if (ret != ESP_OK)
    {
        printf("[OsTimer] ERROR: Failed to register callbacks (0x%x)\n", ret);
        return;
    }
    
    /* Enable timer */
    ret = gptimer_enable(osTimerHandle);
    if (ret != ESP_OK)
    {
        printf("[OsTimer] ERROR: Failed to enable timer (0x%x)\n", ret);
        return;
    }
    
    printf("[OsTimer] Timer initialized successfully\n");
    printf("[OsTimer] Resolution: %u Hz, Period: %u us\n", 
           OS_TIMER_RESOLUTION_HZ, OS_TIMER_PERIOD_US);
}

/**
 * @brief Register timer callback
 * 
 * @param[in] callback Function to call on each timer interrupt
 */
void OsTimer_RegisterCallback(Os_TimerCallbackType callback)
{
    osTimerCallback = callback;
    printf("[OsTimer] Callback registered\n");
}

/**
 * @brief Start hardware timer
 */
void OsTimer_Start(void)
{
    if (osTimerHandle != NULL && !osTimerRunning)
    {
        esp_err_t ret = gptimer_start(osTimerHandle);
        if (ret == ESP_OK)
        {
            osTimerRunning = true;
            printf("[OsTimer] Timer started\n");
        }
        else
        {
            printf("[OsTimer] ERROR: Failed to start timer (0x%x)\n", ret);
        }
    }
}

/**
 * @brief Stop hardware timer
 */
void OsTimer_Stop(void)
{
    if (osTimerHandle != NULL && osTimerRunning)
    {
        esp_err_t ret = gptimer_stop(osTimerHandle);
        if (ret == ESP_OK)
        {
            osTimerRunning = false;
            printf("[OsTimer] Timer stopped\n");
        }
        else
        {
            printf("[OsTimer] ERROR: Failed to stop timer (0x%x)\n", ret);
        }
    }
}

/**
 * @brief Get timer status
 * 
 * @return true if timer is running, false otherwise
 */
bool OsTimer_IsRunning(void)
{
    return osTimerRunning;
}
