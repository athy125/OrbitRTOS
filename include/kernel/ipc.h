/**
 * @file ipc.h
 * @brief Inter-Process Communication mechanisms for RTOS
 *
 * This file defines the IPC mechanisms for the RTOS, including semaphores, 
 * message queues, and mutexes.
 */

 #ifndef IPC_H
 #define IPC_H
 
 #include <stdint.h>
 #include <stddef.h>
 #include "../config.h"
 #include "task.h"
 
 /* Semaphore structure */
 typedef struct {
     uint32_t count;              /* Current semaphore count */
     uint32_t max_count;          /* Maximum semaphore count */
     task_t* waiting_tasks;       /* List of tasks waiting for the semaphore */
     char name[MAX_TASK_NAME_LEN];/* Semaphore name */
 } semaphore_t;
 
 /* Mutex structure (binary semaphore with priority inheritance) */
 typedef struct {
     uint8_t locked;              /* 0 = unlocked, 1 = locked */
     task_t* owner;               /* Task that owns the mutex (if locked) */
     task_t* waiting_tasks;       /* List of tasks waiting for the mutex */
     char name[MAX_TASK_NAME_LEN];/* Mutex name */
 } mutex_t;
 
 /* Message queue structure */
 typedef struct {
     void* buffer;                /* Queue buffer */
     size_t msg_size;             /* Size of each message in bytes */
     uint32_t capacity;           /* Maximum number of messages */
     uint32_t count;              /* Current number of messages */
     uint32_t head;               /* Head index (oldest message) */
     uint32_t tail;               /* Tail index (newest message) */
     task_t* waiting_send;        /* Tasks waiting to send (when queue full) */
     task_t* waiting_recv;        /* Tasks waiting to receive (when queue empty) */
     char name[MAX_TASK_NAME_LEN];/* Queue name */
 } queue_t;
 
 /* Event flags structure */
 typedef struct {
     uint32_t flags;              /* Current event flags */
     task_t* waiting_tasks;       /* Tasks waiting for events */
     char name[MAX_TASK_NAME_LEN];/* Event group name */
 } event_group_t;
 
 /* Wait options for events */
 #define EVENT_WAIT_ALL   0       /* Wait for all specified flags */
 #define EVENT_WAIT_ANY   1       /* Wait for any specified flags */
 #define EVENT_CLEAR      2       /* Clear flags after waiting */
 
 /**
  * @brief Initialize the IPC subsystem
  * 
  * @return int 0 on success, negative error code on failure
  */
 int ipc_init(void);
 
 /* Semaphore functions */
 
 /**
  * @brief Create a semaphore
  * 
  * @param name Semaphore name
  * @param initial_count Initial count value
  * @param max_count Maximum count value
  * @return semaphore_t* Pointer to created semaphore, NULL on failure
  */
 semaphore_t* semaphore_create(const char* name, uint32_t initial_count, uint32_t max_count);
 
 /**
  * @brief Delete a semaphore
  * 
  * @param sem Semaphore to delete
  * @return int 0 on success, negative error code on failure
  */
 int semaphore_delete(semaphore_t* sem);
 
 /**
  * @brief Take (acquire) a semaphore
  * 
  * @param sem Semaphore to take
  * @param timeout Timeout in ticks (MAX_TIMEOUT for infinite)
  * @return int 0 on success, negative error code on failure or timeout
  */
 int semaphore_take(semaphore_t* sem, uint32_t timeout);
 
 /**
  * @brief Give (release) a semaphore
  * 
  * @param sem Semaphore to give
  * @return int 0 on success, negative error code on failure
  */
 int semaphore_give(semaphore_t* sem);
 
 /**
  * @brief Get semaphore count
  * 
  * @param sem Semaphore to query
  * @return uint32_t Current semaphore count
  */
 uint32_t semaphore_get_count(semaphore_t* sem);
 
 /* Mutex functions */
 
 /**
  * @brief Create a mutex
  * 
  * @param name Mutex name
  * @return mutex_t* Pointer to created mutex, NULL on failure
  */
 mutex_t* mutex_create(const char* name);
 
 /**
  * @brief Delete a mutex
  * 
  * @param mutex Mutex to delete
  * @return int 0 on success, negative error code on failure
  */
 int mutex_delete(mutex_t* mutex);
 
 /**
  * @brief Lock a mutex
  * 
  * @param mutex Mutex to lock
  * @param timeout Timeout in ticks (MAX_TIMEOUT for infinite)
  * @return int 0 on success, negative error code on failure or timeout
  */
 int mutex_lock(mutex_t* mutex, uint32_t timeout);
 
 /**
  * @brief Unlock a mutex
  * 
  * @param mutex Mutex to unlock
  * @return int 0 on success, negative error code on failure
  */
 int mutex_unlock(mutex_t* mutex);
 
 /**
  * @brief Check if mutex is locked
  * 
  * @param mutex Mutex to check
  * @return int 1 if locked, 0 if unlocked, negative error code on failure
  */
 int mutex_is_locked(mutex_t* mutex);
 
 /* Message queue functions */
 
 /**
  * @brief Create a message queue
  * 
  * @param name Queue name
  * @param msg_size Size of each message in bytes
  * @param capacity Maximum number of messages
  * @return queue_t* Pointer to created queue, NULL on failure
  */
 queue_t* queue_create(const char* name, size_t msg_size, uint32_t capacity);
 
 /**
  * @brief Delete a message queue
  * 
  * @param queue Queue to delete
  * @return int 0 on success, negative error code on failure
  */
 int queue_delete(queue_t* queue);
 
 /**
  * @brief Send a message to a queue
  * 
  * @param queue Queue to send to
  * @param msg Pointer to message to send
  * @param timeout Timeout in ticks (MAX_TIMEOUT for infinite)
  * @return int 0 on success, negative error code on failure or timeout
  */
 int queue_send(queue_t* queue, const void* msg, uint32_t timeout);
 
 /**
  * @brief Receive a message from a queue
  * 
  * @param queue Queue to receive from
  * @param msg Pointer to buffer to store received message
  * @param timeout Timeout in ticks (MAX_TIMEOUT for infinite)
  * @return int 0 on success, negative error code on failure or timeout
  */
 int queue_receive(queue_t* queue, void* msg, uint32_t timeout);
 
 /**
  * @brief Get number of messages in queue
  * 
  * @param queue Queue to query
  * @return uint32_t Number of messages
  */
 uint32_t queue_get_count(queue_t* queue);
 
 /**
  * @brief Peek at the next message without removing it
  * 
  * @param queue Queue to peek at
  * @param msg Pointer to buffer to store message
  * @return int 0 on success, negative error code on failure (empty queue)
  */
 int queue_peek(queue_t* queue, void* msg);
 
 /* Event group functions */
 
 /**
  * @brief Create an event group
  * 
  * @param name Event group name
  * @return event_group_t* Pointer to created event group, NULL on failure
  */
 event_group_t* event_group_create(const char* name);
 
 /**
  * @brief Delete an event group
  * 
  * @param group Event group to delete
  * @return int 0 on success, negative error code on failure
  */
 int event_group_delete(event_group_t* group);
 
 /**
  * @brief Set flags in an event group
  * 
  * @param group Event group to modify
  * @param flags Flags to set
  * @return uint32_t Previous flags value
  */
 uint32_t event_group_set_flags(event_group_t* group, uint32_t flags);
 
 /**
  * @brief Clear flags in an event group
  * 
  * @param group Event group to modify
  * @param flags Flags to clear
  * @return uint32_t Previous flags value
  */
 uint32_t event_group_clear_flags(event_group_t* group, uint32_t flags);
 
 /**
  * @brief Wait for flags in an event group
  * 
  * @param group Event group to wait on
  * @param flags Flags to wait for
  * @param options Wait options (EVENT_WAIT_ALL, EVENT_WAIT_ANY, EVENT_CLEAR)
  * @param timeout Timeout in ticks (MAX_TIMEOUT for infinite)
  * @return uint32_t Flags that satisfied the wait condition, 0 on timeout
  */
 uint32_t event_group_wait(event_group_t* group, uint32_t flags, uint8_t options, uint32_t timeout);
 
 /**
  * @brief Get current flags in event group
  * 
  * @param group Event group to query
  * @return uint32_t Current flags value
  */
 uint32_t event_group_get_flags(event_group_t* group);
 
 #endif /* IPC_H */