# VirtualWire STM32 Implementation

This project implements the VirtualWire library on an STM32F103C8T6 microcontroller for wireless communication using SRX882S (receiver) and STX882S (transmitter) modules.
This implementation was inspired by virtual wire library for arduino. This is compatible with arduino RX/TX. 
Original arduino library: 
	https://github.com/tenbaht/VirtualWire
	https://electronoobs.com/eng_arduino_tut99_code1.php

## Table of Contents

1. [Hardware Setup](#hardware-setup)
2. [Software Configuration](#software-configuration)
3. [Timer Configuration](#timer-configuration)
4. [Usage](#usage)
5. [Transmitting Data](#transmitting-data)
6. [Receiving Data](#receiving-data)

## Hardware Setup

- Microcontroller: STM32F103C8T6
- Transmitter: STX882S
- Receiver: SRX882S

Connect the STX882S and SRX882S to your STM32F103C8T6 as follows:

- STX882S:
  - DATA pin to a GPIO pin (e.g., PB12)
  - VCC to 3.3V
  - GND to GND

- SRX882S:
  - DATA pin to a GPIO pin (e.g., PB11)
  - VCC to 5V (if using 5V version) or 3.3V
  - GND to GND

## Software Configuration

This project uses STM32CubeMX for initial configuration. The VirtualWire library has been ported to work with STM32 HAL.

### STM32CubeMX Configuration

1. Set up your clock configuration:
   - HSE: 8MHz (if using external crystal)
   - HCLK: 72MHz
   - APB1 Timer clocks: 72MHz

2. Configure TIM2:
   - Mode: Internal Clock
   - Prescaler: 71 (for 72MHz system clock)
   - Counter Period: 62 (for 2000 baud rate)
   - Enable TIM2 global interrupt in NVIC Settings

3. Configure your TX and RX GPIO pins as needed.

## Timer Configuration

The timer (TIM2) is configured to provide interrupts at 8 times the baud rate. This is crucial for the VirtualWire library to function correctly.

### Timer Calculation

For a baud rate of 2000:

1. Timer clock = 72MHz / (Prescaler + 1) = 72MHz / 72 = 1MHz
2. Desired interrupt frequency = Baud rate * 8 = 2000 * 8 = 16kHz
3. Counter Period = (Timer clock / Desired frequency) - 1 = (1MHz / 16kHz) - 1 = 62

These calculations result in:
- Prescaler: 71
- Counter Period: 62

## Usage

1. Include the VirtualWire header in your main.c:
   ```c
   #include "virtualWire.h"
   
#Initialize VirtualWire in your main function:
	vw_set_tx_pin(TX_GPIO_PORT, TX_PIN);
	vw_set_rx_pin(RX_PIN);	
	vw_setup(2000);  // Set to your desired baud rate

# Timer callback: 
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    vw_timer_handler();
  }
}

#Transmit Data
	uint8_t message[] = "Hello, World!";
	vw_send(message, sizeof(message));
	vw_wait_tx(); // Wait until the entire message is sent

#Receiving Data
	vw_rx_start();	// starting the receiver 
	
#Check for received messages in main loop:
	uint8_t buf[VW_MAX_MESSAGE_LEN];
	uint8_t buflen = VW_MAX_MESSAGE_LEN;

	if (vw_get_message(buf, &buflen))
	{
  		// Message received, process buf
	}

   
   
