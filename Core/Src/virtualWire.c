#include "virtualWire.h"
#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_tim.h>
#include <stm32f1xx_hal_gpio.h>
#include <crc16.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern TIM_HandleTypeDef htim2;
extern GPIO_TypeDef *vw_rx_port;
extern GPIO_TypeDef *vw_tx_port;
extern GPIO_TypeDef *vw_ptt_port;

static uint8_t vw_tx_buf[(VW_MAX_MESSAGE_LEN * 2) + VW_HEADER_LEN] = { 0x2a,
		0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x38, 0x2c };
static uint8_t vw_tx_len = 0;
static uint8_t vw_tx_index = 0;
static uint8_t vw_tx_bit = 0;
static uint8_t vw_tx_sample = 0;
static volatile uint8_t vw_tx_enabled = 0;
static uint16_t vw_tx_msg_count = 0;
static uint8_t vw_ptt_pin = 10;
static uint8_t vw_ptt_inverted = 0;
static uint8_t vw_rx_pin = 11;
static uint8_t vw_tx_pin = 12;
static uint8_t vw_rx_sample = 0;
static uint8_t vw_rx_last_sample = 0;
static uint8_t vw_rx_pll_ramp = 0;
static uint8_t vw_rx_integrator = 0;
static uint8_t vw_rx_active = 0;
static volatile uint8_t vw_rx_done = 0;
static uint8_t vw_rx_enabled = 0;
static uint16_t vw_rx_bits = 0;
static uint8_t vw_rx_bit_count = 0;
static uint8_t vw_rx_buf[VW_MAX_MESSAGE_LEN];
static uint8_t vw_rx_count = 0;
static volatile uint8_t vw_rx_len = 0;
static uint8_t vw_rx_bad = 0;
static uint8_t vw_rx_good = 0;
static uint8_t symbols[] = { 0xd, 0xe, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c, 0x23,
		0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34 };

void vw_setup(uint16_t speed) {
	// Initialize internal state
	vw_rx_bits = 0;
	vw_tx_enabled = false;
	vw_rx_enabled = false;
	vw_rx_active = false;
	vw_rx_done = false;
}


uint16_t vw_crc(uint8_t *ptr, uint8_t count) {
	uint16_t crc = 0xffff;
	while (count-- > 0)
		crc = _crc_ccitt_update(crc, *ptr++);
	return crc;
}

uint8_t vw_symbol_6to4(uint8_t symbol) {
	uint8_t i;
	for (i = 0; i < 16; i++)
		if (symbol == symbols[i])
			return i;
	return 0;
}

void vw_set_tx_pin(GPIO_TypeDef *port, uint16_t pin) {
	vw_tx_pin = pin;
}

void vw_set_rx_pin(uint8_t pin) {
	vw_rx_pin = pin;
}

void vw_set_ptt_pin(uint8_t pin) {
	vw_ptt_pin = pin;
}

void vw_set_ptt_inverted(uint8_t inverted) {
	vw_ptt_inverted = inverted;
}

void vw_pll() {
	if (vw_rx_sample)
		vw_rx_integrator++;

	if (vw_rx_sample != vw_rx_last_sample) {
		vw_rx_pll_ramp += (
				(vw_rx_pll_ramp < VW_RAMP_TRANSITION) ?
				VW_RAMP_INC_RETARD :
														VW_RAMP_INC_ADVANCE);
		vw_rx_last_sample = vw_rx_sample;
	} else {
		vw_rx_pll_ramp += VW_RAMP_INC;
	}

	if (vw_rx_pll_ramp >= VW_RX_RAMP_LEN) {
		vw_rx_bits >>= 1;

		if (vw_rx_integrator >= 5)
			vw_rx_bits |= 0x800;

		vw_rx_pll_ramp -= VW_RX_RAMP_LEN;
		vw_rx_integrator = 0;

		if (vw_rx_active) {
			if (++vw_rx_bit_count >= 12) {
				uint8_t this_byte = (vw_symbol_6to4(vw_rx_bits & 0x3f)) << 4
						| vw_symbol_6to4(vw_rx_bits >> 6);

				if (vw_rx_len == 0) {
					vw_rx_count = this_byte;
					if (vw_rx_count < 4 || vw_rx_count > VW_MAX_MESSAGE_LEN) {
						vw_rx_active = false;
						vw_rx_bad++;
						return;
					}
				}
				vw_rx_buf[vw_rx_len++] = this_byte;

				if (vw_rx_len >= vw_rx_count) {
					vw_rx_active = false;
					vw_rx_good++;
					vw_rx_done = true;
				}
				vw_rx_bit_count = 0;
			}
		} else if (vw_rx_bits == 0xb38) {
			vw_rx_active = true;
			vw_rx_bit_count = 0;
			vw_rx_len = 0;
			vw_rx_done = false;
		}
	}
}


void vw_tx_start() {
	vw_tx_index = 0;
	vw_tx_bit = 0;
	vw_tx_sample = 0;
	HAL_GPIO_WritePin(vw_ptt_port, vw_ptt_pin,
			vw_ptt_inverted ? GPIO_PIN_RESET : GPIO_PIN_SET);
	vw_tx_enabled = true;
}

void vw_tx_stop() {
	HAL_GPIO_WritePin(vw_ptt_port, vw_ptt_pin,
			vw_ptt_inverted ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(vw_tx_port, vw_tx_pin, GPIO_PIN_RESET);
	vw_tx_enabled = false;
}

// function to be called from the timer interrupt
void vw_timer_handler(void) {
	if (vw_rx_enabled && !vw_tx_enabled)
		vw_rx_sample = HAL_GPIO_ReadPin(vw_rx_port, vw_rx_pin);

	// Transmitter code
	if (vw_tx_enabled && vw_tx_sample++ == 0) {
		if (vw_tx_index >= vw_tx_len) {
			vw_tx_stop();
			vw_tx_msg_count++;
		} else {
			HAL_GPIO_WritePin(vw_tx_port, vw_tx_pin,
					(vw_tx_buf[vw_tx_index] & (1 << vw_tx_bit++)) ?
							GPIO_PIN_SET : GPIO_PIN_RESET);
			if (vw_tx_bit >= 6) {
				vw_tx_bit = 0;
				vw_tx_index++;
			}
		}
	}
	if (vw_tx_sample > 7)
		vw_tx_sample = 0;

	// Receiver code
	if (vw_rx_enabled && !vw_tx_enabled)
		vw_pll();
}


void vw_rx_start() {
	if (!vw_rx_enabled) {
		vw_rx_enabled = true;
		vw_rx_active = false;
	}
}

void vw_rx_stop() {
	vw_rx_enabled = false;
}

uint8_t vx_tx_active() {
	return vw_tx_enabled;
}

void vw_wait_tx() {
	while (vw_tx_enabled)
		;
}

void vw_wait_rx() {
	while (!vw_rx_done)
		;
}

uint8_t vw_wait_rx_max(unsigned long milliseconds) {
	unsigned long start = HAL_GetTick();
	while (!vw_rx_done && ((HAL_GetTick() - start) < milliseconds))
		;
	return vw_rx_done;
}

uint8_t vw_send(uint8_t *buf, uint8_t len) {
	uint8_t i, index = 0;
	uint16_t crc = 0xffff;
	uint8_t *p = vw_tx_buf + VW_HEADER_LEN;
	uint8_t count = len + 3;
	if (len > VW_MAX_PAYLOAD)
		return false;
	vw_wait_tx();
	crc = _crc_ccitt_update(crc, count);
	p[index++] = symbols[count >> 4];
	p[index++] = symbols[count & 0xf];
	for (i = 0; i < len; i++) {
		crc = _crc_ccitt_update(crc, buf[i]);
		p[index++] = symbols[buf[i] >> 4];
		p[index++] = symbols[buf[i] & 0xf];
	}
	crc = ~crc;
	p[index++] = symbols[(crc >> 4) & 0xf];
	p[index++] = symbols[crc & 0xf];
	p[index++] = symbols[(crc >> 12) & 0xf];
	p[index++] = symbols[(crc >> 8) & 0xf];
	vw_tx_len = index + VW_HEADER_LEN;
	vw_tx_start();
	return true;
}

uint8_t vw_have_message() {
	return vw_rx_done;
}

uint8_t vw_get_message(uint8_t *buf, uint8_t *len) {
	uint8_t rxlen;
	if (!vw_rx_done)
		return false;
	rxlen = vw_rx_len - 3;
	if (*len > rxlen)
		*len = rxlen;
	memcpy(buf, vw_rx_buf + 1, *len);
	vw_rx_done = false;
	return (vw_crc(vw_rx_buf, vw_rx_len) == 0xf0b8);
}
