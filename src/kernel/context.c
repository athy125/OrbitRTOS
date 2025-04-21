/**
 * @file context.c
 * @brief Implementation of context switching functionality
 * 
 * This is a simulated implementation of context switching.
 * For a real embedded system, this would be implemented
 * with architecture-specific assembly code.
 */

 #include <stdlib.h>
 #include <string.h>
 #include <setjmp.h>
 #include <signal.h>
 #include "../../include/kernel/context.h"
 #include "../../include/kernel/task.h"
 #include "../../include/utils/logger.h"
 #include "../../include/config.h"
 
 /* Context switching is simulated using setjmp/longjmp */
 typedef struct {
     jmp_buf env;       /* Task context saved with setjmp */
     void* stack_base;  /* Base of allocated stack memory */
 } ctx_data_t;
 
 /* Critical section nesting counter */
 static uint32_t critical_section_count = 0;
 
 /* Previous interrupt state (simulated) */
 static volatile sig_atomic_t prev_intr_state = 0;
 
 /**
  * Context switch trampoline function
  * This function is the entry point for new tasks
  */
 static void context_trampoline(void) {
     task_t* task = task_get_current();
     
     /* Exit critical section before starting task */
     context_exit_critical(1);
     
     /* Call task function */
     if (task != NULL && task->task_func != NULL) {
         task->task_func(task->task_arg);
     }
     
     /* Task returned, terminate it */
     LOG_INFO("Task '%s' returned from main function, terminating", task->name);
     task->state = TASK_STATE_TERMINATED;
     
     /* Trigger scheduler to select next task */
     scheduler_context_switch();
     
     /* We should never get here */
     LOG_ERROR("Task '%s' resumed after termination", task->name);
     for(;;);
 }
 
 /**
  * Initialize context switching
  */
 int context_init(void) {
     LOG_INFO("Initializing context switching");
     
     /* Reset critical section counter */
     critical_section_count = 0;
     
     return 0;
 }
 
 /**
  * Initialize a task's context
  */
 int context_init_task(task_t* task, void* stack_ptr, uint32_t stack_size,
                       void (*task_func)(void*), void* task_arg) {
     if (task == NULL || stack_ptr == NULL || task_func == NULL) {
         LOG_ERROR("Invalid parameters");
         return -1;
     }
     
     /* Create context data structure */
     ctx_data_t* ctx = (ctx_data_t*)malloc(sizeof(ctx_data_t));
     if (ctx == NULL) {
         LOG_ERROR("Failed to allocate context data");
         return -1;
     }
     
     /* Save stack information */
     task->context.stack_base = (uint32_t)stack_ptr;
     task->context.stack_size = stack_size;
     
     /* Save stack pointer */
     task->context.stack_ptr = (uint32_t*)stack_ptr;
     
     /* Save trampoline context */
     if (setjmp(ctx->env) == 0) {
         /* Initial setup - save context info */
         ctx->stack_base = stack_ptr;
         
         /* Store context in task's stack space */
         memcpy(task->context.stack_ptr, ctx, sizeof(ctx_data_t));
     } else {
         /* Returns here when context is restored */
         context_trampoline();
     }
     
     /* Free temporary context data */
     free(ctx);
     
     return 0;
 }
 
 /**
  * Switch context from one task to another
  */
 int context_switch(task_t* from, task_t* to) {
     if (from == NULL || to == NULL) {
         LOG_ERROR("Invalid task pointers");
         return -1;
     }
     
     /* Get context data from stack */
     ctx_data_t* from_ctx = (ctx_data_t*)from->context.stack_ptr;
     ctx_data_t* to_ctx = (ctx_data_t*)to->context.stack_ptr;
     
     /* Save current context */
     if (setjmp(from_ctx->env) == 0) {
         /* Switch to new context */
         longjmp(to_ctx->env, 1);
     }
     
     /* We return here when switched back to this task */
     return 0;
 }
 
 /**
  * Start first task (no context saving needed)
  */
 int context_start_first_task(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Get context data from stack */
     ctx_data_t* ctx = (ctx_data_t*)task->context.stack_ptr;
     
     /* Set as current task */
     task_set_current(task);
     task->state = TASK_STATE_RUNNING;
     
     /* Jump to task context */
     longjmp(ctx->env, 1);
     
     /* Should never get here */
     return -1;
 }
 
 /**
  * Enter critical section (disable interrupts)
  */
 uint32_t context_enter_critical(void) {
     /* Save current interrupt state */
     sig_atomic_t state = prev_intr_state;
     
     /* Disable interrupts (simulated) */
     prev_intr_state = 1;
     
     /* Increment nesting counter */
     critical_section_count++;
     
     return state;
 }
 
 /**
  * Exit critical section (restore interrupts)
  */
 void context_exit_critical(uint32_t prev_state) {
     /* Ensure counter doesn't underflow */
     if (critical_section_count > 0) {
         critical_section_count--;
     }
     
     /* If counter is zero, restore interrupt state */
     if (critical_section_count == 0) {
         prev_intr_state = prev_state;
     }
 }
 
 /**
  * Check if currently in critical section
  */
 int context_in_critical(void) {
     return (critical_section_count > 0) ? 1 : 0;
 }
 
 /**
  * Check if stack overflow occurred for a task
  */
 int context_check_stack_overflow(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return -1;
     }
     
     /* Placeholder for stack checking */
     /* In a real system, this would check stack usage patterns */
     return 0;
 }
 
 /**
  * Get remaining stack space for a task
  */
 uint32_t context_get_stack_free(task_t* task) {
     if (task == NULL) {
         LOG_ERROR("NULL task pointer");
         return 0;
     }
     
     /* In this simulation, we don't track actual stack usage */
     /* Return a placeholder value */
     return task->context.stack_size / 2;
 }