/**
 * @file logger.h
 * @brief Logging utilities for the RTOS
 *
 * This file defines the logging utilities for the RTOS,
 * providing various log levels and formatting options.
 */

 #ifndef LOGGER_H
 #define LOGGER_H
 
 #include <stdint.h>
 #include <stdarg.h>
 #include "../config.h"
 
 /* Log levels */
 typedef enum {
     LOG_LEVEL_NONE = 0,   /* No logging */
     LOG_LEVEL_ERROR,      /* Critical errors */
     LOG_LEVEL_WARNING,    /* Warnings */
     LOG_LEVEL_INFO,       /* Informational messages */
     LOG_LEVEL_DEBUG       /* Debug messages */
 } log_level_t;
 
 /* Log colors */
 #define LOG_COLOR_RESET   "\033[0m"
 #define LOG_COLOR_RED     "\033[31m"
 #define LOG_COLOR_GREEN   "\033[32m"
 #define LOG_COLOR_YELLOW  "\033[33m"
 #define LOG_COLOR_BLUE    "\033[34m"
 #define LOG_COLOR_MAGENTA "\033[35m"
 #define LOG_COLOR_CYAN    "\033[36m"
 #define LOG_COLOR_WHITE   "\033[37m"
 
 /**
  * @brief Initialize the logging subsystem
  * 
  * @param level Initial log level
  * @return int 0 on success, negative error code on failure
  */
 int logger_init(log_level_t level);
 
 /**
  * @brief Set the log level
  * 
  * @param level New log level
  * @return int 0 on success, negative error code on failure
  */
 int logger_set_level(log_level_t level);
 
 /**
  * @brief Get current log level
  * 
  * @return log_level_t Current log level
  */
 log_level_t logger_get_level(void);
 
 /**
  * @brief Enable/disable colored output
  * 
  * @param enable 1 to enable, 0 to disable
  * @return int 0 on success, negative error code on failure
  */
 int logger_set_colored_output(int enable);
 
 /**
  * @brief Log a message with specified level
  * 
  * @param level Log level for message
  * @param file Source file name
  * @param line Source line number
  * @param func Function name
  * @param format Format string
  * @param ... Variable arguments
  * @return int Number of characters written, negative error code on failure
  */
 int logger_log(log_level_t level, const char* file, int line, 
                const char* func, const char* format, ...);
 
 /**
  * @brief Log a message with variable arguments list
  * 
  * @param level Log level for message
  * @param file Source file name
  * @param line Source line number
  * @param func Function name
  * @param format Format string
  * @param args Variable arguments list
  * @return int Number of characters written, negative error code on failure
  */
 int logger_vlog(log_level_t level, const char* file, int line,
                 const char* func, const char* format, va_list args);
 
 /**
  * @brief Flush log buffer
  * 
  * @return int 0 on success, negative error code on failure
  */
 int logger_flush(void);
 
 /**
  * @brief Set output file for logs
  * 
  * @param filename Output file name, NULL for stdout
  * @return int 0 on success, negative error code on failure
  */
 int logger_set_output_file(const char* filename);
 
 /* Convenience macros for logging */
 #define LOG_ERROR(format, ...) \
     logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
 
 #define LOG_WARNING(format, ...) \
     logger_log(LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
 
 #define LOG_INFO(format, ...) \
     logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
 
 #define LOG_DEBUG(format, ...) \
     logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
 
 /* Assert macro */
 #if ENABLE_ASSERTIONS
 #define ASSERT(condition, message) \
     do { \
         if (!(condition)) { \
             LOG_ERROR("Assertion failed: %s, message: %s", #condition, message); \
             for(;;); /* Infinite loop to halt execution */ \
         } \
     } while(0)
 #else
 #define ASSERT(condition, message) ((void)0)
 #endif
 
 #endif /* LOGGER_H */