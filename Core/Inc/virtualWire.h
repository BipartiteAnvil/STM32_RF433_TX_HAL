#ifndef VIRTUALWIRE_H
#define VIRTUALWIRE_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define VW_MAX_MESSAGE_LEN 30
#define VW_MAX_PAYLOAD VW_MAX_MESSAGE_LEN-3
#define VW_RX_RAMP_LEN 160
#define VW_RX_SAMPLES_PER_BIT 8
#define VW_RAMP_INC (VW_RX_RAMP_LEN/VW_RX_SAMPLES_PER_BIT)
#define VW_RAMP_TRANSITION VW_RX_RAMP_LEN/2
#define VW_RAMP_ADJUST 9
#define VW_RAMP_INC_RETARD (VW_RAMP_INC-VW_RAMP_ADJUST)
#define VW_RAMP_INC_ADVANCE (VW_RAMP_INC+VW_RAMP_ADJUST)
#define VW_HEADER_LEN 8

extern GPIO_TypeDef* vw_rx_port;
extern GPIO_TypeDef* vw_tx_port;
extern GPIO_TypeDef* vw_ptt_port;


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Compute CRC over count bytes.
 * @param ptr Pointer to the data.
 * @param count Number of bytes to compute CRC over.
 * @return The computed CRC.
 */
uint16_t vw_crc(uint8_t *ptr, uint8_t count);

/**
 * @brief Convert a 6-bit encoded symbol into its 4-bit decoded equivalent.
 * @param symbol The 6-bit encoded symbol.
 * @return The 4-bit decoded equivalent.
 */
uint8_t vw_symbol_6to4(uint8_t symbol);

/**
 * @brief Set the output pin number for transmitter data.
 * @param pin The pin number.
 */
void vw_set_tx_pin(GPIO_TypeDef* port ,uint16_t pin);

/**
 * @brief Set the input pin number for receiver data.
 * @param pin The pin number.
 */
void vw_set_rx_pin(uint8_t pin);

/**
 * @brief Set the output pin number for transmitter PTT (Push-To-Talk) enable.
 * @param pin The pin number.
 */
void vw_set_ptt_pin(uint8_t pin);

/**
 * @brief Set the PTT pin to be inverted (low to transmit).
 * @param inverted 1 to invert the PTT pin, 0 to keep it non-inverted.
 */

void vw_timer_handler(void);


void vw_set_ptt_inverted(uint8_t inverted);

/**
 * @brief Set up the VirtualWire library with the specified bit rate.
 * @param speed The bit rate in bits per second.
 */
void vw_setup(uint16_t speed);

/**
 * @brief Start the transmitter.
 */
void vw_tx_start();

/**
 * @brief Stop the transmitter.
 */
void vw_tx_stop();

/**
 * @brief Start the receiver.
 */
void vw_rx_start();

/**
 * @brief Stop the receiver.
 */
void vw_rx_stop();

/**
 * @brief Check if the transmitter is active.
 * @return 1 if the transmitter is active, 0 otherwise.
 */
uint8_t vx_tx_active();

/**
 * @brief Wait for the transmitter to finish.
 */
void vw_wait_tx();

/**
 * @brief Wait for a message to be received.
 */
void vw_wait_rx();

/**
 * @brief Wait for a message to be received, with a maximum timeout.
 * @param milliseconds The maximum time to wait, in milliseconds.
 * @return 1 if a message was received, 0 otherwise.
 */
uint8_t vw_wait_rx_max(unsigned long milliseconds);

/**
 * @brief Send a message.
 * @param buf Pointer to the message data.
 * @param len The length of the message in bytes.
 * @return 1 if the message was sent successfully, 0 otherwise.
 */
uint8_t vw_send(uint8_t* buf, uint8_t len);

/**
 * @brief Check if a message has been received.
 * @return 1 if a message is available, 0 otherwise.
 */
uint8_t vw_have_message();

/**
 * @brief Get the received message.
 * @param buf Pointer to a buffer to store the message.
 * @param len Pointer to a variable to store the message length.
 * @return 1 if the message was received successfully, 0 otherwise.
 */
uint8_t vw_get_message(uint8_t* buf, uint8_t* len);

#ifdef __cplusplus
}
#endif

#endif // VIRTUALWIRE_H
