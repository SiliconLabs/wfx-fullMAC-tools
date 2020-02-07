
#ifndef CONSOLE_H_
#define CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*console_is_rx_end_cb_t)(char *buffer_pos);

typedef struct console_config_s {
  USART_TypeDef *usart_instance;
  DMADRV_PeripheralSignal_t dma_peripheral_signal;
  uint8_t echo;
} console_config_t;

int console_init(console_config_t *config);
int console_get_line(char *buffer, uint32_t buffer_size);
int console_get_line_tmo(char *buffer, uint32_t buffer_size, uint32_t timeout);
int console_get_lines(char *buffer,
                      uint32_t buffer_size,
                      console_is_rx_end_cb_t rx_end_cb);
int console_get_lines_tmo (char *buffer,
                           uint32_t buffer_size,
                           uint32_t timeout,
                           console_is_rx_end_cb_t rx_end_cb);

#ifdef __cplusplus
}
#endif

#endif //CONSOLE_H_
