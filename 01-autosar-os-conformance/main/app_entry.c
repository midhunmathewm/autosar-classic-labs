#include "Os.h"

/**
 * @brief Application entry point for ESP-IDF.
 *
 * @details
 * Called by the ESP-IDF main task after the FreeRTOS scheduler
 * has already started. Initializes the AUTOSAR-style OS layer.
 *
 * @param[in]  None
 * @param[out] None
 *
 * @return void
 *
 * @note
 * This function must return to allow the FreeRTOS Idle task
 * to run and service the task watchdog.
 */
void app_main(void)
{
    StartOS(OSDEFAULTAPPMODE);
    return;
}
