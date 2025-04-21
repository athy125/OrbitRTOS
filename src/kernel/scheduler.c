/**
 * @file scheduler.c
 * @brief Implementation of RTOS scheduler
 */

 #include <stdlib.h>
 #include <string.h>
 #include "../../include/kernel/scheduler.h"
 #include "../../include/kernel/task.h"
 #include "../../include/kernel/context.h"
 #include "../../include/kernel/time.h"
 #include "../../include/utils/logger.h"
 #include "../../include/utils/list.h"
 #include "../../include/config.h"
 
 /* External functions from task.c */
 extern void task_set_current(task_t* task);
 extern task_t* task_get_idle(void);
 
 /* Scheduler state */
 static scheduler_state_t scheduler_state = SCHEDULER_STOPPED;
 
 /* Current scheduling policy */
 static uint8_t current_policy = DEFAULT_SCHEDULING_POLICY;
 
 /* Ready lists for each priority level */
 static list_t ready_lists[MAX_PRIORITY_LEVELS];
 
 /* Blocked task list */
 static list_t blocked_list;
 
 /* Suspended task list */
 static list_t suspended_list;
 
 /* Scheduler statistics */
 static scheduler_stats_t stats;
 
 /* Scheduler lock counter (for critical sections) */
 static uint32_t scheduler_lock_count = 0;
 
 /**
  * Initialize the scheduler
  */
 int scheduler_init(uint8_t policy) {
     LOG_INFO("Initializing scheduler with policy %s", 
              scheduler_policy_to_string(policy));
     
     /* Initialize task lists */
     for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
         if (list_init(&ready_lists[i]) != 0) {
             LOG_ERROR("Failed to initialize ready list %d", i);
             return -1;
         }
     }
     
     if (list_init(&blocked_list) != 0) {
         LOG_ERROR("Failed to initialize blocked list");
         return -1;
     }
     
     if (list_init(&suspended_list) != 0) {
         LOG_ERROR("Failed to initialize suspended list");
         return -1;
     }
     
     /* Set scheduling policy */
     current_policy = policy;
     
     /* Reset statistics */
     memset(&stats, 0, sizeof(scheduler_stats_t));
     
     /* Reset lock counter */
     scheduler_lock_count = 0;
     
     /* Set state */
     scheduler_state = SCHEDULER_STOPPED;
     
     LOG_INFO("Scheduler initialized");
     return 0;
 }
 
 /**
  * Start the scheduler
  */
 int scheduler_start(void) {
     LOG_INFO("Starting scheduler");
     
     /* Check if already running */
     if (scheduler_state == SCHEDULER_RUNNING) {
         LOG_WARNING("Scheduler already running");
         return 0;
     }
     
     /* Reset statistics */
     memset(&stats, 0, sizeof(scheduler_stats_t));
     
     /* Set state */
     scheduler_state = SCHEDULER_RUNNING;
     
     /* Get first task to run */
     task_t* first_task = scheduler_get_next_task();
     if (first_task == NULL) {
         LOG_ERROR("No tasks ready to run");
         scheduler_state = SCHEDULER_STOPPED;
         return -1;
     }
     
     /* Start first task */
     if (context_start_first_task(first_task) != 0) {
         LOG_ERROR("Failed to start first task");
         scheduler_state = SCHEDULER_STOPPED;
         return -1;
     }
     
     /* We should never get here, as context_start_first_task does not return */
     LOG_ERROR("Unexpected return from context_start_first_task");
     return -1;
 }
 
 /**
  * Stop the scheduler
  */
 int scheduler_stop(void) {
     LOG_INFO("Stopping scheduler");
     
     /* Check if already stopped */
     if (scheduler_state == SCHEDULER_STOPPED) {
         LOG_WARNING("Scheduler already stopped");
         return 0;
     }
     
     /* Set state */
     scheduler_state = SCHEDULER_STOPPED;
     
     LOG_INFO("Scheduler stopped");
     return 0;
 }
 
 /**
  * Get current scheduler state
  */
 scheduler_state_t scheduler_get_state(void) {
     return scheduler_state;
 }
 
 /**
  * Add a task to the scheduler
  */
 int scheduler_add_task(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Add to appropriate list based on state */
     switch (task->state) {
         case TASK_STATE_READY:
             if (list_append(&ready_lists[task->priority], task) != 0) {
                 LOG_ERROR("Failed to add task to ready list");
                 return -1;
             }
             break;
             
         case TASK_STATE_BLOCKED:
             if (list_append(&blocked_list, task) != 0) {
                 LOG_ERROR("Failed to add task to blocked list");
                 return -1;
             }
             break;
             
         case TASK_STATE_SUSPENDED:
             if (list_append(&suspended_list, task) != 0) {
                 LOG_ERROR("Failed to add task to suspended list");
                 return -1;
             }
             break;
             
         case TASK_STATE_RUNNING:
         case TASK_STATE_TERMINATED:
             LOG_ERROR("Invalid task state for adding to scheduler");
             return -1;
     }
     
     stats.tasks_created++;
     return 0;
 }
 
 /**
  * Remove a task from the scheduler
  */
 int scheduler_remove_task(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Remove from appropriate list based on state */
     switch (task->state) {
         case TASK_STATE_READY:
             if (list_remove(&ready_lists[task->priority], (list_node_t*)task, 0) != 0) {
                 LOG_ERROR("Failed to remove task from ready list");
                 return -1;
             }
             break;
             
         case TASK_STATE_BLOCKED:
             if (list_remove(&blocked_list, (list_node_t*)task, 0) != 0) {
                 LOG_ERROR("Failed to remove task from blocked list");
                 return -1;
             }
             break;
             
         case TASK_STATE_SUSPENDED:
             if (list_remove(&suspended_list, (list_node_t*)task, 0) != 0) {
                 LOG_ERROR("Failed to remove task from suspended list");
                 return -1;
             }
             break;
             
         case TASK_STATE_RUNNING:
             /* Can't remove running task */
             LOG_ERROR("Cannot remove running task");
             return -1;
             
         case TASK_STATE_TERMINATED:
             /* Task already terminated, nothing to do */
             break;
     }
     
     stats.tasks_deleted++;
     return 0;
 }
 
 /**
  * Get the next task to run according to the scheduling policy
  */
 task_t* scheduler_get_next_task(void) {
     task_t* next_task = NULL;
     
     /* Increment scheduler invocations counter */
     stats.scheduler_invocations++;
     
     /* Check if scheduler is locked */
     if (scheduler_lock_count > 0) {
         task_t* current = task_get_current();
         if (current != NULL && current->state == TASK_STATE_RUNNING) {
             return current;  /* Keep running current task */
         }
     }
     
     /* Select based on scheduling policy */
     switch (current_policy) {
         case SCHEDULING_POLICY_PRIORITY:
             /* Find highest priority task that is ready */
             for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
                 if (!list_is_empty(&ready_lists[i])) {
                     next_task = (task_t*)list_head(&ready_lists[i])->data;
                     break;
                 }
             }
             break;
             
         case SCHEDULING_POLICY_RR:
             /* Round robin within priority */
             for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
                 if (!list_is_empty(&ready_lists[i])) {
                     /* Get first task */
                     next_task = (task_t*)list_head(&ready_lists[i])->data;
                     
                     /* Move to end of list (round robin) */
                     list_remove(&ready_lists[i], (list_node_t*)next_task, 0);
                     list_append(&ready_lists[i], next_task);
                     
                     break;
                 }
             }
             break;
             
         case SCHEDULING_POLICY_EDF:
             {
                 /* Earliest deadline first */
                 task_t* earliest = NULL;
                 uint32_t earliest_deadline = UINT32_MAX;
                 
                 /* Search all ready lists for earliest deadline */
                 for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
                     if (!list_is_empty(&ready_lists[i])) {
                         list_node_t* node = list_head(&ready_lists[i]);
                         while (node != NULL) {
                             task_t* task = (task_t*)node->data;
                             
                             /* Only consider periodic tasks with deadlines */
                             if (task->period > 0 && task->absolute_deadline < earliest_deadline) {
                                 earliest = task;
                                 earliest_deadline = task->absolute_deadline;
                             }
                             
                             node = node->next;
                         }
                     }
                 }
                 
                 /* If we found a task with deadline, use it */
                 if (earliest != NULL) {
                     next_task = earliest;
                 } else {
                     /* Fall back to priority scheduling for non-periodic tasks */
                     for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
                         if (!list_is_empty(&ready_lists[i])) {
                             next_task = (task_t*)list_head(&ready_lists[i])->data;
                             break;
                         }
                     }
                 }
             }
             break;
             
         case SCHEDULING_POLICY_RMS:
             /* Rate monotonic - priority based on period (shorter period = higher priority) */
             /* For this simulation, we just use the configured priorities */
             for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
                 if (!list_is_empty(&ready_lists[i])) {
                     next_task = (task_t*)list_head(&ready_lists[i])->data;
                     break;
                 }
             }
             break;
             
         default:
             LOG_ERROR("Unknown scheduling policy: %d", current_policy);
             break;
     }
     
     /* If no task is ready, use idle task */
     if (next_task == NULL) {
         next_task = task_get_idle();
     }
     
     return next_task;
 }
 
 /**
  * Notify the scheduler that the current task is blocked
  */
 int scheduler_block_task(task_t* task, block_reason_t reason, void* block_object) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Cannot block idle task */
     if (task == task_get_idle()) {
         LOG_ERROR("Cannot block idle task");
         return -1;
     }
     
     /* Set block reason and object */
     task->block_reason = reason;
     task->block_object = block_object;
     
     /* Update task state */
     task->state = TASK_STATE_BLOCKED;
     
     /* Remove from ready list */
     if (list_remove(&ready_lists[task->priority], (list_node_t*)task, 0) != 0) {
         LOG_ERROR("Failed to remove task from ready list");
         return -1;
     }
     
     /* Add to blocked list */
     if (list_append(&blocked_list, task) != 0) {
         LOG_ERROR("Failed to add task to blocked list");
         return -1;
     }
     
     return 0;
 }
 
 /**
  * Notify the scheduler that a task is unblocked
  */
 int scheduler_unblock_task(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Only unblock if actually blocked */
     if (task->state != TASK_STATE_BLOCKED) {
         LOG_WARNING("Task '%s' is not blocked", task->name);
         return 0;
     }
     
     /* Clear block reason and object */
     task->block_reason = BLOCK_REASON_NONE;
     task->block_object = NULL;
     
     /* Update task state */
     task->state = TASK_STATE_READY;
     
     /* Remove from blocked list */
     if (list_remove(&blocked_list, (list_node_t*)task, 0) != 0) {
         LOG_ERROR("Failed to remove task from blocked list");
         return -1;
     }
     
     /* Add to ready list */
     if (list_append(&ready_lists[task->priority], task) != 0) {
         LOG_ERROR("Failed to add task to ready list");
         return -1;
     }
     
     return 0;
 }
 
 /**
  * Perform a context switch
  */
 int scheduler_context_switch(void) {
     task_t* current = task_get_current();
     task_t* next = NULL;
     
     /* Check if scheduler is running */
     if (scheduler_state != SCHEDULER_RUNNING) {
         LOG_ERROR("Scheduler not running");
         return -1;
     }
     
     /* Check if scheduler is locked */
     if (scheduler_lock_count > 0) {
         return 0;  /* Skip context switch while locked */
     }
     
     /* Get next task to run */
     next = scheduler_get_next_task();
     if (next == NULL) {
         LOG_ERROR("No tasks ready to run");
         return -1;
     }
     
     /* Don't switch if it's the same task */
     if (current == next) {
         return 0;
     }
     
     /* Update statistics */
     stats.context_switches++;
     
     /* If current task is running, update its state */
     if (current != NULL && current->state == TASK_STATE_RUNNING) {
         /* Check if time slice expired (for round robin) */
         if (current_policy == SCHEDULING_POLICY_RR && 
             --current->time_slice_count == 0) {
             
             /* Reset time slice counter */
             current->time_slice_count = current->time_slice;
             
             /* Mark as ready and put at end of ready list */
             current->state = TASK_STATE_READY;
             list_append(&ready_lists[current->priority], current);
         } else {
             /* Otherwise just mark as ready and leave in ready list */
             current->state = TASK_STATE_READY;
             if (list_append(&ready_lists[current->priority], current) != 0) {
                 LOG_ERROR("Failed to add current task back to ready list");
             }
         }
         
         /* Update runtime statistics */
         uint32_t now = time_get_ticks();
         uint32_t runtime = now - current->stats.last_start_time;
         current->stats.total_runtime += runtime;
         
         /* Check if this is the longest execution time */
         if (runtime > current->stats.max_execution_time) {
             current->stats.max_execution_time = runtime;
         }
     }
     
     /* Update next task state */
     next->state = TASK_STATE_RUNNING;
     
     /* Remove from ready list */
     if (list_remove(&ready_lists[next->priority], (list_node_t*)next, 0) != 0) {
         LOG_ERROR("Failed to remove next task from ready list");
     }
     
     /* Update task statistics */
     next->stats.last_start_time = time_get_ticks();
     next->stats.num_activations++;
     
     /* Update current task */
     task_set_current(next);
     
     /* If both current and next are valid, perform context switch */
     if (current != NULL) {
         context_switch(current, next);
     } else {
         /* Just start next task (no context saving needed) */
         context_start_first_task(next);
     }
     
     return 0;
 }
 
 /**
  * Update task state in the scheduler
  */
 int scheduler_update_task_state(task_t* task, task_state_t new_state) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* No change needed if state is the same */
     if (task->state == new_state) {
         return 0;
     }
     
     /* Handle based on old state */
     switch (task->state) {
         case TASK_STATE_READY:
             /* Remove from ready list */
             if (list_remove(&ready_lists[task->priority], (list_node_t*)task, 0) != 0) {
                 LOG_ERROR("Failed to remove task from ready list");
                 return -1;
             }
             break;
             
         case TASK_STATE_RUNNING:
             /* Can't change running task state this way */
             LOG_ERROR("Cannot change running task state directly");
             return -1;
             
         case TASK_STATE_BLOCKED:
             /* Remove from blocked list */
             if (list_remove(&blocked_list, (list_node_t*)task, 0) != 0) {
                 LOG_ERROR("Failed to remove task from blocked list");
                 return -1;
             }
             break;
             
         case TASK_STATE_SUSPENDED:
             /* Remove from suspended list */
             if (list_remove(&suspended_list, (list_node_t*)task, 0) != 0) {
                 LOG_ERROR("Failed to remove task from suspended list");
                 return -1;
             }
             break;
             
         case TASK_STATE_TERMINATED:
             /* Can't change terminated task state */
             LOG_ERROR("Cannot change terminated task state");
             return -1;
     }
     
     /* Update task state */
     task->state = new_state;
     
     /* Handle based on new state */
     switch (new_state) {
         case TASK_STATE_READY:
             /* Add to ready list */
             if (list_append(&ready_lists[task->priority], task) != 0) {
                 LOG_ERROR("Failed to add task to ready list");
                 return -1;
             }
             break;
             
         case TASK_STATE_RUNNING:
             /* Can't set to running state this way */
             LOG_ERROR("Cannot set task state to running directly");
             return -1;
             
         case TASK_STATE_BLOCKED:
             /* Add to blocked list */
             if (list_append(&blocked_list, task) != 0) {
                 LOG_ERROR("Failed to add task to blocked list");
                 return -1;
             }
             break;
             
         case TASK_STATE_SUSPENDED:
             /* Add to suspended list */
             if (list_append(&suspended_list, task) != 0) {
                 LOG_ERROR("Failed to add task to suspended list");
                 return -1;
             }
             break;
             
         case TASK_STATE_TERMINATED:
             /* No list for terminated tasks */
             break;
     }
     
     return 0;
 }
 
 /**
  * Process system tick
  */
 int scheduler_tick(void) {
     task_t* current = task_get_current();
     int unblocked_count = 0;
     
     /* Check if scheduler is running */
     if (scheduler_state != SCHEDULER_RUNNING) {
         return 0;
     }
     
     /* Update system time */
     stats.system_time++;
     
     /* Update idle time if idle task is running */
     if (current == task_get_idle()) {
         stats.idle_time++;
     }
     
     /* Check for timed delays that have expired */
     list_node_t* node = list_head(&blocked_list);
     list_node_t* next = NULL;
     
     while (node != NULL) {
         task_t* task = (task_t*)node->data;
         next = node->next;  /* Save next pointer in case we remove this node */
         
         /* Check if blocked on delay and delay has expired */
         if (task->block_reason == BLOCK_REASON_DELAY &&
             time_get_ticks() >= task->delay_until) {
             
             /* Unblock the task */
             if (scheduler_unblock_task(task) == 0) {
                 unblocked_count++;
             }
         }
         
         node = next;
     }
     
     /* Check for periodic tasks that need to be released */
     for (int i = 0; i < MAX_TASKS; i++) {
         task_t* task = task_list[i];
         
         if (task != NULL && task->period > 0) {
             uint32_t current_time = time_get_ticks();
             
             /* Check if it's time for next period */
             if (current_time >= task->next_release) {
                 /* Check if previous job met deadline */
                 if (task->state != TASK_STATE_READY &&
                     task->state != TASK_STATE_RUNNING &&
                     current_time > task->absolute_deadline) {
                     
                     /* Deadline missed */
                     task->stats.deadline_misses++;
                     stats.deadline_misses++;
                     
                     LOG_WARNING("Task '%s' missed deadline (abs=%u, now=%u)",
                                 task->name, task->absolute_deadline, current_time);
                 }
                 
                 /* Calculate next release time and deadline */
                 task->next_release += task->period;
                 task->absolute_deadline = task->next_release + task->deadline;
                 
                 /* If task is blocked or suspended, unblock it */
                 if (task->state == TASK_STATE_BLOCKED) {
                     scheduler_unblock_task(task);
                     unblocked_count++;
                 } else if (task->state == TASK_STATE_SUSPENDED) {
                     task->state = TASK_STATE_READY;
                     list_remove(&suspended_list, (list_node_t*)task, 0);
                     list_append(&ready_lists[task->priority], task);
                     unblocked_count++;
                 }
                 
                 LOG_DEBUG("Released periodic task '%s' (next=%u, deadline=%u)",
                           task->name, task->next_release, task->absolute_deadline);
             }
         }
     }
     
     /* If any tasks were unblocked, trigger scheduler */
     if (unblocked_count > 0) {
         /* Only trigger context switch if scheduler isn't locked */
         if (scheduler_lock_count == 0) {
             scheduler_context_switch();
         }
     }
     
     /* If using round-robin policy, check time slice */
     if (current_policy == SCHEDULING_POLICY_RR && 
         current != NULL && 
         current != task_get_idle()) {
         
         /* Decrement time slice counter */
         if (--current->time_slice_count == 0) {
             /* Time slice expired, reset counter */
             current->time_slice_count = current->time_slice;
             
             /* Trigger context switch if scheduler isn't locked */
             if (scheduler_lock_count == 0) {
                 scheduler_context_switch();
             }
         }
     }
     
     return unblocked_count;
 }
 
 /**
  * Get scheduler statistics
  */
 int scheduler_get_stats(scheduler_stats_t* out_stats) {
     if (out_stats == NULL) {
         LOG_ERROR("NULL stats pointer");
         return -1;
     }
     
     /* Calculate CPU load */
     stats.cpu_load = 1.0f - ((float)stats.idle_time / (float)stats.system_time);
     if (stats.cpu_load < 0.0f) {
         stats.cpu_load = 0.0f;
     } else if (stats.cpu_load > 1.0f) {
         stats.cpu_load = 1.0f;
     }
     
     /* Copy statistics */
     memcpy(out_stats, &stats, sizeof(scheduler_stats_t));
     
     return 0;
 }
 
 /**
  * Reset scheduler statistics
  */
 int scheduler_reset_stats(void) {
     /* Keep track of system time and tasks created/deleted */
     uint32_t system_time = stats.system_time;
     uint32_t tasks_created = stats.tasks_created;
     uint32_t tasks_deleted = stats.tasks_deleted;
     
     /* Reset all other stats */
     memset(&stats, 0, sizeof(scheduler_stats_t));
     
     /* Restore preserved values */
     stats.system_time = system_time;
     stats.tasks_created = tasks_created;
     stats.tasks_deleted = tasks_deleted;
     
     return 0;
 }
 
 /**
  * Set scheduling policy
  */
 int scheduler_set_policy(uint8_t policy) {
     /* Validate policy */
     if (policy != SCHEDULING_POLICY_PRIORITY && 
         policy != SCHEDULING_POLICY_RR && 
         policy != SCHEDULING_POLICY_EDF && 
         policy != SCHEDULING_POLICY_RMS) {
         
         LOG_ERROR("Invalid scheduling policy: %d", policy);
         return -1;
     }
     
     LOG_INFO("Changing scheduling policy from %s to %s",
              scheduler_policy_to_string(current_policy),
              scheduler_policy_to_string(policy));
     
     /* Set new policy */
     current_policy = policy;
     
     return 0;
 }
 
 /**
  * Get current scheduling policy
  */
 uint8_t scheduler_get_policy(void) {
     return current_policy;
 }
 
 /**
  * Check for and handle deadline misses
  */
 int scheduler_check_deadlines(void) {
     int missed_count = 0;
     uint32_t current_time = time_get_ticks();
     
     /* Check all tasks for deadline misses */
     for (int i = 0; i < MAX_TASKS; i++) {
         task_t* task = task_list[i];
         
         if (task != NULL && task->period > 0) {
             /* Only check tasks with active deadlines */
             if (task->absolute_deadline > 0 && 
                 current_time > task->absolute_deadline) {
                 
                 /* Deadline missed */
                 if (task->state != TASK_STATE_TERMINATED) {
                     task->stats.deadline_misses++;
                     stats.deadline_misses++;
                     missed_count++;
                     
                     LOG_WARNING("Task '%s' missed deadline (abs=%u, now=%u)",
                                 task->name, task->absolute_deadline, current_time);
                 }
             }
         }
     }
     
     return missed_count;
 }
 
 /**
  * Lock scheduler (prevent context switches)
  */
 int scheduler_lock(void) {
     uint32_t prev_state = context_enter_critical();
     scheduler_lock_count++;
     context_exit_critical(prev_state);
     return 0;
 }
 
 /**
  * Unlock scheduler (allow context switches)
  */
 int scheduler_unlock(void) {
     uint32_t prev_state = context_enter_critical();
     
     if (scheduler_lock_count > 0) {
         scheduler_lock_count--;
     }
     
     /* If count reaches 0, trigger context switch */
     if (scheduler_lock_count == 0) {
         context_exit_critical(prev_state);
         scheduler_context_switch();
     } else {
         context_exit_critical(prev_state);
     }
     
     return 0;
 }
 
 /**
  * Get string name for scheduling policy
  */
 const char* scheduler_policy_to_string(uint8_t policy) {
     switch (policy) {
         case SCHEDULING_POLICY_PRIORITY:
             return "Priority";
         case SCHEDULING_POLICY_RR:
             return "Round Robin";
         case SCHEDULING_POLICY_EDF:
             return "Earliest Deadline First";
         case SCHEDULING_POLICY_RMS:
             return "Rate Monotonic";
         default:
             return "Unknown";
     }
 }