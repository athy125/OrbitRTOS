/**
 * Send a message to a queue
 */
 int queue_send(queue_t* queue, const void* msg, uint32_t timeout) {
    if (queue == NULL || msg == NULL) {
        LOG_ERROR("Invalid parameters");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Check if queue is full */
    if (queue->count >= queue->capacity) {
        /* If there are tasks waiting to receive, wake one up and send directly */
        if (queue->waiting_recv != NULL) {
            /* Get first waiting task */
            task_t* task = queue->waiting_recv;
            
            /* Remove from waiting list */
            queue->waiting_recv = task->next;
            if (queue->waiting_recv != NULL) {
                queue->waiting_recv->prev = NULL;
            }
            
            /* Clear task's next/prev pointers */
            task->next = NULL;
            task->prev = NULL;
            
            /* Copy message directly to waiting task's buffer */
            void* task_buffer = task->block_object;
            if (task_buffer != NULL) {
                memcpy(task_buffer, msg, queue->msg_size);
            }
            
            /* Unblock the task */
            scheduler_unblock_task(task);
            
            /* Exit critical section */
            context_exit_critical(prev_state);
            
            /* Trigger scheduler to check if unblocked task should run */
            scheduler_context_switch();
            
            return 0;
        }
        
        /* Queue is full and no tasks waiting, handle timeout */
        if (timeout == 0) {
            context_exit_critical(prev_state);
            return -1;  /* Timeout */
        }
        
        /* Block task */
        task_t* current = task_get_current();
        if (current == NULL) {
            LOG_ERROR("No current task");
            context_exit_critical(prev_state);
            return -1;
        }
        
        /* Store message pointer for later */
        current->block_object = (void*)msg;
        
        /* Set up delay if timeout is not infinite */
        if (timeout != MAX_TIMEOUT) {
            current->delay_until = time_get_ticks() + timeout;
        }
        
        /* Add task to queue's waiting send list */
        current->next = queue->waiting_send;
        if (queue->waiting_send != NULL) {
            queue->waiting_send->prev = current;
        }
        queue->waiting_send = current;
        
        /* Block the task */
        int result = scheduler_block_task(current, BLOCK_REASON_QUEUE_FULL, queue);
        if (result != 0) {
            LOG_ERROR("Failed to block task");
            context_exit_critical(prev_state);
            return -1;
        }
        
        /* Exit critical section */
        context_exit_critical(prev_state);
        
        /* Trigger context switch */
        scheduler_context_switch();
        
        /* When we get here, either space became available or timeout occurred */
        
        /* Check if we sent the message */
        if (current->block_reason != BLOCK_REASON_NONE) {
            /* Timeout occurred */
            return -1;
        }
        
        return 0;
    }
    
    /* Queue not full, add message */
    uint8_t* buffer = (uint8_t*)queue->buffer;
    memcpy(buffer + (queue->tail * queue->msg_size), msg, queue->msg_size);
    
    /* Update tail pointer */
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    return 0;
}

/**
 * Receive a message from a queue
 */
int queue_receive(queue_t* queue, void* msg, uint32_t timeout) {
    if (queue == NULL || msg == NULL) {
        LOG_ERROR("Invalid parameters");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Check if queue is empty */
    if (queue->count == 0) {
        /* If there are tasks waiting to send, receive directly from one */
        if (queue->waiting_send != NULL) {
            /* Get first waiting task */
            task_t* task = queue->waiting_send;
            
            /* Remove from waiting list */
            queue->waiting_send = task->next;
            if (queue->waiting_send != NULL) {
                queue->waiting_send->prev = NULL;
            }
            
            /* Clear task's next/prev pointers */
            task->next = NULL;
            task->prev = NULL;
            
            /* Copy message directly from waiting task's buffer */
            void* task_buffer = task->block_object;
            if (task_buffer != NULL) {
                memcpy(msg, task_buffer, queue->msg_size);
            }
            
            /* Unblock the task */
            scheduler_unblock_task(task);
            
            /* Exit critical section */
            context_exit_critical(prev_state);
            
            /* Trigger scheduler to check if unblocked task should run */
            scheduler_context_switch();
            
            return 0;
        }
        
        /* Queue is empty and no tasks waiting, handle timeout */
        if (timeout == 0) {
            context_exit_critical(prev_state);
            return -1;  /* Timeout */
        }
        
        /* Block task */
        task_t* current = task_get_current();
        if (current == NULL) {
            LOG_ERROR("No current task");
            context_exit_critical(prev_state);
            return -1;
        }
        
        /* Store message buffer for later */
        current->block_object = msg;
        
        /* Set up delay if timeout is not infinite */
        if (timeout != MAX_TIMEOUT) {
            current->delay_until = time_get_ticks() + timeout;
        }
        
        /* Add task to queue's waiting receive list */
        current->next = queue->waiting_recv;
        if (queue->waiting_recv != NULL) {
            queue->waiting_recv->prev = current;
        }
        queue->waiting_recv = current;
        
        /* Block the task */
        int result = scheduler_block_task(current, BLOCK_REASON_QUEUE_EMPTY, queue);
        if (result != 0) {
            LOG_ERROR("Failed to block task");
            context_exit_critical(prev_state);
            return -1;
        }
        
        /* Exit critical section */
        context_exit_critical(prev_state);
        
        /* Trigger context switch */
        scheduler_context_switch();
        
        /* When we get here, either a message arrived or timeout occurred */
        
        /* Check if we received a message */
        if (current->block_reason != BLOCK_REASON_NONE) {
            /* Timeout occurred */
            return -1;
        }
        
        return 0;
    }
    
    /* Queue not empty, get message */
    uint8_t* buffer = (uint8_t*)queue->buffer;
    memcpy(msg, buffer + (queue->head * queue->msg_size), queue->msg_size);
    
    /* Update head pointer */
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    /* Check if any tasks are waiting to send */
    if (queue->waiting_send != NULL) {
        /* Get first waiting task */
        task_t* task = queue->waiting_send;
        
        /* Remove from waiting list */
        queue->waiting_send = task->next;
        if (queue->waiting_send != NULL) {
            queue->waiting_send->prev = NULL;
        }
        
        /* Clear task's next/prev pointers */
        task->next = NULL;
        task->prev = NULL;
        
        /* Get message from waiting task and add to queue */
        void* task_buffer = task->block_object;
        if (task_buffer != NULL) {
            uint8_t* buffer = (uint8_t*)queue->buffer;
            memcpy(buffer + (queue->tail * queue->msg_size), task_buffer, queue->msg_size);
            
            /* Update tail pointer */
            queue->tail = (queue->tail + 1) % queue->capacity;
            queue->count++;
        }
        
        /* Unblock the task */
        scheduler_unblock_task(task);
        
        /* Exit critical section */
        context_exit_critical(prev_state);
        
        /* Trigger scheduler to check if unblocked task should run */
        scheduler_context_switch();
        
        return 0;
    }
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    return 0;
}

/**
 * Get number of messages in queue
 */
uint32_t queue_get_count(queue_t* queue) {
    if (queue == NULL) {
        LOG_ERROR("NULL queue pointer");
        return 0;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    uint32_t count = queue->count;
    context_exit_critical(prev_state);
    
    return count;
}

/**
 * Peek at the next message without removing it
 */
int queue_peek(queue_t* queue, void* msg) {
    if (queue == NULL || msg == NULL) {
        LOG_ERROR("Invalid parameters");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Check if queue is empty */
    if (queue->count == 0) {
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Get message without removing */
    uint8_t* buffer = (uint8_t*)queue->buffer;
    memcpy(msg, buffer + (queue->head * queue->msg_size), queue->msg_size);
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    return 0;
}

/* Event group functions */

/**
 * Create an event group
 */
event_group_t* event_group_create(const char* name) {
    event_group_t* group = NULL;
    int index = -1;
    
    /* Validate parameters */
    if (name == NULL) {
        LOG_ERROR("NULL event group name");
        return NULL;
    }
    
    /* Find free slot */
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!event_group_used[i]) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("No free event group slots");
        return NULL;
    }
    
    /* Initialize event group */
    group = &event_groups[index];
    group->flags = 0;
    group->waiting_tasks = NULL;
    strncpy(group->name, name, MAX_TASK_NAME_LEN - 1);
    group->name[MAX_TASK_NAME_LEN - 1] = '\0';
    
    /* Mark as used */
    event_group_used[index] = 1;
    
    LOG_INFO("Created event group '%s'", group->name);
    
    return group;
}

/**
 * Delete an event group
 */
int event_group_delete(event_group_t* group) {
    if (group == NULL) {
        LOG_ERROR("NULL event group pointer");
        return -1;
    }
    
    /* Find index of this event group */
    int index = -1;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (&event_groups[i] == group) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("Invalid event group pointer");
        return -1;
    }
    
    /* Check if any tasks are waiting */
    if (group->waiting_tasks != NULL) {
        LOG_WARNING("Deleting event group '%s' with waiting tasks", group->name);
        
        /* Unblock all waiting tasks with error */
        task_t* task = group->waiting_tasks;
        while (task != NULL) {
            task_t* next = task->next;
            scheduler_unblock_task(task);
            task = next;
        }
    }
    
    /* Mark as unused */
    event_group_used[index] = 0;
    
    LOG_INFO("Deleted event group '%s'", group->name);
    return 0;
}

/**
 * Set flags in an event group
 */
uint32_t event_group_set_flags(event_group_t* group, uint32_t flags) {
    if (group == NULL) {
        LOG_ERROR("NULL event group pointer");
        return 0;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Save previous flags */
    uint32_t prev_flags = group->flags;
    
    /* Set flags */
    group->flags |= flags;
    
    /* Check if any tasks can be unblocked */
    task_t* task = group->waiting_tasks;
    task_t* next = NULL;
    
    while (task != NULL) {
        next = task->next; /* Save next pointer in case we remove this task */
        
        /* Get wait condition from task's block object */
        uint32_t wait_flags = (uint32_t)task->block_object;
        uint8_t options = wait_flags >> 24;
        wait_flags &= 0x00FFFFFF;
        
        /* Check if condition is met */
        int condition_met = 0;
        
        if ((options & EVENT_WAIT_ALL) != 0) {
            /* Wait for all flags */
            if ((group->flags & wait_flags) == wait_flags) {
                condition_met = 1;
            }
        } else {
            /* Wait for any flags */
            if ((group->flags & wait_flags) != 0) {
                condition_met = 1;
            }
        }
        
        if (condition_met) {
            /* Condition met, unblock the task */
            
            /* Clear flags if requested */
            if ((options & EVENT_CLEAR) != 0) {
                group->flags &= ~wait_flags;
            }
            
            /* Remove from waiting list */
            if (task->prev != NULL) {
                task->prev->next = task->next;
            } else {
                group->waiting_tasks = task->next;
            }
            
            if (task->next != NULL) {
                task->next->prev = task->prev;
            }
            
            /* Clear task's next/prev pointers */
            task->next = NULL;
            task->prev = NULL;
            
            /* Unblock the task */
            scheduler_unblock_task(task);
        }
        
        task = next;
    }
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    /* Trigger scheduler to check if unblocked tasks should run */
    if (prev_flags != group->flags) {
        scheduler_context_switch();
    }
    
    return prev_flags;
}

/**
 * Clear flags in an event group
 */
uint32_t event_group_clear_flags(event_group_t* group, uint32_t flags) {
    if (group == NULL) {
        LOG_ERROR("NULL event group pointer");
        return 0;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Save previous flags */
    uint32_t prev_flags = group->flags;
    
    /* Clear flags */
    group->flags &= ~flags;
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    return prev_flags;
}

/**
 * Wait for flags in an event group
 */
uint32_t event_group_wait(event_group_t* group, uint32_t flags, uint8_t options, uint32_t timeout) {
    if (group == NULL || flags == 0) {
        LOG_ERROR("Invalid parameters");
        return 0;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Check if condition already met */
    int condition_met = 0;
    
    if ((options & EVENT_WAIT_ALL) != 0) {
        /* Wait for all flags */
        if ((group->flags & flags) == flags) {
            condition_met = 1;
        }
    } else {
        /* Wait for any flags */
        if ((group->flags & flags) != 0) {
            condition_met = 1;
        }
    }
    
    if (condition_met) {
        /* Condition already met */
        uint32_t return_flags = group->flags & flags;
        
        /* Clear flags if requested */
        if ((options & EVENT_CLEAR) != 0) {
            group->flags &= ~flags;
        }
        
        /* Exit critical section */
        context_exit_critical(prev_state);
        
        return return_flags;
    }
    
    /* Condition not met */
    
    /* If timeout is 0, return immediately */
    if (timeout == 0) {
        context_exit_critical(prev_state);
        return 0;
    }
    
    /* Block task */
    task_t* current = task_get_current();
    if (current == NULL) {
        LOG_ERROR("No current task");
        context_exit_critical(prev_state);
        return 0;
    }
    
    /* Store wait flags and options in block object */
    /* Use high byte of flags for options */
    current->block_object = (void*)((uint32_t)flags | ((uint32_t)options << 24));
    
    /* Set up delay if timeout is not infinite */
    if (timeout != MAX_TIMEOUT) {
        current->delay_until = time_get_ticks() + timeout;
    }
    
    /* Add task to event group's waiting list */
    current->next = group->waiting_tasks;
    if (group->waiting_tasks != NULL) {
        group->waiting_tasks->prev = current;
    }
    group->waiting_tasks = current;
    
    /* Block the task */
    int result = scheduler_block_task(current, BLOCK_REASON_EVENT, group);
    if (result != 0) {
        LOG_ERROR("Failed to block task");
        context_exit_critical(prev_state);
        return 0;
    }
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    /* Trigger context switch */
    scheduler_context_switch();
    
    /* When we get here, either flags were set or timeout occurred */
    
    /* Check if we were unblocked due to flags */
    if (current->block_reason != BLOCK_REASON_NONE) {
        /* Timeout occurred */
        return 0;
    }
    
    /* We were unblocked due to flags, return matched flags */
    return group->flags & flags;
}

/**
 * Get current flags in event group
 */
uint32_t event_group_get_flags(event_group_t* group) {
    if (group == NULL) {
        LOG_ERROR("NULL event group pointer");
        return 0;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    uint32_t flags = group->flags;
    context_exit_critical(prev_state);
    
    return flags;
}/**
 * @file ipc.c
 * @brief Implementation of Inter-Process Communication mechanisms
 */

#include <stdlib.h>
#include <string.h>
#include "../../include/kernel/ipc.h"
#include "../../include/kernel/task.h"
#include "../../include/kernel/scheduler.h"
#include "../../include/kernel/context.h"
#include "../../include/kernel/time.h"
#include "../../include/utils/logger.h"
#include "../../include/utils/list.h"
#include "../../include/config.h"

/* Arrays of IPC objects */
static semaphore_t semaphores[MAX_SEMAPHORES];
static mutex_t mutexes[MAX_SEMAPHORES];  /* Reuse same limit */
static queue_t queues[MAX_QUEUES];
static event_group_t event_groups[MAX_SEMAPHORES];  /* Reuse same limit */

/* Track usage of IPC objects */
static uint8_t semaphore_used[MAX_SEMAPHORES];
static uint8_t mutex_used[MAX_SEMAPHORES];
static uint8_t queue_used[MAX_QUEUES];
static uint8_t event_group_used[MAX_SEMAPHORES];

/**
 * Initialize the IPC subsystem
 */
int ipc_init(void) {
    LOG_INFO("Initializing IPC subsystem");
    
    /* Initialize all usage flags to 0 (unused) */
    memset(semaphore_used, 0, sizeof(semaphore_used));
    memset(mutex_used, 0, sizeof(mutex_used));
    memset(queue_used, 0, sizeof(queue_used));
    memset(event_group_used, 0, sizeof(event_group_used));
    
    LOG_INFO("IPC subsystem initialized");
    return 0;
}

/* Semaphore functions */

/**
 * Create a semaphore
 */
semaphore_t* semaphore_create(const char* name, uint32_t initial_count, uint32_t max_count) {
    semaphore_t* sem = NULL;
    int index = -1;
    
    /* Validate parameters */
    if (name == NULL || max_count == 0 || initial_count > max_count) {
        LOG_ERROR("Invalid semaphore parameters");
        return NULL;
    }
    
    /* Find free slot */
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphore_used[i]) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("No free semaphore slots");
        return NULL;
    }
    
    /* Initialize semaphore */
    sem = &semaphores[index];
    sem->count = initial_count;
    sem->max_count = max_count;
    sem->waiting_tasks = NULL;
    strncpy(sem->name, name, MAX_TASK_NAME_LEN - 1);
    sem->name[MAX_TASK_NAME_LEN - 1] = '\0';
    
    /* Mark as used */
    semaphore_used[index] = 1;
    
    LOG_INFO("Created semaphore '%s' (count=%u, max=%u)",
             sem->name, initial_count, max_count);
    
    return sem;
}

/**
 * Delete a semaphore
 */
int semaphore_delete(semaphore_t* sem) {
    if (sem == NULL) {
        LOG_ERROR("NULL semaphore pointer");
        return -1;
    }
    
    /* Find index of this semaphore */
    int index = -1;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (&semaphores[i] == sem) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("Invalid semaphore pointer");
        return -1;
    }
    
    /* Check if any tasks are waiting */
    if (sem->waiting_tasks != NULL) {
        LOG_WARNING("Deleting semaphore '%s' with waiting tasks", sem->name);
        
        /* Unblock all waiting tasks with error */
        task_t* task = sem->waiting_tasks;
        while (task != NULL) {
            task_t* next = task->next;
            scheduler_unblock_task(task);
            task = next;
        }
    }
    
    /* Mark as unused */
    semaphore_used[index] = 0;
    
    LOG_INFO("Deleted semaphore '%s'", sem->name);
    return 0;
}

/**
 * Take (acquire) a semaphore
 */
int semaphore_take(semaphore_t* sem, uint32_t timeout) {
    if (sem == NULL) {
        LOG_ERROR("NULL semaphore pointer");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Check if semaphore is available */
    if (sem->count > 0) {
        /* Semaphore available, decrement count */
        sem->count--;
        context_exit_critical(prev_state);
        return 0;
    }
    
    /* Semaphore not available */
    
    /* If timeout is 0, return immediately */
    if (timeout == 0) {
        context_exit_critical(prev_state);
        return -1;  /* Timeout */
    }
    
    /* Block task */
    task_t* current = task_get_current();
    if (current == NULL) {
        LOG_ERROR("No current task");
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Set up delay if timeout is not infinite */
    if (timeout != MAX_TIMEOUT) {
        current->delay_until = time_get_ticks() + timeout;
    }
    
    /* Add task to semaphore's waiting list */
    current->next = sem->waiting_tasks;
    if (sem->waiting_tasks != NULL) {
        sem->waiting_tasks->prev = current;
    }
    sem->waiting_tasks = current;
    
    /* Block the task */
    int result = scheduler_block_task(current, BLOCK_REASON_SEMAPHORE, sem);
    if (result != 0) {
        LOG_ERROR("Failed to block task");
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    /* Trigger context switch */
    scheduler_context_switch();
    
    /* When we get here, either the semaphore was given or timeout occurred */
    
    /* Check if we got the semaphore */
    if (current->block_reason != BLOCK_REASON_NONE) {
        /* Timeout occurred */
        return -1;
    }
    
    return 0;
}

/**
 * Give (release) a semaphore
 */
int semaphore_give(semaphore_t* sem) {
    if (sem == NULL) {
        LOG_ERROR("NULL semaphore pointer");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Check if at maximum count */
    if (sem->count >= sem->max_count) {
        LOG_WARNING("Semaphore '%s' already at maximum count", sem->name);
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Check if tasks are waiting */
    if (sem->waiting_tasks != NULL) {
        /* Unblock highest priority waiting task */
        task_t* task = sem->waiting_tasks;
        
        /* Remove from waiting list */
        sem->waiting_tasks = task->next;
        if (sem->waiting_tasks != NULL) {
            sem->waiting_tasks->prev = NULL;
        }
        
        /* Clear task's next/prev pointers */
        task->next = NULL;
        task->prev = NULL;
        
        /* Unblock the task */
        scheduler_unblock_task(task);
        
        /* Exit critical section */
        context_exit_critical(prev_state);
        
        /* Trigger scheduler to check if unblocked task should run */
        scheduler_context_switch();
        
        return 0;
    }
    
    /* No tasks waiting, increment count */
    sem->count++;
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    return 0;
}

/**
 * Get semaphore count
 */
uint32_t semaphore_get_count(semaphore_t* sem) {
    if (sem == NULL) {
        LOG_ERROR("NULL semaphore pointer");
        return 0;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    uint32_t count = sem->count;
    context_exit_critical(prev_state);
    
    return count;
}

/* Mutex functions */

/**
 * Create a mutex
 */
mutex_t* mutex_create(const char* name) {
    mutex_t* mutex = NULL;
    int index = -1;
    
    /* Validate parameters */
    if (name == NULL) {
        LOG_ERROR("NULL mutex name");
        return NULL;
    }
    
    /* Find free slot */
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!mutex_used[i]) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("No free mutex slots");
        return NULL;
    }
    
    /* Initialize mutex */
    mutex = &mutexes[index];
    mutex->locked = 0;
    mutex->owner = NULL;
    mutex->waiting_tasks = NULL;
    strncpy(mutex->name, name, MAX_TASK_NAME_LEN - 1);
    mutex->name[MAX_TASK_NAME_LEN - 1] = '\0';
    
    /* Mark as used */
    mutex_used[index] = 1;
    
    LOG_INFO("Created mutex '%s'", mutex->name);
    
    return mutex;
}

/**
 * Delete a mutex
 */
int mutex_delete(mutex_t* mutex) {
    if (mutex == NULL) {
        LOG_ERROR("NULL mutex pointer");
        return -1;
    }
    
    /* Find index of this mutex */
    int index = -1;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (&mutexes[i] == mutex) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("Invalid mutex pointer");
        return -1;
    }
    
    /* Check if mutex is locked */
    if (mutex->locked) {
        LOG_WARNING("Deleting locked mutex '%s'", mutex->name);
        
        /* Restore owner's priority if needed */
        if (mutex->owner != NULL && 
            mutex->owner->priority != mutex->owner->original_priority) {
            task_set_priority(mutex->owner, mutex->owner->original_priority);
        }
    }
    
    /* Check if any tasks are waiting */
    if (mutex->waiting_tasks != NULL) {
        LOG_WARNING("Deleting mutex '%s' with waiting tasks", mutex->name);
        
        /* Unblock all waiting tasks with error */
        task_t* task = mutex->waiting_tasks;
        while (task != NULL) {
            task_t* next = task->next;
            scheduler_unblock_task(task);
            task = next;
        }
    }
    
    /* Mark as unused */
    mutex_used[index] = 0;
    
    LOG_INFO("Deleted mutex '%s'", mutex->name);
    return 0;
}

/**
 * Lock a mutex
 */
int mutex_lock(mutex_t* mutex, uint32_t timeout) {
    if (mutex == NULL) {
        LOG_ERROR("NULL mutex pointer");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Get current task */
    task_t* current = task_get_current();
    if (current == NULL) {
        LOG_ERROR("No current task");
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Check if mutex is already locked by this task */
    if (mutex->locked && mutex->owner == current) {
        LOG_WARNING("Task '%s' attempting to lock mutex '%s' it already owns",
                   current->name, mutex->name);
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Check if mutex is available */
    if (!mutex->locked) {
        /* Mutex available, lock it */
        mutex->locked = 1;
        mutex->owner = current;
        context_exit_critical(prev_state);
        return 0;
    }
    
    /* Mutex not available */
    
    /* If timeout is 0, return immediately */
    if (timeout == 0) {
        context_exit_critical(prev_state);
        return -1;  /* Timeout */
    }
    
    /* Priority inheritance - boost owner's priority if needed */
    if (current->priority < mutex->owner->priority) {
        /* Current task has higher priority (lower value) */
        task_set_priority(mutex->owner, current->priority);
    }
    
    /* Set up delay if timeout is not infinite */
    if (timeout != MAX_TIMEOUT) {
        current->delay_until = time_get_ticks() + timeout;
    }
    
    /* Add task to mutex's waiting list */
    current->next = mutex->waiting_tasks;
    if (mutex->waiting_tasks != NULL) {
        mutex->waiting_tasks->prev = current;
    }
    mutex->waiting_tasks = current;
    
    /* Block the task */
    int result = scheduler_block_task(current, BLOCK_REASON_MUTEX, mutex);
    if (result != 0) {
        LOG_ERROR("Failed to block task");
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    /* Trigger context switch */
    scheduler_context_switch();
    
    /* When we get here, either the mutex was unlocked or timeout occurred */
    
    /* Check if we got the mutex */
    if (current->block_reason != BLOCK_REASON_NONE) {
        /* Timeout occurred */
        
        /* Remove from waiting list */
        uint32_t prev_state = context_enter_critical();
        
        if (current->prev != NULL) {
            current->prev->next = current->next;
        } else if (mutex->waiting_tasks == current) {
            mutex->waiting_tasks = current->next;
        }
        
        if (current->next != NULL) {
            current->next->prev = current->prev;
        }
        
        current->next = NULL;
        current->prev = NULL;
        
        context_exit_critical(prev_state);
        
        return -1;
    }
    
    return 0;
}

/**
 * Unlock a mutex
 */
int mutex_unlock(mutex_t* mutex) {
    if (mutex == NULL) {
        LOG_ERROR("NULL mutex pointer");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    
    /* Get current task */
    task_t* current = task_get_current();
    if (current == NULL) {
        LOG_ERROR("No current task");
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Check if mutex is locked */
    if (!mutex->locked) {
        LOG_WARNING("Attempting to unlock mutex '%s' that is not locked", mutex->name);
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Check if current task owns the mutex */
    if (mutex->owner != current) {
        LOG_WARNING("Task '%s' attempting to unlock mutex '%s' it doesn't own",
                   current->name, mutex->name);
        context_exit_critical(prev_state);
        return -1;
    }
    
    /* Restore owner's priority if it was boosted */
    if (current->priority != current->original_priority) {
        task_set_priority(current, current->original_priority);
    }
    
    /* Check if tasks are waiting */
    if (mutex->waiting_tasks != NULL) {
        /* Unblock highest priority waiting task */
        task_t* task = mutex->waiting_tasks;
        
        /* Find highest priority task (lowest priority value) */
        task_t* highest = task;
        task = task->next;
        
        while (task != NULL) {
            if (task->priority < highest->priority) {
                highest = task;
            }
            task = task->next;
        }
        
        /* Remove from waiting list */
        if (highest->prev != NULL) {
            highest->prev->next = highest->next;
        } else {
            mutex->waiting_tasks = highest->next;
        }
        
        if (highest->next != NULL) {
            highest->next->prev = highest->prev;
        }
        
        /* Clear task's next/prev pointers */
        highest->next = NULL;
        highest->prev = NULL;
        
        /* Set new owner */
        mutex->owner = highest;
        
        /* Unblock the task */
        scheduler_unblock_task(highest);
        
        /* Exit critical section */
        context_exit_critical(prev_state);
        
        /* Trigger scheduler to check if unblocked task should run */
        scheduler_context_switch();
        
        return 0;
    }
    
    /* No tasks waiting, unlock mutex */
    mutex->locked = 0;
    mutex->owner = NULL;
    
    /* Exit critical section */
    context_exit_critical(prev_state);
    
    return 0;
}

/**
 * Check if mutex is locked
 */
int mutex_is_locked(mutex_t* mutex) {
    if (mutex == NULL) {
        LOG_ERROR("NULL mutex pointer");
        return -1;
    }
    
    /* Enter critical section */
    uint32_t prev_state = context_enter_critical();
    int locked = mutex->locked;
    context_exit_critical(prev_state);
    
    return locked;
}

/* Message queue functions */

/**
 * Create a message queue
 */
queue_t* queue_create(const char* name, size_t msg_size, uint32_t capacity) {
    queue_t* queue = NULL;
    int index = -1;
    
    /* Validate parameters */
    if (name == NULL || msg_size == 0 || capacity == 0) {
        LOG_ERROR("Invalid queue parameters");
        return NULL;
    }
    
    /* Find free slot */
    for (int i = 0; i < MAX_QUEUES; i++) {
        if (!queue_used[i]) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("No free queue slots");
        return NULL;
    }
    
    /* Allocate buffer for queue messages */
    void* buffer = malloc(msg_size * capacity);
    if (buffer == NULL) {
        LOG_ERROR("Failed to allocate queue buffer");
        return NULL;
    }
    
    /* Initialize queue */
    queue = &queues[index];
    queue->buffer = buffer;
    queue->msg_size = msg_size;
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->waiting_send = NULL;
    queue->waiting_recv = NULL;
    strncpy(queue->name, name, MAX_TASK_NAME_LEN - 1);
    queue->name[MAX_TASK_NAME_LEN - 1] = '\0';
    
    /* Mark as used */
    queue_used[index] = 1;
    
    LOG_INFO("Created queue '%s' (size=%u, capacity=%u)",
             queue->name, msg_size, capacity);
    
    return queue;
}

/**
 * Delete a message queue
 */
int queue_delete(queue_t* queue) {
    if (queue == NULL) {
        LOG_ERROR("NULL queue pointer");
        return -1;
    }
    
    /* Find index of this queue */
    int index = -1;
    for (int i = 0; i < MAX_QUEUES; i++) {
        if (&queues[i] == queue) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LOG_ERROR("Invalid queue pointer");
        return -1;
    }
    
    /* Check if any tasks are waiting to send */
    if (queue->waiting_send != NULL) {
        LOG_WARNING("Deleting queue '%s' with tasks waiting to send", queue->name);
        
        /* Unblock all waiting tasks with error */
        task_t* task = queue->waiting_send;
        while (task != NULL) {
            task_t* next = task->next;
            scheduler_unblock_task(task);
            task = next;
        }
    }
    
    /* Check if any tasks are waiting to receive */
    if (queue->waiting_recv != NULL) {
        LOG_WARNING("Deleting queue '%s' with tasks waiting to receive", queue->name);
        
        /* Unblock all waiting tasks with error */
        task_t* task = queue->waiting_recv;
        while (task != NULL) {
            task_t* next = task->next;
            scheduler_unblock_task(task);
            task = next;
        }
    }
    
    /* Free buffer */
    if (queue->buffer != NULL) {
        free(queue->buffer);
    }
    
    /* Mark as unused */
    queue_used[index] = 0;
    
    LOG_INFO("Deleted queue '%s'", queue->name);
    return 0;
}