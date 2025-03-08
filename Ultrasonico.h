/*
 * Ultrasonico.h
 *
 * Created: 2/03/2025 12:28:41
 *  Author: pablo
 */ 


#ifndef ULTRASONICO_H_
#define ULTRASONICO_H_

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// Definiciones para el sensor ultraso?nico

// Trigger: se conecta a PB4
#define ULTRASONIC_TRIGGER_DDR   DDRB
#define ULTRASONIC_TRIGGER_PORT  PORTB
#define ULTRASONIC_TRIGGER_PIN   PB4

// Echo: se conecta a PB5 y se usara? con interrupcio?n de cambio de pin (PCINT)
// Nota: PB5 corresponde a PCINT5 en el ATmega328P.
#define ULTRASONIC_ECHO_DDR      DDRB
#define ULTRASONIC_ECHO_PORT     PORTB
#define ULTRASONIC_ECHO_PIN      PB5
#define ULTRASONIC_ECHO_PIN_IN   PINB

// Prototipos de funciones
void Ultrasound_Init(void);
uint16_t Ultrasound_ReadDistance(void);
void Ultrasound_Task(void);

// Funciones UART (para salida serial, opcional)
void UART_Init(uint16_t baud);
void UART_SendChar(char data);
void UART_SendString(const char *str);
void UART_SendNumber(uint16_t num);





#endif /* ULTRASONICO_H_ */