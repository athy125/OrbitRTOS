/**
 * @file uart.h
 * @brief UART driver for console output
 *
 * This file defines the UART interface for the RTOS, providing
 * console output for logging and user interaction.
 */

 #ifndef UART_H
 #define UART_H
 
 #include <stdint.h>
 #include <stddef.h>
 
 /* UART baud rates */
 typedef enum {
     UART_BAUD_9600 = 9600,
     UART_BAUD_19200 = 19200,
     UART_BAUD_38400 = 38400,
     UART_BAUD_57600 = 57600,
     UART_BAUD_115200 = 115200,
     UART_BAUD_230400 = 230400,
     UART_BAUD_460800 = 460800,
     UART_BAUD_921600 = 921600
 } uart_baud_t;
 
 /* UART parity */
 typedef enum {
     UART_PARITY_NONE,
     UART_PARITY_ODD,
     UART_PARITY_EVEN
 } uart_parity_t;
 
 /* UART data bits */
 typedef enum {
     UART_DATA_BITS_5,
     UART_DATA_BITS_6,
     UART_DATA_BITS_7,
     UART_DATA_BITS_8,
     UART_DATA_BITS_9
 } uart_data_bits_t;
 
 /* UART stop bits */
 typedef enum {
     UART_STOP_BITS_1,
     UART_STOP_BITS_1_5,
     UART_STOP_BITS_2
 } uart_stop_bits_t;
 
 /* UART flow control */
 typedef enum {
     UART_FLOW_CONTROL_NONE,
     UART_FLOW_CONTROL_RTS_CTS,
     UART_FLOW_CONTROL_XON_XOFF
 } uart_flow_control_t;
 
 /* UART configuration */
 typedef struct {
     uart_baud_t baud_rate;
     uart_parity_t parity;
     uart_data_bits_t data_bits;
     uart_stop_bits_t stop_bits;
     uart_flow_control_t flow_control;
 } uart_config_t;
 
 /* Default UART configuration */
 #define UART_CONFIG_DEFAULT { \
     .baud_rate = UART_BAUD_115200, \
     .parity = UART_PARITY_NONE, \
     .data_bits = UART_DATA_BITS_8, \
     .stop_bits = UART_STOP_BITS_1, \
     .flow_control = UART_FLOW_CONTROL_NONE \
 }
 
 /**
  * @brief Initialize the UART driver
  * 
  * @param config UART configuration
  * @return int 0 on success, negative error code on failure
  */
 int uart_init(const uart_config_t* config);
 
 /**
  * @brief Deinitialize the UART driver
  * 
  * @return int 0 on success, negative error code on failure
  */
 int uart_deinit(void);
 
 /**
  * @brief Set UART configuration
  * 
  * @param config UART configuration
  * @return int 0 on success, negative error code on failure
  */
 int uart_set_config(const uart_config_t* config);
 
 /**
  * @brief Get current UART configuration
  * 
  * @param config Pointer to store configuration
  * @return int 0 on success, negative error code on failure
  */
 int uart_get_config(uart_config_t* config);
 
 /**
  * @brief Write data to UART
  * 
  * @param data Data to write
  * @param len Length of data in bytes
  * @return int Number of bytes written, negative error code on failure
  */
 int uart_write(const void* data, size_t len);
 
 /**
  * @brief Read data from UART
  * 
  * @param data Buffer to store read data
  * @param len Maximum number of bytes to read
  * @return int Number of bytes read, negative error code on failure
  */
 int uart_read(void* data, size_t len);
 
 /**
  * @brief Read one character from UART
  * 
  * @param timeout_ms Timeout in milliseconds, 0 for non-blocking
  * @return int Character read (0-255), negative error code on failure or timeout
  */
 int uart_getc(uint32_t timeout_ms);
 
 /**
  * @brief Write one character to UART
  * 
  * @param c Character to write
  * @return int 0 on success, negative error code on failure
  */
 int uart_putc(int c);
 
 /**
  * @brief Write string to UART
  * 
  * @param str String to write (null-terminated)
  * @return int Number of bytes written, negative error code on failure
  */
 int uart_puts(const char* str);
 
 /**
  * @brief Flush UART transmit buffer
  * 
  * @return int 0 on success, negative error code on failure
  */
 int uart_flush_tx(void);
 
 /**
  * @brief Flush UART receive buffer
  * 
  * @return int 0 on success, negative error code on failure
  */
 int uart_flush_rx(void);
 
 /**
  * @brief Check if UART is ready to transmit
  * 
  * @return int 1 if ready, 0 if not ready, negative error code on failure
  */
 int uart_tx_ready(void);
 
 /**
  * @brief Check if data is available to read from UART
  * 
  * @return int Number of bytes available, negative error code on failure
  */
 int uart_rx_available(void);
 
 /**
  * @brief Write formatted string to UART (printf-style)
  * 
  * @param format Format string
  * @param ... Variable arguments
  * @return int Number of bytes written, negative error code on failure
  */
 int uart_printf(const char* format, ...);
 
 #endif /* UART_H */