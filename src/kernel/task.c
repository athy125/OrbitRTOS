/**
 * @file task.c
 * @brief Implementation of RTOS task management
 */

 #include <stdlib.h>
 #include <string.h>
 #include "../../include/kernel/task.h"
 #include "../../include/kernel/scheduler.h"
 #include "../../include/kernel/context.h"
 #include "../../include/kernel/time.h"
 #include "../../include/utils/logger.h"
 #include "../../include/config.h"
 
 /* Array of all tasks in the system */
 static task_t* task_list[MAX_TASKS];
 static uint32_t task_count = 0;
 
 /* Current running task */
 static task_t* current_task = NULL;
 
 /* Idle task */
 static task_t* idle_task = NULL;
 
 /**
  * Idle task function - runs when no other tasks are ready
  */
 static void idle_task_func(void* arg) {
     while (1) {
         /* CPU usage can be calculated based on idle task runtime */
         /* Just yield to other tasks */
         task_yield();
     }
 }
 
 /**
  * Initialize the task management subsystem
  */
 int task_init(void) {
     LOG_INFO("Initializing task management");
     
     /* Clear task list */
     for (int i = 0; i < MAX_TASKS; i++) {
         task_list[i] = NULL;
     }
     
     task_count = 0;
     current_task = NULL;
     
     /* Create idle task */
     idle_task = task_create("idle", MAX_PRIORITY_LEVELS - 1, idle_task_func, NULL, DEFAULT_STACK_SIZE / 2);
     if (idle_task == NULL) {
         LOG_ERROR("Failed to create idle task");
         return -1;
     }
     
     LOG_INFO("Task management initialized with idle task");
     return 0;
 }
 
 /**
  * Create a new task
  */
 task_t* task_create(const char* name, uint8_t priority, 
                    void (*task_func)(void*), void* task_arg,
                    uint32_t stack_size) {
     task_t* task = NULL;
     uint32_t* stack = NULL;
     
     /* Check parameters */
     if (name == NULL || task_func == NULL || 
         priority >= MAX_PRIORITY_LEVELS || 
         task_count >= MAX_TASKS) {
         LOG_ERROR("Invalid task parameters");
         return NULL;
     }
     
     /* Allocate task structure */
     task = (task_t*) malloc(sizeof(task_t));
     if (task == NULL) {
         LOG_ERROR("Failed to allocate task structure");
         return NULL;
     }
     
     /* Allocate stack */
     stack = (uint32_t*) malloc(stack_size);
     if (stack == NULL) {
         LOG_ERROR("Failed to allocate task stack");
         free(task);
         return NULL;
     }
     
     /* Initialize task structure */
     strncpy(task->name, name, MAX_TASK_NAME_LEN - 1);
     task->name[MAX_TASK_NAME_LEN - 1] = '\0';
     task->state = TASK_STATE_READY;
     task->priority = priority;
     task->original_priority = priority;
     task->time_slice = DEFAULT_TIME_SLICE;
     task->time_slice_count = DEFAULT_TIME_SLICE;
     task->task_func = task_func;
     task->task_arg = task_arg;
     task->delay_until = 0;
     task->block_reason = BLOCK_REASON_NONE;
     task->block_object = NULL;
     task->period = 0;
     task->deadline = 0;
     task->next_release = 0;
     task->absolute_deadline = 0;
     task->next = NULL;
     task->prev = NULL;
     
     /* Initialize task context */
     if (context_init_task(task, stack, stack_size, task_func, task_arg) != 0) {
         LOG_ERROR("Failed to initialize task context");
         free(stack);
         free(task);
         return NULL;
     }
     
     /* Initialize task statistics */
     task->stats.total_runtime = 0;
     task->stats.last_start_time = 0;
     task->stats.num_activations = 0;
     task->stats.deadline_misses = 0;
     task->stats.max_execution_time = 0;
     
     /* Add task to array */
     for (int i = 0; i < MAX_TASKS; i++) {
         if (task_list[i] == NULL) {
             task_list[i] = task;
             task_count++;
             break;
         }
     }
     
     /* Add task to scheduler */
     if (scheduler_add_task(task) != 0) {
         LOG_ERROR("Failed to add task to scheduler");
         free(stack);
         free(task);
         return NULL;
     }
     
     LOG_INFO("Created task '%s', priority=%u, stack=%u bytes", 
              task->name, task->priority, stack_size);
     
     return task;
 }
 
 /**
  * Delete a task
  */
 int task_delete(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Cannot delete current task this way */
     if (task == current_task) {
         LOG_ERROR("Cannot delete current task");
         return -1;
     }
     
     /* Cannot delete idle task */
     if (task == idle_task) {
         LOG_ERROR("Cannot delete idle task");
         return -1;
     }
     
     /* Remove from scheduler */
     if (scheduler_remove_task(task) != 0) {
         LOG_ERROR("Failed to remove task from scheduler");
         return -1;
     }
     
     /* Remove from task list */
     for (int i = 0; i < MAX_TASKS; i++) {
         if (task_list[i] == task) {
             task_list[i] = NULL;
             task_count--;
             break;
         }
     }
     
     LOG_INFO("Deleted task '%s'", task->name);
     
     /* Free stack */
     if (task->context.stack_ptr != NULL) {
         free((void*)task->context.stack_base);
     }
     
     /* Free task structure */
     free(task);
     
     return 0;
 }
 
 /**
  * Set task priority
  */
 int task_set_priority(task_t* task, uint8_t priority) {
     if (task == NULL || priority >= MAX_PRIORITY_LEVELS) {
         LOG_ERROR("Invalid parameters");
         return -1;
     }
     
     /* Update priority */
     task->priority = priority;
     task->original_priority = priority;
     
     /* Notify scheduler of priority change */
     if (scheduler_update_task_state(task, task->state) != 0) {
         LOG_ERROR("Failed to update scheduler for priority change");
         return -1;
     }
     
     LOG_INFO("Set task '%s' priority to %u", task->name, priority);
     return 0;
 }
 
 /**
  * Get task priority
  */
 uint8_t task_get_priority(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return MAX_PRIORITY_LEVELS;  /* Invalid priority */
     }
     
     return task->priority;
 }
 
 /**
  * Suspend a task
  */
 int task_suspend(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Cannot suspend idle task */
     if (task == idle_task) {
         LOG_ERROR("Cannot suspend idle task");
         return -1;
     }
     
     /* Update task state */
     task->state = TASK_STATE_SUSPENDED;
     
     /* Notify scheduler of state change */
     if (scheduler_update_task_state(task, TASK_STATE_SUSPENDED) != 0) {
         LOG_ERROR("Failed to update scheduler for suspend");
         return -1;
     }
     
     /* If suspending current task, yield */
     if (task == current_task) {
         task_yield();
     }
     
     LOG_INFO("Suspended task '%s'", task->name);
     return 0;
 }
 
 /**
  * Resume a suspended task
  */
 int task_resume(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Only resume if suspended */
     if (task->state != TASK_STATE_SUSPENDED) {
         LOG_WARNING("Task '%s' is not suspended", task->name);
         return 0;
     }
     
     /* Update task state */
     task->state = TASK_STATE_READY;
     
     /* Notify scheduler of state change */
     if (scheduler_update_task_state(task, TASK_STATE_READY) != 0) {
         LOG_ERROR("Failed to update scheduler for resume");
         return -1;
     }
     
     LOG_INFO("Resumed task '%s'", task->name);
     return 0;
 }
 
 /**
  * Get current running task
  */
 task_t* task_get_current(void) {
     return current_task;
 }
 
 /**
  * Yield execution to next ready task
  */
 void task_yield(void) {
     /* Trigger context switch */
     scheduler_context_switch();
 }
 
 /**
  * Delay task for specified number of ticks
  */
 int task_delay(uint32_t ticks) {
     if (current_task == NULL) {
         LOG_ERROR("No current task");
         return -1;
     }
     
     /* Cannot delay idle task */
     if (current_task == idle_task) {
         LOG_ERROR("Cannot delay idle task");
         return -1;
     }
     
     if (ticks == 0) {
         /* Just yield if delay is 0 */
         task_yield();
         return 0;
     }
     
     /* Calculate wake time */
     uint32_t current_tick = time_get_ticks();
     current_task->delay_until = current_tick + ticks;
     
     /* Block task */
     if (scheduler_block_task(current_task, BLOCK_REASON_DELAY, NULL) != 0) {
         LOG_ERROR("Failed to block task for delay");
         return -1;
     }
     
     /* Trigger context switch */
     scheduler_context_switch();
     
     return 0;
 }
 
 /**
  * Delay task until a specific tick value
  */
 int task_delay_until(uint32_t tick_value) {
     if (current_task == NULL) {
         LOG_ERROR("No current task");
         return -1;
     }
     
     /* Cannot delay idle task */
     if (current_task == idle_task) {
         LOG_ERROR("Cannot delay idle task");
         return -1;
     }
     
     uint32_t current_tick = time_get_ticks();
     
     /* If target time is in the past, just yield */
     if (tick_value <= current_tick) {
         task_yield();
         return 0;
     }
     
     /* Set wake time */
     current_task->delay_until = tick_value;
     
     /* Block task */
     if (scheduler_block_task(current_task, BLOCK_REASON_DELAY, NULL) != 0) {
         LOG_ERROR("Failed to block task for delay until");
         return -1;
     }
     
     /* Trigger context switch */
     scheduler_context_switch();
     
     return 0;
 }
 
 /**
  * Configure task as periodic
  */
 int task_set_periodic(task_t* task, uint32_t period, uint32_t deadline) {
     if (task == NULL || period == 0) {
         LOG_ERROR("Invalid parameters");
         return -1;
     }
     
     /* Set periodic parameters */
     task->period = period;
     task->deadline = (deadline > 0) ? deadline : period;
     
     /* Initialize next release time */
     task->next_release = time_get_ticks() + period;
     task->absolute_deadline = task->next_release + task->deadline;
     
     LOG_INFO("Set task '%s' as periodic (period=%u, deadline=%u)",
              task->name, period, task->deadline);
     return 0;
 }
 
 /**
  * Get task state as string
  */
 const char* task_state_to_string(task_state_t state) {
     switch (state) {
         case TASK_STATE_READY:
             return "READY";
         case TASK_STATE_RUNNING:
             return "RUNNING";
         case TASK_STATE_BLOCKED:
             return "BLOCKED";
         case TASK_STATE_SUSPENDED:
             return "SUSPENDED";
         case TASK_STATE_TERMINATED:
             return "TERMINATED";
         default:
             return "UNKNOWN";
     }
 }
 
 /**
  * Get task statistics
  */
 int task_get_stats(task_t* task, task_stats_t* stats) {
     if (task == NULL || stats == NULL) {
         LOG_ERROR("Invalid parameters");
         return -1;
     }
     
     /* Copy statistics */
     memcpy(stats, &task->stats, sizeof(task_stats_t));
     
     return 0;
 }
 
 /**
  * Reset task statistics
  */
 int task_reset_stats(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Reset statistics */
     task->stats.total_runtime = 0;
     task->stats.last_start_time = 0;
     task->stats.num_activations = 0;
     task->stats.deadline_misses = 0;
     task->stats.max_execution_time = 0;
     
     return 0;
 }
 
 /**
  * Get task by name
  */
 task_t* task_get_by_name(const char* name) {
     if (name == NULL) {
         LOG_ERROR("NULL name pointer");
         return NULL;
     }
     
     /* Search for task with matching name */
     for (int i = 0; i < MAX_TASKS; i++) {
         if (task_list[i] != NULL && strcmp(task_list[i]->name, name) == 0) {
             return task_list[i];
         }
     }
     
     LOG_WARNING("Task '%s' not found", name);
     return NULL;
 }
 
 /**
  * Set the current task (called by scheduler)
  */
 void task_set_current(task_t* task) {
     current_task = task;
 }
 
 /**
  * Get the idle task
  */
 task_t* task_get_idle(void) {
     return idle_task;
 }