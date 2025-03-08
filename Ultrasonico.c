/*
 * ultrasound.c
 *
 * Created: 1/03/2025 17:12:44
 *  Author: pablo
 */
#include "Ultrasonico.h"
#include <stdlib.h>  // Para itoa()
#include <avr/interrupt.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// Variables para medir el pulso mediante PCINT
volatile uint16_t echo_start = 0;
volatile uint16_t echo_end = 0;
volatile uint8_t echo_captured = 0; // 0: no capturado, 1: flanco ascendente, 2: flanco descendente

// Timeout para la medicio?n (valor ajustable)
#define TIMEOUT_COUNT 60000UL

// ISR para interrupcio?n de cambio de pin en PORTB (PCINT0_vect cubre PB0-PB7)
ISR(PCINT0_vect) {
    // Verifica el estado del pin Echo (PB5)
    if (ULTRASONIC_ECHO_PIN_IN & (1 << ULTRASONIC_ECHO_PIN)) {
        // Flanco ascendente
        if (echo_captured == 0) {
            echo_start = TCNT1;
            echo_captured = 1;
        }
    } else {
        // Flanco descendente
        if (echo_captured == 1) {
            echo_end = TCNT1;
            echo_captured = 2;
        }
    }
}

//
// Funciones UART (para depuracio?n, si se desea)
//
void UART_Init(uint16_t baud) {
    uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << TXEN0); // Habilita el transmisor
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits, 1 stop, sin paridad
}

void UART_SendChar(char data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data;
}

void UART_SendString(const char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

void UART_SendNumber(uint16_t num) {
    char buffer[6];  // Suficiente para nu?meros hasta 65535
    itoa(num, buffer, 10);
    UART_SendString(buffer);
}

//
// Inicializa Timer1 en modo normal (sin usar Input Capture)
// Se usara? para medir el tiempo entre flancos en PB5 mediante PCINT.
// Se configura con prescaler 8: cada tick equivale a 0.5 탎 con F_CPU=16MHz.
static void Timer1_Init(void) {
    TCCR1A = 0; // Modo normal
    TCCR1B = (1 << CS11); // Prescaler 8
    TCNT1 = 0; // Reinicia el contador
    
    // Habilita interrupciones de cambio de pin en PORTB para PB5:
    PCICR |= (1 << PCIE0);   // Habilita interrupcio?n de cambio de pin para PORTB
    PCMSK0 |= (1 << PCINT5); // Habilita interrupcio?n solo para PB5
    sei(); // Habilita interrupciones globales
}

//
// Funcio?n para medir el ancho del pulso usando Timer1 y PCINT.
// Se espera que el flanco ascendente y luego el descendente se capturen mediante la ISR.
// Devuelve el ancho del pulso en microsegundos.
static uint32_t measurePulseWidth(void) {
    uint16_t diff;
    uint32_t timeout = TIMEOUT_COUNT;
    
    echo_captured = 0;
    TCNT1 = 0; // Reinicia Timer1
    
    // Espera a que se capture el flanco ascendente y luego el descendente
    while (echo_captured != 2 && timeout--) {}
    if (echo_captured != 2) return 0; // Timeout: no se capturo? el pulso completo
    
    // Calcula la diferencia, considerando posible desbordamiento (pulsos cortos, normalmente sin overflow)
    if (echo_end >= echo_start)
        diff = echo_end - echo_start;
    else
        diff = (0xFFFF - echo_start) + echo_end;
    
    // Con prescaler 8 y F_CPU = 16MHz, cada tick dura 0.5 탎.
    // Por lo tanto, el ancho del pulso (en 탎) es: diff * 0.5
    return diff / 2;
}

//
// Inicializa el sensor ultraso?nico:
// - Configura el pin Trigger (PB4) como salida
// - Configura el pin Echo (PB5) como entrada
// - Inicializa Timer1 y la interrupcio?n de cambio de pin
// - Inicializa la UART (opcional, para depuracio?n)
void Ultrasound_Init(void) {
    UART_Init(9600);
    
    // Configura Trigger como salida
    ULTRASONIC_TRIGGER_DDR |= (1 << ULTRASONIC_TRIGGER_PIN);
    // Inicializa Trigger en bajo
    ULTRASONIC_TRIGGER_PORT &= ~(1 << ULTRASONIC_TRIGGER_PIN);
    
    // Configura Echo como entrada
    ULTRASONIC_ECHO_DDR &= ~(1 << ULTRASONIC_ECHO_PIN);
    
    // Inicializa Timer1 y PCINT para medir el pulso
    Timer1_Init();
}

//
// Lee la distancia midiendo el pulso de Echo y aplicando la fo?rmula:
//   distancia (cm) = (t (탎) * 0.0343) / 2  ? t * 0.01715
// Para aritme?tica entera: distance = (t * 1715) / 100000
uint16_t Ultrasound_ReadDistance(void) {
    uint32_t pulseWidthMicrosec;
    uint16_t distance;
    
    // Envi?a un pulso de 10 탎 en Trigger
    ULTRASONIC_TRIGGER_PORT |= (1 << ULTRASONIC_TRIGGER_PIN);
    _delay_us(10);
    ULTRASONIC_TRIGGER_PORT &= ~(1 << ULTRASONIC_TRIGGER_PIN);
    
    // Mide el ancho del pulso en Echo (en 탎)
    pulseWidthMicrosec = measurePulseWidth();
    
    // Convierte el ancho del pulso a distancia en cm usando aritme?tica entera
    distance = (uint16_t)((pulseWidthMicrosec * 1715UL) / 100000UL);
    
    return distance;
}

//
// Funcio?n de depuracio?n: envi?a la distancia medida por UART
//
void Ultrasound_Task(void) {
    uint16_t dist = Ultrasound_ReadDistance();
    UART_SendString("Distancia: ");
    UART_SendNumber(dist);
    UART_SendString(" cm\r\n");
}