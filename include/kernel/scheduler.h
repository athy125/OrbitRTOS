/**
 * @file scheduler.h
 * @brief RTOS scheduler interface
 *
 * This file defines the structures and functions for the RTOS scheduler,
 * which manages task execution based on priorities and scheduling policies.
 */

 #ifndef SCHEDULER_H
 #define SCHEDULER_H
 
 #include <stdint.h>
 #include "task.h"
 
 /* Scheduler statistics */
 typedef struct {
     uint32_t context_switches;       /* Number of context switches */
     uint32_t tasks_created;          /* Number of tasks created */
     uint32_t tasks_deleted;          /* Number of tasks deleted */
     uint32_t scheduler_invocations;  /* Number of times scheduler was invoked */
     uint32_t idle_time;              /* Time spent in idle task (in ticks) */
     uint32_t system_time;            /* Total system uptime (in ticks) */
     float cpu_load;                  /* CPU load (0.0-1.0) */
     uint32_t deadline_misses;        /* Total number of deadline misses */
 } scheduler_stats_t;
 
 /* Scheduler state */
 typedef enum {
     SCHEDULER_STOPPED,       /* Scheduler is not running */
     SCHEDULER_RUNNING        /* Scheduler is running */
 } scheduler_state_t;
 
 /**
  * @brief Initialize the scheduler
  * 
  * @param policy Scheduling policy to use
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_init(uint8_t policy);
 
 /**
  * @brief Start the scheduler
  * 
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_start(void);
 
 /**
  * @brief Stop the scheduler
  * 
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_stop(void);
 
 /**
  * @brief Get current scheduler state
  * 
  * @return scheduler_state_t Current scheduler state
  */
 scheduler_state_t scheduler_get_state(void);
 
 /**
  * @brief Add a task to the scheduler
  * 
  * @param task Task to add
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_add_task(task_t* task);
 
 /**
  * @brief Remove a task from the scheduler
  * 
  * @param task Task to remove
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_remove_task(task_t* task);
 
 /**
  * @brief Get the next task to run according to the scheduling policy
  * 
  * @return task_t* Next task to run, NULL if no tasks are ready
  */
 task_t* scheduler_get_next_task(void);
 
 /**
  * @brief Notify the scheduler that the current task is blocked
  * 
  * @param task Task that is blocked
  * @param reason Reason for blocking
  * @param block_object Object task is blocked on (e.g., semaphore)
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_block_task(task_t* task, block_reason_t reason, void* block_object);
 
 /**
  * @brief Notify the scheduler that a task is unblocked
  * 
  * @param task Task that is unblocked
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_unblock_task(task_t* task);
 
 /**
  * @brief Perform a context switch
  * 
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_context_switch(void);
 
 /**
  * @brief Update task state in the scheduler
  * 
  * @param task Task to update
  * @param new_state New task state
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_update_task_state(task_t* task, task_state_t new_state);
 
 /**
  * @brief Process system tick
  * This function should be called every system tick
  * 
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_tick(void);
 
 /**
  * @brief Get scheduler statistics
  * 
  * @param stats Pointer to statistics structure to fill
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_get_stats(scheduler_stats_t* stats);
 
 /**
  * @brief Reset scheduler statistics
  * 
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_reset_stats(void);
 
 /**
  * @brief Set scheduling policy
  * 
  * @param policy Scheduling policy to use
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_set_policy(uint8_t policy);
 
 /**
  * @brief Get current scheduling policy
  * 
  * @return uint8_t Current scheduling policy
  */
 uint8_t scheduler_get_policy(void);
 
 /**
  * @brief Check for and handle deadline misses
  * 
  * @return int Number of deadline misses detected
  */
 int scheduler_check_deadlines(void);
 
 /**
  * @brief Lock scheduler (prevent context switches)
  * 
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_lock(void);
 
 /**
  * @brief Unlock scheduler (allow context switches)
  * 
  * @return int 0 on success, negative error code on failure
  */
 int scheduler_unlock(void);
 
 /**
  * @brief Get string name for scheduling policy
  * 
  * @param policy Policy ID
  * @return const char* String name for policy
  */
 const char* scheduler_policy_to_string(uint8_t policy);
 
 #endif /* SCHEDULER_H */