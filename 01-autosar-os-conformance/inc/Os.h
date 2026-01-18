#ifndef OS_H
#define OS_H

/**
 * @file    Os_Cfg.h
 * @brief   OS configuration interface.
 *
 * @details
 * Defines application mode types and the OS startup interface.
 * Platform-independent header for AUTOSAR-style OS abstraction.
 */

/**
 * @brief Application startup mode.
 */
typedef enum
{
    OSDEFAULTAPPMODE = 0  /**< Default application mode */
} AppModeType;

/**
 * @brief   Starts the OS abstraction.
 *
 * @param[in] mode  Application mode.
 *
 * @return  None.
 */
void StartOS(AppModeType mode);

#endif /* OS_H */
