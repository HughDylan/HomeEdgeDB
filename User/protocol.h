#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

void protocol_rx_byte(uint8_t byte);
void protocol_poll(void);

#endif
