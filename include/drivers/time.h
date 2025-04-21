/**
 * @file timer.h
 * @brief Timer driver for the RTOS
 *
 * This file defines the timer interface for the RTOS, providing
 * hardware abstraction for system tick generation and timing.
 */

 #ifndef TIMER_H
 #define TIMER_H
 
 #include <stdint.h>
 
 /* Timer callback function type */
 typedef void (*timer_callback_t)(void* arg);
 
 /* Timer structure */
 typedef struct {
     uint32_t period;                /* Timer period in ticks */
     uint32_t count;                 /* Current count value */
     uint8_t is_periodic;            /* 1 for periodic, 0 for one-shot */
     uint8_t is_active;              /* 1 if timer is running */
     timer_callback_t callback;      /* Callback function */
     void* callback_arg;             /* Callback argument */
     char name[16];                  /* Timer name */
 } timer_t;
 
 /**
  * @brief Initialize the timer subsystem
  * 
  * @return int 0 on success, negative error code on failure
  */
 int timer_init(void);
 
 /**
  * @brief Start system tick timer
  * 
  * @param tick_rate_ms System tick rate in milliseconds
  * @return int 0 on success, negative error code on failure
  */
 int timer_start_tick(uint32_t tick_rate_ms);
 
 /**
  * @brief Stop system tick timer
  * 
  * @return int 0 on success, negative error code on failure
  */
 int timer_stop_tick(void);
 
 /**
  * @brief Create a software timer
  * 
  * @param name Timer name
  * @param period_ms Timer period in milliseconds
  * @param is_periodic 1 for periodic, 0 for one-shot
  * @param callback Callback function
  * @param callback_arg Callback argument
  * @return timer_t* Pointer to created timer, NULL on failure
  */
 timer_t* timer_create(const char* name, uint32_t period_ms, uint8_t is_periodic,
                       timer_callback_t callback, void* callback_arg);
 
 /**
  * @brief Delete a software timer
  * 
  * @param timer Timer to delete
  * @return int 0 on success, negative error code on failure
  */
 int timer_delete(timer_t* timer);
 
 /**
  * @brief Start a software timer
  * 
  * @param timer Timer to start
  * @return int 0 on success, negative error code on failure
  */
 int timer_start(timer_t* timer);
 
 /**
  * @brief Stop a software timer
  * 
  * @param timer Timer to stop
  * @return int 0 on success, negative error code on failure
  */
 int timer_stop(timer_t* timer);
 
 /**
  * @brief Reset a software timer
  * 
  * @param timer Timer to reset
  * @return int 0 on success, negative error code on failure
  */
 int timer_reset(timer_t* timer);
 
 /**
  * @brief Change period of a software timer
  * 
  * @param timer Timer to modify
  * @param period_ms New period in milliseconds
  * @return int 0 on success, negative error code on failure
  */
 int timer_set_period(timer_t* timer, uint32_t period_ms);
 
 /**
  * @brief Check if timer is running
  * 
  * @param timer Timer to check
  * @return int 1 if running, 0 if stopped, negative error code on failure
  */
 int timer_is_running(timer_t* timer);
 
 /**
  * @brief Get time remaining until timer expires
  * 
  * @param timer Timer to query
  * @param ms_remaining Pointer to store remaining time in milliseconds
  * @return int 0 on success, negative error code on failure
  */
 int timer_get_remaining(timer_t* timer, uint32_t* ms_remaining);
 
 /**
  * @brief Get high-resolution time since startup
  * 
  * @return uint64_t Time in microseconds
  */
 uint64_t timer_get_us(void);
 
 /**
  * @brief Delay for specified number of microseconds
  * This is a busy-wait delay, not suitable for RTOS tasks
  * 
  * @param us Time to delay in microseconds
  * @return void
  */
 void timer_delay_us(uint32_t us);
 
 /**
  * @brief Process all active timers
  * Called internally by system tick handler
  * 
  * @return int Number of timers processed
  */
 int timer_process(void);
 
 #endif /* TIMER_H */