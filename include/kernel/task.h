/**
 * @file task.h
 * @brief Task management interface for the RTOS simulator
 *
 * This file defines the structures and functions for task management,
 * including task creation, deletion, and state transitions.
 */

 #ifndef TASK_H
 #define TASK_H
 
 #include <stdint.h>
 #include "../config.h"
 
 /* Task states */
 typedef enum {
     TASK_STATE_READY,       /* Task is ready to execute */
     TASK_STATE_RUNNING,     /* Task is currently running */
     TASK_STATE_BLOCKED,     /* Task is blocked (e.g., waiting for resource) */
     TASK_STATE_SUSPENDED,   /* Task is suspended */
     TASK_STATE_TERMINATED   /* Task has terminated */
 } task_state_t;
 
 /* Task block reasons */
 typedef enum {
     BLOCK_REASON_NONE,          /* Not blocked */
     BLOCK_REASON_DELAY,         /* Blocked for a time delay */
     BLOCK_REASON_SEMAPHORE,     /* Blocked on a semaphore */
     BLOCK_REASON_QUEUE_FULL,    /* Blocked on a full message queue */
     BLOCK_REASON_QUEUE_EMPTY,   /* Blocked on an empty message queue */
     BLOCK_REASON_EVENT,         /* Blocked waiting for an event */
     BLOCK_REASON_MUTEX          /* Blocked on a mutex */
 } block_reason_t;
 
 /* Task context - architecture specific */
 typedef struct {
     uint32_t* stack_ptr;         /* Stack pointer */
     uint32_t stack_base;         /* Base of stack memory */
     uint32_t stack_size;         /* Size of stack in bytes */
 } task_context_t;
 
 /* Task statistics */
 typedef struct {
     uint32_t total_runtime;      /* Total time task has been running (in ticks) */
     uint32_t last_start_time;    /* Time when task last started running */
     uint32_t num_activations;    /* Number of times task has been activated */
     uint32_t deadline_misses;    /* Number of deadline misses */
     uint32_t max_execution_time; /* Maximum execution time observed */
 } task_stats_t;
 
 /* Task control block */
 typedef struct task_struct {
     char name[MAX_TASK_NAME_LEN];          /* Task name */
     task_state_t state;                    /* Current state */
     uint8_t priority;                      /* Task priority (0 = highest) */
     uint8_t original_priority;             /* Original priority (for priority inheritance) */
     uint32_t time_slice;                   /* Time slice in system ticks */
     uint32_t time_slice_count;             /* Remaining time slice counter */
     task_context_t context;                /* Task context information */
     void (*task_func)(void*);              /* Task function pointer */
     void* task_arg;                        /* Task function argument */
     uint32_t delay_until;                  /* Tick to delay until (if delayed) */
     block_reason_t block_reason;           /* Reason for blocking */
     void* block_object;                    /* Object task is blocked on */
     uint32_t period;                       /* Period for periodic tasks (in ticks) */
     uint32_t deadline;                     /* Relative deadline (in ticks) */
     uint32_t next_release;                 /* Next release time for periodic tasks */
     uint32_t absolute_deadline;            /* Absolute deadline for current job */
     task_stats_t stats;                    /* Task statistics */
     struct task_struct* next;              /* Next task in list */
     struct task_struct* prev;              /* Previous task in list */
 } task_t;
 
 /* Function prototypes */
 
 /**
  * @brief Initialize the task management subsystem
  * 
  * @return int 0 on success, negative error code on failure
  */
 int task_init(void);
 
 /**
  * @brief Create a new task
  * 
  * @param name Task name
  * @param priority Task priority (0 = highest)
  * @param task_func Task function pointer
  * @param task_arg Task function argument
  * @param stack_size Stack size in bytes
  * @return task_t* Pointer to created task, NULL on failure
  */
 task_t* task_create(const char* name, uint8_t priority, 
                    void (*task_func)(void*), void* task_arg,
                    uint32_t stack_size);
 
 /**
  * @brief Delete a task
  * 
  * @param task Task to delete
  * @return int 0 on success, negative error code on failure
  */
 int task_delete(task_t* task);
 
 /**
  * @brief Set task priority
  * 
  * @param task Task to modify
  * @param priority New priority
  * @return int 0 on success, negative error code on failure
  */
 int task_set_priority(task_t* task, uint8_t priority);
 
 /**
  * @brief Get task priority
  * 
  * @param task Task to query
  * @return uint8_t Task priority
  */
 uint8_t task_get_priority(task_t* task);
 
 /**
  * @brief Suspend a task
  * 
  * @param task Task to suspend
  * @return int 0 on success, negative error code on failure
  */
 int task_suspend(task_t* task);
 
 /**
  * @brief Resume a suspended task
  * 
  * @param task Task to resume
  * @return int 0 on success, negative error code on failure
  */
 int task_resume(task_t* task);
 
 /**
  * @brief Get current running task
  * 
  * @return task_t* Pointer to current task, NULL if no task running
  */
 task_t* task_get_current(void);
 
 /**
  * @brief Yield execution to next ready task
  * 
  * @return void
  */
 void task_yield(void);
 
 /**
  * @brief Delay task for specified number of ticks
  * 
  * @param ticks Number of ticks to delay
  * @return int 0 on success, negative error code on failure
  */
 int task_delay(uint32_t ticks);
 
 /**
  * @brief Delay task until a specific tick value
  * 
  * @param tick_value Tick value to delay until
  * @return int 0 on success, negative error code on failure
  */
 int task_delay_until(uint32_t tick_value);
 
 /**
  * @brief Configure task as periodic
  * 
  * @param task Task to configure as periodic
  * @param period Period in ticks
  * @param deadline Relative deadline in ticks (or 0 for deadline = period)
  * @return int 0 on success, negative error code on failure
  */
 int task_set_periodic(task_t* task, uint32_t period, uint32_t deadline);
 
 /**
  * @brief Get task state as string
  * 
  * @param task Task to query
  * @return const char* String representation of task state
  */
 const char* task_state_to_string(task_state_t state);
 
 /**
  * @brief Get task statistics
  * 
  * @param task Task to query
  * @param stats Pointer to statistics structure to fill
  * @return int 0 on success, negative error code on failure
  */
 int task_get_stats(task_t* task, task_stats_t* stats);
 
 /**
  * @brief Reset task statistics
  * 
  * @param task Task to reset statistics for
  * @return int 0 on success, negative error code on failure
  */
 int task_reset_stats(task_t* task);
 
 /**
  * @brief Get task by name
  * 
  * @param name Task name to search for
  * @return task_t* Pointer to found task, NULL if not found
  */
 task_t* task_get_by_name(const char* name);
 
 #endif /* TASK_H */