/**
 * @file main.c
 * @brief Main entry point for the RTOS Task Scheduler Simulator
 * 
 * This file contains the main entry point for the RTOS simulator,
 * which initializes the system and runs a set of example satellite tasks.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <signal.h>
 #include <unistd.h>
 #include <time.h>
 
 #include "../include/kernel/task.h"
 #include "../include/kernel/scheduler.h"
 #include "../include/kernel/time.h"
 #include "../include/kernel/context.h"
 #include "../include/kernel/ipc.h"
 #include "../include/drivers/timer.h"
 #include "../include/drivers/uart.h"
 #include "../include/utils/logger.h"
 #include "../include/config.h"
 
 /* Demo task prototypes */
 static void task_telemetry(void* arg);
 static void task_attitude_control(void* arg);
 static void task_thermal_control(void* arg);
 static void task_command_handler(void* arg);
 static void task_housekeeping(void* arg);
 static void task_payload_control(void* arg);
 static void task_system_monitor(void* arg);
 
 /* Global shared resources */
 static semaphore_t* telemetry_sem;
 static queue_t* command_queue;
 static event_group_t* system_events;
 static mutex_t* resource_mutex;
 
 /* Event flags */
 #define EVENT_THERMAL_ALERT      (1 << 0)
 #define EVENT_ATTITUDE_UPDATE    (1 << 1)
 #define EVENT_PAYLOAD_READY      (1 << 2)
 #define EVENT_COMMAND_RECEIVED   (1 << 3)
 #define EVENT_LOW_POWER          (1 << 4)
 
 /* Command types */
 typedef enum {
     CMD_NOOP,
     CMD_RESET,
     CMD_SET_MODE,
     CMD_TAKE_PICTURE,
     CMD_DEPLOY_SOLAR_PANEL,
     CMD_ADJUST_ORBIT
 } command_type_t;
 
 /* Command structure */
 typedef struct {
     command_type_t type;
     uint32_t parameter;
     uint32_t timestamp;
 } command_t;
 
 /* Satellite modes */
 typedef enum {
     MODE_SAFE,
     MODE_NORMAL,
     MODE_LOW_POWER,
     MODE_SCIENCE,
     MODE_MAINTENANCE
 } satellite_mode_t;
 
 /* Shared satellite state */
 static struct {
     satellite_mode_t mode;
     uint32_t orbit_position;      /* 0-359 degrees */
     float battery_level;          /* 0.0-1.0 */
     float temperature;            /* in Celsius */
     uint8_t solar_panels_deployed;
     uint8_t payload_active;
     uint32_t uptime;              /* in seconds */
     uint32_t command_count;
     uint32_t telemetry_packets;
 } satellite_state;
 
 /* Flag to indicate if simulator should continue running */
 static volatile int running = 1;
 
 /* Signal handler for graceful termination */
 static void signal_handler(int sig) {
     printf("\nReceived signal %d, shutting down...\n", sig);
     running = 0;
 }
 
 /**
  * Initialize the satellite state
  */
 static void satellite_init(void) {
     /* Initialize satellite state */
     satellite_state.mode = MODE_SAFE;
     satellite_state.orbit_position = 0;
     satellite_state.battery_level = 0.8f;
     satellite_state.temperature = 25.0f;
     satellite_state.solar_panels_deployed = 0;
     satellite_state.payload_active = 0;
     satellite_state.uptime = 0;
     satellite_state.command_count = 0;
     satellite_state.telemetry_packets = 0;
     
     LOG_INFO("Satellite state initialized");
 }
 
 /**
  * Display satellite status
  */
 static void display_status(void) {
     printf("\033[2J\033[H"); /* Clear screen and move cursor to home */
     
     printf("=== Satellite RTOS Simulator ===\n");
     printf("Uptime: %u seconds\n", satellite_state.uptime);
     
     /* Display mode */
     printf("Mode: ");
     switch (satellite_state.mode) {
         case MODE_SAFE:        printf("SAFE\n"); break;
         case MODE_NORMAL:      printf("NORMAL\n"); break;
         case MODE_LOW_POWER:   printf("LOW POWER\n"); break;
         case MODE_SCIENCE:     printf("SCIENCE\n"); break;
         case MODE_MAINTENANCE: printf("MAINTENANCE\n"); break;
         default:               printf("UNKNOWN\n"); break;
     }
     
     /* Display other states */
     printf("Orbit Position: %u degrees\n", satellite_state.orbit_position);
     printf("Battery Level: %.1f%%\n", satellite_state.battery_level * 100.0f);
     printf("Temperature: %.1f°C\n", satellite_state.temperature);
     printf("Solar Panels: %s\n", satellite_state.solar_panels_deployed ? "DEPLOYED" : "STOWED");
     printf("Payload: %s\n", satellite_state.payload_active ? "ACTIVE" : "INACTIVE");
     printf("Commands Processed: %u\n", satellite_state.command_count);
     printf("Telemetry Packets: %u\n", satellite_state.telemetry_packets);
     
     /* Display system events */
     printf("\nActive System Events:\n");
     uint32_t events = event_group_get_flags(system_events);
     if (events & EVENT_THERMAL_ALERT)      printf("- Thermal Alert\n");
     if (events & EVENT_ATTITUDE_UPDATE)    printf("- Attitude Update Needed\n");
     if (events & EVENT_PAYLOAD_READY)      printf("- Payload Ready\n");
     if (events & EVENT_COMMAND_RECEIVED)   printf("- Command Received\n");
     if (events & EVENT_LOW_POWER)          printf("- Low Power Condition\n");
     if (events == 0)                       printf("- None\n");
     
     /* Display scheduler statistics */
     scheduler_stats_t stats;
     scheduler_get_stats(&stats);
     
     printf("\nRTOS Statistics:\n");
     printf("Context Switches: %u\n", stats.context_switches);
     printf("CPU Load: %.1f%%\n", stats.cpu_load * 100.0f);
     printf("Tasks Created: %u\n", stats.tasks_created);
     printf("Deadline Misses: %u\n", stats.deadline_misses);
     
     /* Display task status */
     printf("\nTask States:\n");
     printf("%-20s %-10s %-10s %-15s\n", "Task Name", "Priority", "State", "Runtime (ms)");
     printf("--------------------------------------------------------------\n");
     
     /* This is simplified - in a real system we'd iterate through all tasks */
     task_t* tasks[] = {
         task_get_by_name("telemetry"),
         task_get_by_name("attitude"),
         task_get_by_name("thermal"),
         task_get_by_name("command"),
         task_get_by_name("housekeep"),
         task_get_by_name("payload"),
         task_get_by_name("monitor"),
         task_get_by_name("idle")
     };
     
     for (int i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
         if (tasks[i] != NULL) {
             task_stats_t task_stats;
             task_get_stats(tasks[i], &task_stats);
             
             printf("%-20s %-10u %-10s %-15u\n",
                    tasks[i]->name,
                    tasks[i]->priority,
                    task_state_to_string(tasks[i]->state),
                    task_stats.total_runtime * 10); /* Convert ticks to ms */
         }
     }
     
     printf("\nPress Ctrl+C to exit\n");
 }
 
 /**
  * Simulate environment changes
  */
 static void update_environment(void) {
     /* Update orbit position (simulate orbital movement) */
     satellite_state.orbit_position = (satellite_state.orbit_position + 1) % 360;
     
     /* Simulate day/night cycle based on orbit position */
     int in_sunlight = (satellite_state.orbit_position >= 0 && 
                        satellite_state.orbit_position <= 180);
     
     /* Update battery level based on sunlight and payload activity */
     if (in_sunlight && satellite_state.solar_panels_deployed) {
         satellite_state.battery_level += 0.01f;
         if (satellite_state.battery_level > 1.0f) {
             satellite_state.battery_level = 1.0f;
         }
     } else {
         /* Drain battery */
         float drain_rate = 0.005f;
         if (satellite_state.payload_active) {
             drain_rate *= 2;
         }
         satellite_state.battery_level -= drain_rate;
         if (satellite_state.battery_level < 0.0f) {
             satellite_state.battery_level = 0.0f;
         }
     }
     
     /* Update temperature based on sunlight and payload activity */
     float target_temp;
     if (in_sunlight) {
         target_temp = 30.0f;
     } else {
         target_temp = 10.0f;
     }
     
     if (satellite_state.payload_active) {
         target_temp += 10.0f;
     }
     
     /* Gradually move towards target temperature */
     if (satellite_state.temperature < target_temp) {
         satellite_state.temperature += 0.5f;
     } else if (satellite_state.temperature > target_temp) {
         satellite_state.temperature -= 0.5f;
     }
     
     /* Trigger thermal alert if temperature is too high or too low */
     if (satellite_state.temperature > 40.0f || satellite_state.temperature < 0.0f) {
         event_group_set_flags(system_events, EVENT_THERMAL_ALERT);
     } else {
         event_group_clear_flags(system_events, EVENT_THERMAL_ALERT);
     }
     
     /* Trigger low power alert if battery level is too low */
     if (satellite_state.battery_level < 0.2f) {
         event_group_set_flags(system_events, EVENT_LOW_POWER);
     } else {
         event_group_clear_flags(system_events, EVENT_LOW_POWER);
     }
     
     /* Increment uptime */
     satellite_state.uptime++;
 }
 
 /**
  * Telemetry task
  * Collects and transmits satellite telemetry
  */
 static void task_telemetry(void* arg) {
     LOG_INFO("Telemetry task starting");
     
     while (running) {
         /* Take semaphore to ensure exclusive access to telemetry system */
         if (semaphore_take(telemetry_sem, 100) == 0) {
             /* Lock resource mutex for satellite state access */
             mutex_lock(resource_mutex, MAX_TIMEOUT);
             
             /* Simulate telemetry collection and transmission */
             LOG_DEBUG("Collecting telemetry data");
             
             /* In a real system, we would read sensors, format data, etc. */
             satellite_state.telemetry_packets++;
             
             /* Unlock resource mutex */
             mutex_unlock(resource_mutex);
             
             /* Release semaphore */
             semaphore_give(telemetry_sem);
             
             /* Set telemetry event flag to signal data is ready */
             event_group_set_flags(system_events, EVENT_ATTITUDE_UPDATE);
         }
         
         /* Regular telemetry transmission period */
         time_delay_ms(5000);
     }
 }
 
 /**
  * Attitude control task
  * Maintains satellite orientation
  */
 static void task_attitude_control(void* arg) {
     LOG_INFO("Attitude control task starting");
     
     while (running) {
         /* Wait for attitude update event */
         event_group_wait(system_events, EVENT_ATTITUDE_UPDATE, 
                         EVENT_WAIT_ANY | EVENT_CLEAR, MAX_TIMEOUT);
         
         /* Lock resource mutex for satellite state access */
         mutex_lock(resource_mutex, MAX_TIMEOUT);
         
         /* Simulate attitude control action */
         LOG_DEBUG("Adjusting satellite attitude");
         
         /* In a real system, we would command reaction wheels, thrusters, etc. */
         
         /* Unlock resource mutex */
         mutex_unlock(resource_mutex);
         
         /* Attitude control processing period */
         time_delay_ms(1000);
     }
 }
 
 /**
  * Thermal control task
  * Manages satellite temperature
  */
 static void task_thermal_control(void* arg) {
     LOG_INFO("Thermal control task starting");
     
     while (running) {
         /* Check for thermal alert */
         if (event_group_get_flags(system_events) & EVENT_THERMAL_ALERT) {
             /* Lock resource mutex for satellite state access */
             mutex_lock(resource_mutex, MAX_TIMEOUT);
             
             /* Simulate thermal control action */
             LOG_WARNING("Thermal alert detected, taking corrective action");
             
             /* In a real system, we would activate heaters, radiators, etc. */
             /* For simulation, just move temperature towards safe range */
             if (satellite_state.temperature > 40.0f) {
                 satellite_state.temperature -= 2.0f;
             } else if (satellite_state.temperature < 0.0f) {
                 satellite_state.temperature += 2.0f;
             }
             
             /* Unlock resource mutex */
             mutex_unlock(resource_mutex);
         }
         
         /* Thermal control processing period */
         time_delay_ms(2000);
     }
 }
 
 /**
  * Command handler task
  * Processes commands from ground station
  */
 static void task_command_handler(void* arg) {
     LOG_INFO("Command handler task starting");
     command_t cmd;
     
     while (running) {
         /* Wait for command in queue */
         if (queue_receive(command_queue, &cmd, MAX_TIMEOUT) == 0) {
             /* Lock resource mutex for satellite state access */
             mutex_lock(resource_mutex, MAX_TIMEOUT);
             
             LOG_INFO("Processing command: %d", cmd.type);
             
             /* Process command based on type */
             switch (cmd.type) {
                 case CMD_NOOP:
                     /* No operation, just acknowledge */
                     break;
                     
                 case CMD_RESET:
                     /* Simulate system reset */
                     LOG_WARNING("System reset command received");
                     satellite_state.mode = MODE_SAFE;
                     satellite_state.payload_active = 0;
                     break;
                     
                 case CMD_SET_MODE:
                     /* Set satellite mode */
                     if (cmd.parameter <= MODE_MAINTENANCE) {
                         satellite_state.mode = (satellite_mode_t)cmd.parameter;
                         LOG_INFO("Mode changed to: %d", satellite_state.mode);
                     }
                     break;
                     
                 case CMD_TAKE_PICTURE:
                     /* Simulate taking picture with payload */
                     if (satellite_state.payload_active) {
                         LOG_INFO("Taking picture");
                         event_group_set_flags(system_events, EVENT_PAYLOAD_READY);
                     } else {
                         LOG_WARNING("Cannot take picture, payload not active");
                     }
                     break;
                     
                 case CMD_DEPLOY_SOLAR_PANEL:
                     /* Deploy solar panels */
                     if (!satellite_state.solar_panels_deployed) {
                         LOG_INFO("Deploying solar panels");
                         satellite_state.solar_panels_deployed = 1;
                     } else {
                         LOG_WARNING("Solar panels already deployed");
                     }
                     break;
                     
                 case CMD_ADJUST_ORBIT:
                     /* Simulate orbit adjustment */
                     LOG_INFO("Adjusting orbit");
                     break;
                     
                 default:
                     LOG_WARNING("Unknown command type: %d", cmd.type);
                     break;
             }
             
             /* Increment command counter */
             satellite_state.command_count++;
             
             /* Unlock resource mutex */
             mutex_unlock(resource_mutex);
             
             /* Set command received event flag */
             event_group_set_flags(system_events, EVENT_COMMAND_RECEIVED);
         }
     }
 }
 
 /**
  * Housekeeping task
  * Performs routine maintenance
  */
 static void task_housekeeping(void* arg) {
     LOG_INFO("Housekeeping task starting");
     
     while (running) {
         /* Lock resource mutex for satellite state access */
         mutex_lock(resource_mutex, MAX_TIMEOUT);
         
         /* Simulate housekeeping activities */
         LOG_DEBUG("Performing housekeeping");
         
         /* In a real system, we would check memory usage, clear logs, etc. */
         
         /* Unlock resource mutex */
         mutex_unlock(resource_mutex);
         
         /* Housekeeping processing period */
         time_delay_ms(10000);
     }
 }
 
 /**
  * Payload control task
  * Manages satellite payload (e.g., camera, instruments)
  */
 static void task_payload_control(void* arg) {
     LOG_INFO("Payload control task starting");
     
     while (running) {
         /* Wait for payload ready event */
         event_group_wait(system_events, EVENT_PAYLOAD_READY, 
                         EVENT_WAIT_ANY | EVENT_CLEAR, MAX_TIMEOUT);
         
         /* Lock resource mutex for satellite state access */
         mutex_lock(resource_mutex, MAX_TIMEOUT);
         
         /* Simulate payload operation */
         LOG_INFO("Operating payload");
         
         /* In a real system, we would capture data, process it, etc. */
         
         /* Unlock resource mutex */
         mutex_unlock(resource_mutex);
         
         /* Payload processing time */
         time_delay_ms(3000);
     }
 }
 
 /**
  * System monitor task
  * Monitors system health and updates status display
  */
 static void task_system_monitor(void* arg) {
     LOG_INFO("System monitor task starting");
     
     while (running) {
         /* Update environment simulation */
         mutex_lock(resource_mutex, MAX_TIMEOUT);
         update_environment();
         mutex_unlock(resource_mutex);
         
         /* Update status display */
         display_status();
         
         /* Monitor processing period */
         time_delay_ms(1000);
     }
 }
 
 /**
  * Main entry point
  */
 int main(int argc, char* argv[]) {
     /* Initialize UART for console output */
     uart_config_t uart_cfg = UART_CONFIG_DEFAULT;
     uart_init(&uart_cfg);
     
     /* Initialize logger */
     logger_init(LOG_LEVEL_INFO);
     logger_set_colored_output(1);
     
     LOG_INFO("Starting RTOS Task Scheduler Simulator");
     
     /* Register signal handler for Ctrl+C */
     signal(SIGINT, signal_handler);
     
     /* Initialize RTOS components */
     context_init();
     timer_init();
     task_init();
     scheduler_init(DEFAULT_SCHEDULING_POLICY);
     ipc_init();
     time_init();
     
     /* Create IPC objects */
     telemetry_sem = semaphore_create("telemetry", 1, 1);
     command_queue = queue_create("commands", sizeof(command_t), 10);
     system_events = event_group_create("events");
     resource_mutex = mutex_create("resource");
     
     if (!telemetry_sem || !command_queue || !system_events || !resource_mutex) {
         LOG_ERROR("Failed to create IPC objects");
         return -1;
     }
     
     /* Initialize satellite state */
     satellite_init();
     
     /* Create tasks */
     task_create("telemetry", 2, task_telemetry, NULL, DEFAULT_STACK_SIZE);
     task_create("attitude", 1, task_attitude_control, NULL, DEFAULT_STACK_SIZE);
     task_create("thermal", 1, task_thermal_control, NULL, DEFAULT_STACK_SIZE);
     task_create("command", 0, task_command_handler, NULL, DEFAULT_STACK_SIZE);
     task_create("housekeep", 3, task_housekeeping, NULL, DEFAULT_STACK_SIZE);
     task_create("payload", 2, task_payload_control, NULL, DEFAULT_STACK_SIZE);
     task_create("monitor", 4, task_system_monitor, NULL, DEFAULT_STACK_SIZE);
     
     /* Set some tasks as periodic */
     task_t* telemetry_task = task_get_by_name("telemetry");
     if (telemetry_task) {
         task_set_periodic(telemetry_task, time_ms_to_ticks(5000), time_ms_to_ticks(4800));
     }
     
     task_t* housekeeping_task = task_get_by_name("housekeep");
     if (housekeeping_task) {
         task_set_periodic(housekeeping_task, time_ms_to_ticks(10000), time_ms_to_ticks(9500));
     }
     
     /* Generate some initial commands for demonstration */
     command_t cmd;
     cmd.type = CMD_DEPLOY_SOLAR_PANEL;
     cmd.parameter = 0;
     cmd.timestamp = time_get_ticks();
     queue_send(command_queue, &cmd, 0);
     
     cmd.type = CMD_SET_MODE;
     cmd.parameter = MODE_NORMAL;
     cmd.timestamp = time_get_ticks();
     queue_send(command_queue, &cmd, 0);
     
     /* Start the scheduler */
     LOG_INFO("Starting scheduler");
     scheduler_start();
     
     /* We should never get here, as scheduler takes over */
     LOG_ERROR("Scheduler returned unexpectedly");
     
     return 0;
 }