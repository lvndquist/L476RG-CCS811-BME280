/**
******************************************************************************
@brief header for the office environment monitor project.
@details bla bla bla
@file office_environment_monitor.h
@author  Jonatan Lundqvist Silins, jonls@kth.se
@author  Sebastian Divander,       sdiv@kth.se
@date 29-03-2021
@version 1.0
******************************************************************************
*/

#include "stdio.h"
#include "ESP8266.h"
#include "CCS811_BME280.h"
#include "ssd1306.h"

/* Status codes */
typedef enum
{
	ESP8266_START_SUCCESS = 0,
	ESP8266_START_ERROR,
	ESP8266_WIFI_CON_SUCCESS,
	ESP8266_WIFI_CON_ERROR,
	ESP8266_WEB_CONNECTED,
	ESP8266_WEB_DISCONNECTED,
	ESP8266_WEB_REQUEST_SUCCESS,
	ESP8266_WEB_REQUEST_ERROR,
	CCS811_START_SUCCESS,
	CCS811_START_ERROR,
	CCS811_RUNNING_ERROR,
	BME280_START_SUCCESS,
	BME280_START_ERROR
	// Environment sensor status codes go here
	// Distance sensor status codes go here
} RETURN_STATUS;

/**
 * @brief initiate the display
 * @param void
 * @return void
 */
void display_startscreen(void);

/**
 * @brief waits until data from the CCS811 sensor is available. Implements a loading animation.
 * @param void
 * @return void
 */
void display_getting_data_screen(void);

/**
 * @brief shows the temperature, humidity, co2 and tVoc values on the display
 * @param float temp, the temperature
 * @param float hum, the humidity
 * @param uint16_t co2, the CO2 value
 * @param uint16_t tVoc, the Tvoc value
 * @return void
 */
void show_measurements(float temp, float hum, uint16_t co2, uint16_t tVoc);

/**
 * @brief main program
 * @param void
 * @return void
 */
void office_environment_monitor(void);

/**
 * @brief error handler, which handles all errors defined in the RETURN_STATUS enum.
 * 		  Most errors will just print some message to the display and then freeze.
 * @param void
 * @return void
 */
void error_handler(void);

/**
 * @brief initiates the esp8266. Enables interrupts for uart4 and checks the return status.
 * @param void
 * @return RETURN_STATUS, either ESP8266_START_ERROR or ESP8266_START_SUCCESS
 */
RETURN_STATUS esp8266_start(void);

/**
 * @brief connects the esp8266 to wifi, and checks the return status.
 * @param void
 * @return RETURN_STATUS, either ESP8266_WIFI_CON_ERROR or ESP8266_WIFI_CON_SUCCESS
 */
RETURN_STATUS esp8266_wifi_start(void);

/**
 * @brief establishes a tcp connection to the project website.
 * @param void
 * @return RETURN_STATUS, either ESP8266_WEB_DISCONNECTED or ESP8266_WEB_CONNECTED
 */
RETURN_STATUS esp8266_web_connection(void);

/**
 * @brief sends a http post request to the project website. Data passed in the parameters are posted to the website.
 * @param uint16_t co2, CO2 value
 * @param uint16_t tvoc, tVOC value
 * @param float temp, temperature value
 * @param float hum, humidity value
 * @return RETURN_STATUS, either ESP8266_WEB_REQUEST_ERROR or ESP8266_WEB_REQUEST_SUCCESS
 */
RETURN_STATUS esp8266_web_request(uint16_t co2, uint16_t tvoc, float temp, float hum);

/**
 * @brief initiates the ccs811 with all settings needed for environmental measurements each second.
 * @param void
 * @return RETURN_STATUS, either CCS811_START_ERROR or CCS811_START_SUCCESS
 */
RETURN_STATUS ccs811_start(void);

/**
 * @brief initiates the bme280 to measure temperature and humidity.
 * @param void
 * @return RETURN_STATUS, either BME280_START_ERROR or BME280_START_SUCCESS.
 */
RETURN_STATUS bme280_start(void);

void transmit(const char* command);
