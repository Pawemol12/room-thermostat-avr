/*
 * Pokojowy regulator temepratury.cpp
 *
 * Created: 05.08.2020 22:45:04
 * Author : Pawemol
 */ 
#define F_CPU 16000000UL
#define BUFF_SIZE   25
#define DS1621_SENSOR_NOT_REACH -862
#define LCD_MAX_CHARS_IN_LINE 16

//#define USART_EXTEND_RX_BUFFER 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <util/delay.h>
#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atoi */
#include <string.h>

extern "C" {
	
#include "HD44780.h"
#include "ds1621avr.h"
//#include "i2cmaster.h"
//#include "mk_i2c.h"

}
#include "usart.h"
#include "Macros.h"
#include "bits_config.h"



//DS1621
volatile int16_t temperatures[SensorTopAdr+1];

//Konfiguracja EEPROMU
float EEMEM transmiters_threshold_eem[TRANSMITERS_COUNT] = {
	25.0,
	30.0,
	35.0
};

float EEMEM ledbar_leds_threshold_eem[LEDBAR_LED_COUNT] = {
	15.0,
	20.0,
	25.0,
	30.0,
	35.0,
	40.0,
	45.0,
	50.0
};

float EEMEM hysteresis_eem = 1.0;

void timer0100us_start(void) {
	TCCR0B|=(1<<CS01);
	TIMSK0|=(1<<TOIE0);
}

ISR(TIMER0_OVF_vect) {
	tickTWItimeoutCounter();
}
//

int main(void)
{
    TRANSMITERS_DDR = 0xFF;
	LEDBAR_DDR = 0xFF;
	
	//Zczytywanie wartoœci uistawien z epromu
	
	float transmiters_threshold[TRANSMITERS_COUNT];
	float ledbar_leds_threshold[LEDBAR_LED_COUNT];
	float hysteresis;
	
	eeprom_read_block((void*)&transmiters_threshold, (const void*)&transmiters_threshold_eem, sizeof(transmiters_threshold));
	eeprom_read_block((void*)&ledbar_leds_threshold, (const void*)&ledbar_leds_threshold_eem, sizeof(ledbar_leds_threshold));
	eeprom_read_block((void*)&hysteresis, (const void*)&hysteresis_eem, sizeof(float));

	LCD_Initalize();

	uart3_init(BAUD_CALC(9600));
	//stdout = &uart3_io; // attach uart stream to stdout & stdin
	//stdin = &uart3_io; // uart0_in and uart0_out are only available if NO_USART_RX or NO_USART_TX is defined
	
	sei();
	timer0100us_start();
	outputDataPtr=temperatures; 
	ds1621twi_init();

	char buffer[BUFF_SIZE];
	
	int16_t last_temp = DS1621_SENSOR_NOT_REACH;
	int16_t max_temp = DS1621_SENSOR_NOT_REACH;
	
	LCD_WriteText("Inicjalizacja");
	LCD_GoTo(0, 1);
	LCD_WriteText("Programu");
	
    while (1) 
    {
		//uart3_puts("bytes waiting in receiver buffer : ");
		_delay_ms(50);
		
		ds1621StateMachine();
		
		
		//Wczytywanie temp z czujnika na lcd
			
		if (max_temp < temperatures[0])
			max_temp = temperatures[0];
		
		float current_temp_float = temperatures[0]/10 + float((temperatures[0]%10))/10;
		float max_temp_float = max_temp/10 + float((max_temp%10))/10;
		
		if (last_temp != temperatures[0]) {
			last_temp = temperatures[0];
			
			LCD_Clear();
			
			char current_temp_text[LCD_MAX_CHARS_IN_LINE];
			char max_temp_text[LCD_MAX_CHARS_IN_LINE];
			// 
			sprintf(current_temp_text, "Ak. t. %.1f C", current_temp_float);
			//
			sprintf(max_temp_text, "Max. t. %.1f C", max_temp_float);
			
			LCD_WriteText(current_temp_text);
			LCD_GoTo(0, 1);
			LCD_WriteText(max_temp_text);
			
			//Obs³uga przekaŸników
			
			//Za³¹czanie
			if (current_temp_float >= transmiters_threshold[TRANSMITER_0])
				bit_set(TRANSMITERS_PORT, BIT(TRANSMITER_0_PIN));
			
			if (current_temp_float >= transmiters_threshold[TRANSMITER_1])
				bit_set(TRANSMITERS_PORT, BIT(TRANSMITER_1_PIN));
			
			if (current_temp_float >= transmiters_threshold[TRANSMITER_2])
				bit_set(TRANSMITERS_PORT, BIT(TRANSMITER_2_PIN));
			
			//Wy³¹czanie
			if (current_temp_float <= transmiters_threshold[TRANSMITER_0] - hysteresis)
				bit_clear(TRANSMITERS_PORT, BIT(TRANSMITER_0_PIN));
			
			if (current_temp_float <= transmiters_threshold[TRANSMITER_1] - hysteresis)
				bit_clear(TRANSMITERS_PORT, BIT(TRANSMITER_1_PIN));
			
			if (current_temp_float <= transmiters_threshold[TRANSMITER_2] - hysteresis)
				bit_clear(TRANSMITERS_PORT, BIT(TRANSMITER_2_PIN));
			
			//Obs³uga diód
			
			//Za³¹czanie
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_0])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_0_PIN));
			
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_1])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_1_PIN));
			
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_2])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_2_PIN));
			
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_3])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_3_PIN));
			
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_4])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_4_PIN));
			
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_5])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_5_PIN));
			
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_6])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_6_PIN));
			
			if (current_temp_float >= ledbar_leds_threshold[LEDBAR_LED_7])
				bit_set(LEDBAR_PORT, BIT(LEDBAR_LED_7_PIN));
			
			//Wy³¹cznie diód
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_0] - hysteresis)
				bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_0_PIN));
			
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_1] - hysteresis)
			bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_1_PIN));
			
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_2] - hysteresis)
				bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_2_PIN));
			
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_3] - hysteresis)
				bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_3_PIN));
			
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_4] - hysteresis)
				bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_4_PIN));
			
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_5] - hysteresis)
				bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_5_PIN));
			
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_6] - hysteresis)
				bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_6_PIN));
			
			if (current_temp_float <= ledbar_leds_threshold[LEDBAR_LED_7] - hysteresis)
				bit_clear(LEDBAR_PORT, BIT(LEDBAR_LED_7_PIN));
		}
		
		if (uart3_AvailableBytes())
		{
			uart3_gets(buffer, BUFF_SIZE);
			uart3_putstr(buffer);
		}
		
		
		/*uart0_puts("bytes waiting in receiver buffer : ");
		uart1_puts("bytes waiting in receiver buffer : ");
		uart2_puts("bytes waiting in receiver buffer : ");*/
		//uart3_puts("test");
		//uart3_puts("\r\n");
		//uart_putc('>');
		//uart_puts_P(" Googles");
		//uart_puts("test");
		
		//uart3_putint(uart3_AvailableBytes()); // ask for bytes waiting in receiver buffer
		
		//uart3_getln(buffer, BUFF_SIZE); // read 24 bytes or one line from usart buffer
		//char test[BUFF_SIZE];
		//sprintf(test, " test - %s", buffer);
		
		/*LCD_GoTo(0, 1);
		LCD_WriteText(test);
		
		_delay_ms(500);
		LCD_Clear();*/
		/*int temp = temperatures[0];

		if (temperatures[0] > 0)
		{
			LCD_WriteText("TEST");
		}

		if (temperatures[0] < 0)
		{
			LCD_WriteText("Miejsze");
		}

		if (temperatures[0] == 0)
		{
			LCD_WriteText("Null");
		}
		char temp_char[5];
		sprintf(temp_char, "%d", temperatures[0]);
		LCD_GoTo(0, 1);
		LCD_WriteText(temp_char);*/
		//LCD_WriteText("TEST");
		/*PORTF = 0xFF;
		_delay_ms(1000);
		PORTF = 0x00;
		_delay_ms(1000);*/
    }
}

