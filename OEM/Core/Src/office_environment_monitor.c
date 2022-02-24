/**
******************************************************************************
@brief main program for the office environment monitor project.
@details bla bla bla
@file office_environment_monitor.c
@author  Jonatan Lundqvist Silins, jonls@kth.se
@author  Sebastian Divander,       sdiv@kth.se
@date 29-03-2021
@version 1.0
******************************************************************************
*/


#include "office_environment_monitor.h"

#define CCS811_BME280_SEND_INTERVAL 30

/* Current return statuses */
static RETURN_STATUS 	 current_status;			// return status for functions within this program
static ENV_SENSOR_STATUS current_sensor_status; // return status for environmental sensor functions
static const char* esp8266_return_string;

/* Temperature and humidity */
static float	 		 temperature;
static float			 humidity;

void office_environment_monitor(void){

	/* Initiate display, if the init fails, leds will flash and it will try to init again */
	display_init();
	display_startscreen();
	HAL_Delay(2000);
	reset_screen_canvas();

	/* Initiate the wifi module */
	display_write_string("Starting ESP8266", WHITE);
	current_status = esp8266_start();
	if(current_status == ESP8266_START_ERROR){
		error_handler();
	}
	display_set_position(1, (display_get_y() + ROW_SIZE));
	display_write_string("STARTED", WHITE);
	reset_screen_canvas();

	/* Connect to WIFI */
	display_write_string("Connecting to WIFI", WHITE);
	current_status = esp8266_wifi_start();
	if(current_status == ESP8266_WIFI_CON_ERROR){
		error_handler();
	}
	display_set_position(1, (display_get_y() + ROW_SIZE));
	display_write_string(esp8266_return_string, WHITE);
	reset_screen_canvas();

	/* Initiate CCS811 for CO2 and tVOC measurements */
	display_write_string("Starting CCS811", WHITE);
	current_status = ccs811_start();
	if(current_status == CCS811_START_ERROR){
		error_handler();
	}
	display_set_position(1, (display_get_y() + ROW_SIZE));
	display_write_string("STARTED", WHITE);
	reset_screen_canvas();

	/* Initiate BME280 for humidity and temperature */
	display_write_string("Starting BME280", WHITE);
	current_status = bme280_start();
	if(current_status == BME280_START_ERROR){
		reset_screen_canvas();
		display_write_string("BME280 ERROR", WHITE);
		error_handler();
	}
	display_set_position(1, (display_get_y() + ROW_SIZE));
	display_write_string("STARTED", WHITE);
	reset_screen_canvas();

	/* Loading bar when waiting for some sensor data to be available */
	display_getting_data_screen();

	uint8_t timer = 0;
	for(;;){

		// TODO: BLINK GREEN LED WHILE RUNNING
		if(CCS811_data_available() == CCS811_NEW_DATA){

			timer++;
			CCS811_read_alg_res();
			temperature = BME280_read_temp();
			humidity = BME280_read_hum();
			CCS811_set_temp_hum(temperature, humidity);
			uint16_t co2 = CCS811_get_co2();
			uint16_t tVoc = CCS811_get_tvoc();

			show_measurements(temperature, humidity, co2, tVoc);

			if(timer == CCS811_BME280_SEND_INTERVAL){
				timer = 0;
				if((current_status = esp8266_web_connection()) != ESP8266_WEB_CONNECTED)
					error_handler();
				if((current_status = esp8266_web_request(co2, tVoc, temperature, humidity)) != ESP8266_WEB_REQUEST_SUCCESS)
					error_handler();
			}
		}
		/* Check for CCS811 errors */
		else if(CCS811_read_status_error()){
			current_status = CCS811_RUNNING_ERROR;
			error_handler();
		}
	}
}


void error_handler(void){

	char buf[28];
	reset_screen_canvas();
	switch (current_status) {

		case ESP8266_START_ERROR:
			 display_write_string_no_update("ESP8266 START ERR", WHITE);
			 display_string_on_line_no_update("Check connections", WHITE, 2);
			 display_update();

		case ESP8266_WIFI_CON_ERROR:
			 display_string_on_line_no_update("WIFI CON ERROR:", WHITE, 1);
			 display_string_on_line_no_update(esp8266_return_string, WHITE, 2);
			 display_update();
			 break;

		case ESP8266_WEB_DISCONNECTED:
			 display_write_string_no_update("WEB FAIL:", WHITE);
			 display_string_on_line_no_update(esp8266_return_string, WHITE, 2);
			 display_update();
			 break;

		case ESP8266_WEB_REQUEST_ERROR:
			 display_write_string_no_update("WEB REQUEST FAIL", WHITE);
			 display_string_on_line_no_update(esp8266_return_string, WHITE, 2);
			 display_update();
			 break;

		case CCS811_START_ERROR:
			 display_write_string_no_update("CCS811 START ERROR", WHITE);
			 if(current_sensor_status == CCS811_I2C_ERROR){
				 display_string_on_line_no_update("I2C FAILURE", WHITE, 2);
			 	 display_string_on_line_no_update("CHECK CONNECTIONS!", WHITE, 3);
			 	 display_string_on_line_no_update("OR RESET...", WHITE, 4);
			 }
			 display_update();
			 break;

		case CCS811_RUNNING_ERROR:
			 sprintf (buf, "%d", CCS811_read_error_id());
			 display_write_string_no_update("CCS811 RUNNING ERR", WHITE);
			 display_string_on_line_no_update("ERROR CODE:", WHITE, 2);
			 display_string_on_line_no_update(buf, WHITE, 3);
			 display_update();
			 break;

		case BME280_START_ERROR:
			 display_write_string_no_update("BME280 START ERROR", WHITE);
			 if(current_sensor_status == BME280_I2C_ERROR || current_sensor_status == BME280_ID_ERR){
				 display_string_on_line_no_update("I2C FAILURE", WHITE, 2);
			 	 display_string_on_line_no_update("CHECK CONNECTIONS!", WHITE, 3);
			 }
			 display_update();
			 break;

		default:
			display_write_string_no_update("UNKNOWN ERROR", WHITE);
			display_string_on_line_no_update("PLEASE RESTART", WHITE, 2);
			display_update();
			break;

	}
	/* Errors printed, freeze here */
	while(1){
		// TODO: BLINK RED LED WHILE RUNNING
	}
}

void display_startscreen(void){
	display_set_position(1, (display_get_y() + ROW_SIZE));
	display_write_string("Office            "
						 "Environment       "
						 "Monitor           ", WHITE);
}

void display_getting_data_screen(void){

	uint8_t init_count = 0;
	uint8_t init_count_limit = 0;

	display_write_string("GETTING DATA", WHITE);
	display_set_position(1, (display_get_y() + ROW_SIZE));

	/* Loop until sensor data is available */
	do {
		if(init_count == 100){

			/* If 18 # have been printed, reset the line and start printing from the beginning of the line */
			if(init_count_limit == 18){
				uint16_t y = display_get_y();
				display_set_position(1, y);
				display_write_string("                  ", WHITE);
				display_set_position(1, y);
				init_count_limit = 0;
			}
			display_write_string("#", WHITE);
			init_count_limit++;
		}
		init_count++;
	} while (CCS811_data_available() == CCS811_NO_NEW_DATA);
	reset_screen_canvas();
}

void show_measurements(float temp, float hum, uint16_t co2, uint16_t tVoc){

	/* String buffers */
	char buffer    [38] = {};
	char tempbuffer[15] = {};
	char humbuffer [15] = {};
	char co2buffer [19] = {};
	char tvocbuffer[19] = {};

	/* Make temperature and humidity output and print on screen */
	snprintf (tempbuffer, 15, "Temp: %f" , temp);
	snprintf (humbuffer,  15, "Hum:  %f" , hum);
	sprintf  (buffer, "%s    %s    ", tempbuffer, humbuffer);
	display_write_string_no_update(buffer, WHITE);

	/* Make co2 output and print on screen */
	sprintf  (co2buffer, "CO2:  %dppm   ", co2);
	display_write_string_no_update(co2buffer, WHITE);
	display_set_position(1, (display_get_y() + ROW_SIZE));

	/* Make tVOC output and print on screen */
	sprintf  (tvocbuffer, "tVoc: %dppb   ", tVoc);
	display_write_string_no_update(tvocbuffer, WHITE);
	display_set_position(1, 1);
	display_update();
}

/* Initiates the wifi module */
RETURN_STATUS esp8266_start(void){

	/* Module needs to return OK else an error has occurred */
	esp8266_return_string = esp8266_init();
	if(strcmp(esp8266_return_string, ESP8266_AT_OK) != 0){
		return ESP8266_START_ERROR;
	}
	return ESP8266_START_SUCCESS;

}

/* Connects the module to wifi */
RETURN_STATUS esp8266_wifi_start(void){

	/* Module needs to return WIFI CONNECTED else an error has occurred */
	esp8266_return_string = esp8266_wifi_init();
	if(strcmp(esp8266_return_string, ESP8266_AT_WIFI_CONNECTED) != 0){
		return ESP8266_WIFI_CON_ERROR;
	}
	return ESP8266_WIFI_CON_SUCCESS;

	/* This code can be used to get an animation, but it is not very reliable, havent figured out why tho
	uint8_t init_count = 0;
	uint8_t init_count_limit = 0;
	char wifi_command[256] = {0};

	// Build the command
	esp8266_get_wifi_command(wifi_command);

	esp8266_clear();
	HAL_Delay(100);
	transmit(wifi_command);

	display_set_position(1, (display_get_y() + ROW_SIZE));

	// Loop until sensor data is available
	do {
		if(init_count == 200){
			// If 18 # have been printed, reset the line and start printing from the beginning of the line
			if(init_count_limit == 18){
				uint16_t y = display_get_y();
				display_set_position(1, y);
				display_write_string("                  ", WHITE);
				display_set_position(1, y);
				init_count_limit = 0;
			}
				display_write_string("#", WHITE);
				init_count_limit++;
		}
		if(strstr(rx_buffer, ESP8266_AT_ERROR) != NULL){
			break;
		}

		if(strstr(rx_buffer, ESP8266_AT_FAIL) != NULL){
			break;
		}

		init_count++;
	}while (strstr(rx_buffer, ESP8266_AT_OK_TERMINATOR) == NULL);

	esp8266_return_string = get_return(wifi_command);

	reset_screen_canvas();

	if(strcmp(esp8266_return_string, ESP8266_AT_WIFI_CONNECTED) != 0){
			return ESP8266_WIFI_CON_ERROR;
	}
	return ESP8266_WIFI_CON_SUCCESS;
	*/
}

/* Start connection to website */
RETURN_STATUS esp8266_web_connection(void){
	char connection_command[256] = {0};
	char remote_ip[] 			 = "ii1302-project-office-enviroment-monitor.eu-gb.mybluemix.net";
	char type[] 				 = "TCP";
	char remote_port[] 		     = "80";

	esp8266_get_connection_command(connection_command, type, remote_ip, remote_port);
	esp8266_return_string = esp8266_send_command(connection_command);
	if(strcmp(esp8266_return_string, ESP8266_AT_CONNECT) != 0){
		return ESP8266_WEB_DISCONNECTED;
	}
	return ESP8266_WEB_CONNECTED;
}

RETURN_STATUS esp8266_web_request(uint16_t co2, uint16_t tvoc, float temp, float hum){
	//"GET /api/sensor HTTP/1.1\r\nHost: ii1302-project-office-enviroment-monitor.eu-gb.mybluemix.net\r\nConnection: close\r\n\r\n";
	///api/sensor/airquality?carbon=10&volatile=10 HTTP/1.1
	char request	[512] = {0};
	char init_send	[64]  = {0};
	char data		[100]  = {0};
	char uri		[50]  = "/api/sensor?";
	char host		[  ]  = "ii1302-project-office-enviroment-monitor.eu-gb.mybluemix.net";

	sprintf  (data, "carbon=%d&volatile=%d&temperature=%f&humidity=%f", co2, tvoc, temp, hum);
	strcat   (uri,data);

	uint8_t len = esp8266_http_get_request(request, HTTP_POST, uri, host);
	esp8266_get_at_send_command(init_send, len);

	esp8266_return_string = esp8266_send_command(init_send);
	if(strcmp(esp8266_return_string, ESP8266_AT_SEND_OK) != 0){
		return ESP8266_WEB_REQUEST_ERROR;
	}

	esp8266_return_string = esp8266_send_data(request);
	if(strcmp(esp8266_return_string, ESP8266_AT_CLOSED) != 0){
		return ESP8266_WEB_REQUEST_ERROR;
	}
	return ESP8266_WEB_REQUEST_SUCCESS;;
}

/* Initiate CCS811 */
RETURN_STATUS ccs811_start(void){

	uint8_t register_value = 0;
	current_sensor_status = CCS811_SUCCESS;
	HAL_StatusTypeDef hal_status = HAL_OK;
	display_set_position(1, (display_get_y() + ROW_SIZE));

	RETRY:
	HAL_Delay(100);
	register_value = 0;
	hal_status = HAL_OK;

	/* Read the HW ID register to make sure the sensor is responsive */
	// i2c mem read here, otherwise i2c seems to break
	hal_status = HAL_I2C_Mem_Read(&hi2c1, CCS811_ADDR, 0x20, 1, &register_value, 1, 500);

	/* If we fail here, try again. If the module is connected correctly it should work eventually */
	if((hal_status != HAL_OK) || (register_value != 0x81)){
		HAL_Delay(100);
		goto RETRY;
	}
	display_write_string("##", WHITE);

	/* Reset the device & wait a bit */
	current_sensor_status = CCS811_reset();
	if(current_sensor_status != CCS811_SUCCESS)
		return CCS811_START_ERROR;
	HAL_Delay(30);
	display_write_string("##", WHITE);

	/* Check for sensor errors */
	if(CCS811_read_status_error() != 0){
		return CCS811_START_ERROR;
	}
	HAL_Delay(30);
	display_write_string("##", WHITE);

	/* Check if app is valid */
	if(CCS811_read_app_valid() != 1)
		return CCS811_START_ERROR;
	HAL_Delay(30);
	display_write_string("##", WHITE);


	/* Write to app start register to start */
	current_sensor_status = CCS811_app_start();
	if(current_sensor_status != CCS811_SUCCESS)
		return CCS811_START_ERROR;
	HAL_Delay(30);
	display_write_string("##", WHITE);


	/* Set drive mode to 1; measurement each second
	 * mode 2; measurement every 10 seconds
	 * mode 3; measurement every 60 seconds
	 * mode 4; measurement every 250ms
	 **/
	current_sensor_status = CCS811_write_mode(1);
	if(current_sensor_status != CCS811_SUCCESS)
		return CCS811_START_ERROR;
	HAL_Delay(30);
	display_write_string("##", WHITE);

	/* Check for sensor errors before exiting */
	if(CCS811_read_status_error() != 0){
		return CCS811_START_ERROR;
	}
	HAL_Delay(30);
	display_write_string("##", WHITE);

	return CCS811_START_SUCCESS;
}

/* Initiate BME280 */
RETURN_STATUS bme280_start(void){

	current_sensor_status = BME280_init();
	if(current_sensor_status != BME280_SUCCESS){
			return BME280_START_ERROR;
	}
	return BME280_START_SUCCESS;
}

void transmit(const char* command){
	HAL_UART_Transmit(&huart4, (uint8_t*) command, strlen(command), 100);
}
