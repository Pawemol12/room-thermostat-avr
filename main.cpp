/*
 * Pokojowy regulator temepratury
 * main.cpp
 *
 * Created: 05.08.2020 22:45:04
 * Author : Pawemol (Pawe³ Lodzik)
 */ 
#define F_CPU 16000000UL
#define BUFF_SIZE   100
#define CHECK_DS1621_FIRST_CYCLES   10
#define DS1621_SENSOR_NOT_REACH -862
#define LCD_MAX_CHARS_IN_LINE 16

#define DS1621_MIN_TEMP -55
#define DS1621_MAX_TEMP 125
#define HYSTERESIS_MAX_TEMP 10

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

	sei();
	timer0100us_start();
	outputDataPtr=temperatures; 
	ds1621twi_init();

	char buffer[BUFF_SIZE];
	
	int16_t last_temp = DS1621_SENSOR_NOT_REACH;
	int16_t max_temp = DS1621_SENSOR_NOT_REACH;
	int16_t min_temp = DS1621_SENSOR_NOT_REACH;
	
	LCD_WriteText("Inicjalizacja");
	LCD_GoTo(0, 1);
	LCD_WriteText("Programu");
	
	for (int i = 0; i < CHECK_DS1621_FIRST_CYCLES; i++)
	{
		ds1621StateMachine();
		_delay_ms(100);
	}
	
	bool settingsChanged = false;
	
    while (1) 
    {
		ds1621StateMachine();

		//Wczytywanie temp z czujnika na lcd
		if (max_temp < temperatures[0])
			max_temp = temperatures[0];
			
		if (min_temp == DS1621_SENSOR_NOT_REACH) {
			if (temperatures[0] != 0) {
				min_temp = temperatures[0];
			}
		}
		
		if (min_temp > temperatures[0])
		{
			min_temp = temperatures[0];
		}
		
		float current_temp_float = temperatures[0]/10 + float((temperatures[0]%10))/10;
		float max_temp_float = max_temp/10 + float((max_temp%10))/10;
		float min_temp_float = min_temp/10 + float((min_temp%10))/10;
		
		if (last_temp != temperatures[0] || settingsChanged) {
			settingsChanged = false;
			
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
				
			//Wysy³anie informacji o temperaturze do komputera
			char tempInfoString[50];
			sprintf(tempInfoString, "TEMP_INFO|%.1f|%.1f|%.1f\n", current_temp_float, max_temp_float, min_temp_float);
			uart3_putstr(tempInfoString);
		}
		
		if (uart3_AvailableBytes())
		{
			char outInfoString[200];
			uart3_getln(buffer, BUFF_SIZE);
			
			if (strcmp(buffer, "FETCH_SETTINGS") == 0)
			{
				//Temperatura
				sprintf(outInfoString, "TEMP_INFO|%.1f|%.1f|%.1f\n", current_temp_float, max_temp_float, min_temp_float);
				uart3_putstr(outInfoString);
				
				//Ostereza
				sprintf(outInfoString, "OSTERESIS_SETTINGS_INFO|%.1f\n", hysteresis);
				uart3_putstr(outInfoString);
				
				//Wartoœci przekaŸników
				for (int i = 0; i < TRANSMITERS_COUNT; i++) {
					sprintf(outInfoString, "TRANSMITER_SETTTINGS_INFO|%i|%.1f\n", i, transmiters_threshold[i]);
					uart3_putstr(outInfoString);
				}
				
				//Wartoœci led
				for (int i = 0; i < LEDBAR_LED_COUNT; i++) {
					sprintf(outInfoString, "LED_SETTTINGS_INFO|%i|%.1f\n", i, ledbar_leds_threshold[i]);
					uart3_putstr(outInfoString);
				}
			} else {
				//uart3_putstr(buffer);
				char delim[] = "|";
				char *ptr = strtok(buffer, delim);
				//Ustawianie osterozy
				if (strcmp(ptr, "SET_OSTERESIS") == 0)
				{
					bool hysteresisChanged = false;
					int tokensNumber = 0;
					float hysteresisValue = hysteresis;
					while(ptr != NULL)
					{
						ptr = strtok(NULL, delim);
						
						if (tokensNumber == 0)
						{
							hysteresisValue = atof(ptr);
							
							if (hysteresisValue > 0 && hysteresisValue <= HYSTERESIS_MAX_TEMP) {
								hysteresisChanged = true;
							}
						}
						tokensNumber++;
					}
					
					if (hysteresisChanged) {
						hysteresis = hysteresisValue;
						eeprom_write_block((const void*)&hysteresis, (void*)&hysteresis_eem, sizeof(float));
						settingsChanged = true;
					}
				}
				//Ustawianie wartosci transmitera
				else if (strcmp(ptr, "SET_TRANSMITER_VALUE") == 0)
				{
					bool transmiterValueChanged = false;
					
					int tokensNumber = 0;
					int transmiterNumber = -1;
					float transmiterValue = -1;
					
					while(ptr != NULL)
					{
						ptr = strtok(NULL, delim);
						
						if (tokensNumber == 0)
						{
							transmiterNumber = atoi(ptr);
						}
						else if (tokensNumber == 1)
						{
							transmiterValue = atof(ptr);
							transmiterValueChanged = true;
						}
						tokensNumber++;
					}
					
					if (transmiterValueChanged) {
						if (transmiterNumber >= 0 && transmiterNumber <= TRANSMITERS_COUNT)
						{
							if (transmiterValue >= DS1621_MIN_TEMP && transmiterValue <= DS1621_MAX_TEMP)
							{
								transmiters_threshold[transmiterNumber] = transmiterValue;
								eeprom_write_block((const void*)&transmiters_threshold[transmiterNumber], (void*)&transmiters_threshold_eem[transmiterNumber], sizeof(float));
								settingsChanged = true;
							}
						}
					}
				}
				//Ustawianie wartosci led
				else if (strcmp(ptr, "SET_LED_VALUE") == 0)
				{
					bool ledValueChanged = false;
					
					int tokensNumber = 0;
					int ledNumber = -1;
					float ledValue = -1;
					
					while(ptr != NULL)
					{
						ptr = strtok(NULL, delim);
						
						if (tokensNumber == 0)
						{
							ledNumber = atoi(ptr);
						}
						else if (tokensNumber == 1)
						{
							ledValue = atof(ptr);
							ledValueChanged = true;
						}
						tokensNumber++;
					}
					
					if (ledValueChanged) {
						if (ledNumber >= 0 && ledNumber <= LEDBAR_LED_COUNT)
						{
							if (ledValue >= DS1621_MIN_TEMP && ledValue <= DS1621_MAX_TEMP)
							{
								ledbar_leds_threshold[ledNumber] = ledValue;
								eeprom_write_block((const void*)&ledbar_leds_threshold[ledNumber], (void*)&ledbar_leds_threshold_eem[ledNumber], sizeof(float));
								settingsChanged = true;
							}
						}
					}
				}
			}
		}
    }
}

