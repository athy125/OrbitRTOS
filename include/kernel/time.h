/**
 * @file time.h
 * @brief Time management for the RTOS
 *
 * This file defines the time management utilities for the RTOS,
 * including system tick handling and time conversion functions.
 */

 #ifndef TIME_H
 #define TIME_H
 
 #include <stdint.h>
 
 /**
  * @brief Initialize the time management subsystem
  * 
  * @return int 0 on success, negative error code on failure
  */
 int time_init(void);
 
 /**
  * @brief Get the current system tick count
  * 
  * @return uint32_t Current tick count
  */
 uint32_t time_get_ticks(void);
 
 /**
  * @brief Get system uptime in milliseconds
  * 
  * @return uint32_t System uptime in milliseconds
  */
 uint32_t time_get_ms(void);
 
 /**
  * @brief Convert milliseconds to ticks
  * 
  * @param ms Time in milliseconds
  * @return uint32_t Equivalent time in ticks
  */
 uint32_t time_ms_to_ticks(uint32_t ms);
 
 /**
  * @brief Convert ticks to milliseconds
  * 
  * @param ticks Time in ticks
  * @return uint32_t Equivalent time in milliseconds
  */
 uint32_t time_ticks_to_ms(uint32_t ticks);
 
 /**
  * @brief Process a system tick
  * This is called by the timer interrupt handler
  * 
  * @return void
  */
 void time_tick(void);
 
 /**
  * @brief Set the system tick rate
  * 
  * @param tick_rate_ms Tick rate in milliseconds
  * @return int 0 on success, negative error code on failure
  */
 int time_set_tick_rate(uint32_t tick_rate_ms);
 
 /**
  * @brief Get the system tick rate
  * 
  * @return uint32_t Tick rate in milliseconds
  */
 uint32_t time_get_tick_rate(void);
 
 /**
  * @brief Delay for specified number of milliseconds
  * Convenience function that converts ms to ticks and calls task_delay()
  * 
  * @param ms Time to delay in milliseconds
  * @return int 0 on success, negative error code on failure
  */
 int time_delay_ms(uint32_t ms);
 
 /**
  * @brief Get time in seconds since startup
  * 
  * @return uint32_t Time in seconds
  */
 uint32_t time_get_seconds(void);
 
 /**
  * @brief Get timestamp formatted as string
  * 
  * @param buf Buffer to store formatted timestamp
  * @param buf_size Size of buffer
  * @return char* Pointer to buffer with formatted timestamp
  */
 char* time_get_timestamp(char* buf, size_t buf_size);
 
 #endif /* TIME_H */