/**
 * @file context.h
 * @brief Context switching functionality for the RTOS
 *
 * This file defines the architecture-specific context switching
 * functionality for the RTOS. For simulation purposes, this uses
 * standard C library methods rather than actual assembly.
 */

 #ifndef CONTEXT_H
 #define CONTEXT_H
 
 #include <stdint.h>
 #include "task.h"
 
 /**
  * @brief Initialize context switching
  * 
  * @return int 0 on success, negative error code on failure
  */
 int context_init(void);
 
 /**
  * @brief Initialize a task's context
  * 
  * @param task Task to initialize context for
  * @param stack_ptr Pointer to task's stack
  * @param stack_size Size of task's stack in bytes
  * @param task_func Task function pointer
  * @param task_arg Task function argument
  * @return int 0 on success, negative error code on failure
  */
 int context_init_task(task_t* task, void* stack_ptr, uint32_t stack_size,
                       void (*task_func)(void*), void* task_arg);
 
 /**
  * @brief Switch context from one task to another
  * 
  * @param from Task switching from
  * @param to Task switching to
  * @return int 0 on success, negative error code on failure
  */
 int context_switch(task_t* from, task_t* to);
 
 /**
  * @brief Start first task (no context saving needed)
  * 
  * @param task Task to start
  * @return int 0 on success, negative error code on failure
  */
 int context_start_first_task(task_t* task);
 
 /**
  * @brief Enter critical section (disable interrupts)
  * 
  * @return uint32_t Previous interrupt state
  */
 uint32_t context_enter_critical(void);
 
 /**
  * @brief Exit critical section (restore interrupts)
  * 
  * @param prev_state Previous interrupt state to restore
  * @return void
  */
 void context_exit_critical(uint32_t prev_state);
 
 /**
  * @brief Check if currently in critical section
  * 
  * @return int 1 if in critical section, 0 otherwise
  */
 int context_in_critical(void);
 
 /**
  * @brief Check if stack overflow occurred for a task
  * 
  * @param task Task to check
  * @return int 1 if overflow detected, 0 otherwise
  */
 int context_check_stack_overflow(task_t* task);
 
 /**
  * @brief Get remaining stack space for a task
  * 
  * @param task Task to check
  * @return uint32_t Remaining stack space in bytes
  */
 uint32_t context_get_stack_free(task_t* task);
 
 #endif /* CONTEXT_H */