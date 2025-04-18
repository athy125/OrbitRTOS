/**
 * @file config.h
 * @brief System configuration for the RTOS simulator
 *
 * This file contains all configurable parameters for the RTOS simulator,
 * including system limits, timing parameters, and debugging options.
 */

 #ifndef CONFIG_H
 #define CONFIG_H
 
 /* System capacity limits */
 #define MAX_TASKS                32      /* Maximum number of tasks in the system */
 #define MAX_PRIORITY_LEVELS      16      /* Number of priority levels supported */
 #define MAX_SEMAPHORES           16      /* Maximum number of semaphores */
 #define MAX_QUEUES               16      /* Maximum number of message queues */
 #define MAX_QUEUE_SIZE           32      /* Maximum size of each message queue */
 #define MAX_TASK_NAME_LEN        16      /* Maximum length of task name */
 
 /* Time parameters */
 #define SYSTEM_TICK_MS           10      /* System tick in milliseconds */
 #define MILLISECONDS_PER_TICK    10
 #define DEFAULT_STACK_SIZE       2048    /* Default stack size for tasks in bytes */
 #define DEFAULT_TIME_SLICE       10      /* Default time slice in system ticks */
 #define MAX_TIMEOUT              0xFFFFFFFF
 
 /* Scheduling policies */
 #define SCHEDULING_POLICY_PRIORITY   0   /* Priority-based scheduling */
 #define SCHEDULING_POLICY_RR         1   /* Round-robin scheduling */
 #define SCHEDULING_POLICY_EDF        2   /* Earliest deadline first */
 #define SCHEDULING_POLICY_RMS        3   /* Rate monotonic scheduling */
 
 /* Default scheduling policy */
 #define DEFAULT_SCHEDULING_POLICY SCHEDULING_POLICY_PRIORITY
 
 /* Debug options */
 #define DEBUG_LEVEL              2       /* 0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG */
 #define ENABLE_STATS             1       /* Enable collection of performance statistics */
 #define ENABLE_ASSERTIONS        1       /* Enable runtime assertions */
 #define SIMULATE_JITTER          0       /* Simulate timing jitter */
 #define JITTER_MAX_PCT           5       /* Maximum jitter percentage */
 #define LOG_BUFFER_SIZE          4096    /* Size of the logging buffer */
 
 /* Visualization options */
 #define ENABLE_VISUALIZATION     1       /* Enable console visualization */
 #define VISUALIZATION_REFRESH_MS 1000    /* Refresh rate for visualization in ms */
 
 /* Satellite specific parameters */
 #define SATELLITE_ORBIT_PERIOD   5400    /* Orbit period in seconds (90 minutes) */
 #define CRITICAL_TASK_PRIORITY   0       /* Highest priority level */
 #define HOUSEKEEPING_PRIORITY    5       /* Medium priority level */
 #define LOW_PRIORITY             10      /* Low priority tasks */
 
 /* Hardware simulation parameters */
 #define SIMULATE_RADIATION_EFFECTS 0     /* Simulate radiation-induced errors */
 #define SIMULATE_POWER_CONSTRAINTS 0     /* Simulate power limitations */
 
 #endif /* CONFIG_H */