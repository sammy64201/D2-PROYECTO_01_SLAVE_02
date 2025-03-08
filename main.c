/*
 * D2_PROYECTO_01_SLAVE_02.c
 *
 * Created: 3/6/2025 3:06:56 PM
 * Author : samue
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>   // Para itoa
#include <stdio.h>

#include "I2C/I2C.h"
#include "Ultrasonico/Ultrasonico.h"
#include "UART/UART.h"
#define SlaveAddress 0x29
#define DISTANCIA_OBJETIVO 17

char buffer[16];
uint16_t distancia;
volatile uint8_t en_retroceso = 0;  
volatile uint8_t sistema_riego_activado = 0;  
volatile uint8_t modo_operacion = 0;
uint8_t bandera;


void setup(){
	cli();
	DDRC |= (1 << PC0) | (1 << PC1);
	DDRC &= ~(1 << PC2);
	PORTC |= (1 << PC2);
	Ultrasound_Init();
	I2C_Slave_Init(SlaveAddress);
// 	EICRA |= (1 << ISC01);
// 	EIMSK |= (1 << INT0);

	sei();
	
}

int main(void)
{
    setup();
	
    while (1) 
    {
		
		 
		 if (modo_operacion == 1){
			 bandera = 1;
		 } else if (modo_operacion == 0){
			 bandera = 0;
		 }
		 
		distancia = Ultrasound_ReadDistance();
		
		
		if (bandera == 1) {
			if (!en_retroceso) {
				if (distancia < DISTANCIA_OBJETIVO) {
					// Avanzar el motor 
					PORTC |= (1 << PC0);
					PORTC &= ~(1 << PC1);
					} else {
					// Detener el motor
					PORTC &= ~(1 << PC0);
					PORTC &= ~(1 << PC1);
					en_retroceso = 1;  
				}
				} else {
				if (distancia > 1) {  
					// Retroceder el motor 
					PORTC &= ~(1 << PC0);
					PORTC |= (1 << PC1);
					} else {
					
					PORTC &= ~(1 << PC0);
					PORTC &= ~(1 << PC1);
					en_retroceso = 0;  // Reset para la siguiente vez
				}
			}
		}
		else {
			
			PORTC &= ~(1 << PC0); // Motor apagado
			PORTC &= ~(1 << PC1);
			sistema_riego_activado = 0;
		}
			
    }
}


ISR(INT0_vect) {
	// Detener el motor inmediatamente
	PORTC &= ~(1 << PC0);
	PORTC &= ~(1 << PC1);
	
	// Activar modo retroceso
	en_retroceso = 1;
	sistema_riego_activado = 0;  // Apagar el riego
}

ISR(TWI_vect){
	uint8_t estado;
	estado = TWSR & 0xFC;
	
	switch(estado){
		case 0x60:  // Dirección recibida con escritura
		case 0x70:
		TWCR |= (1 << TWINT);
		break;
		
		case 0x80:  // Datos recibidos del maestro
		case 0x90:
		modo_operacion = TWDR;  // Recibe el comando del maestro (1 o 0)
		TWCR |= (1 << TWINT);
		break;
		
		case 0xA8:  // Maestro solicita datos
		case 0xB8:
		TWDR = distancia;  // Enviar la distancia medida sin importar el estado
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;
		
		default:  // Error o fin de comunicación
		TWCR |= (1 << TWINT) | (1 << TWSTO);
		break;
	}
}
