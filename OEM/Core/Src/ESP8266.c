/**
******************************************************************************
@brief functions for the ESP8266 wifi-module
@details The functions implemented here should be everything needed to
		 use the ESP8266 wifi-module with the Nucleo 476RG board.
		 For each command sent the esp8266 response is returned as a string.
		 Most basic AT commands for the module are implemented. The main purpose
		 of the code is to support the office environment program, thus the code
		 leaves some functionalities of the ESP unimplemented.

		 Functions for easily initiating the module and connecting it to wifi are
		 present. But for connecting to a website, and sending data by using for
		 example HTTP requests, some user code is needed. The user needs to define
		 the website to connect to, and then supply the data to send. There are
		 helper functions which do all the work such as formatting and sending the
		 actual data. To see some examples of how the functions should be used,
		 see the unit_test.c file.

@file ESP8266.c
@author  Jonatan Lundqvist Silins, jonls@kth.se
@date 06-04-2021
@version 2
*******************************************************************************/
#include "ESP8266.h"

/* Global variables */
static uint8_t rx_variable;
static uint8_t rx_buffer_index = 0;
static bool error_flag = false;
static bool fail_flag = false;
static char rx_buffer[RX_BUFFER_SIZE]; //rx recieve buffer for handling all the ESP8266 data it sends back

void
init_uart_interrupt(void){
	HAL_UART_Receive_IT(&huart4, &rx_variable, 1);	// change &huart4 to whatever handler you need
}

/* Probably not the most efficient solution
 * each time a byte is received, it is put into the rx_buffer
 * the rx_buffer is used to check for different responses from the esp8266
 */
void
HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
   if (huart->Instance == UART4) {					 // change UART4 to whatever handler you are using
      rx_buffer[rx_buffer_index++] = rx_variable;    // Add 1 byte to rx_Buffer
   }
   HAL_UART_Receive_IT(&huart4, &rx_variable, 1); // Clear flags and read next byte
}

/* djb2 hashing algorithm which is used in mapping sent commands to the right ESP8266 response code.
   an alternative to using this would be to use some enums or defines instead	 	 	 	 	 	 	 	 */
const unsigned long
hash(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}


/* DMA would probably have been a better fit for this...? */
const char*
esp8266_send_command(const char* command){

	esp8266_clear();
	HAL_UART_Transmit(&huart4, (uint8_t*) command, strlen(command), 100);

	// wait for OK or ERROR/FAIL
	while((strstr(rx_buffer, ESP8266_AT_OK_TERMINATOR) == NULL)){
		if(strstr(rx_buffer, ESP8266_AT_ERROR) != NULL){
			error_flag = true;
			break;
		}
		if(strstr(rx_buffer, ESP8266_AT_FAIL) != NULL){
			fail_flag = true;
			break;
		}
		if(strstr(rx_buffer, "rst") != NULL){
			fail_flag = true;
			break;
		}
	}

	//return evaluate(); would more efficient but not as clear in debugging//error handling
	return get_return(command);
}

const char*
esp8266_send_data(const char* data){

	/* if the function is called after an error, cancel */
	if(error_flag || fail_flag)
		return ESP8266_AT_ERROR;

	rx_buffer_index = 0;

	memset(rx_buffer, 0, RX_BUFFER_SIZE);
	HAL_UART_Transmit(&huart4, (uint8_t*) data, strlen(data), 100);

	while((strstr(rx_buffer, ESP8266_AT_CLOSED) == NULL));

	return ESP8266_AT_CLOSED;
}

const char*
esp8266_init(void){

	/* Init the uart to use here*/
	MX_UART4_Init();
	HAL_Delay(100);

	/* Enable interrupts for UART4 */
	init_uart_interrupt();
	HAL_Delay(100);

	/* Get OK from esp8266 */
	if(strcmp(esp8266_send_command(ESP8266_AT), ESP8266_AT_OK) != 0)
		return ESP8266_AT_ERROR;

	/* Esp8266 sends lots of data when first started */
	HAL_Delay(500);
	/* Reset the esp8266 */
	if(strcmp(esp8266_send_command(ESP8266_AT_RST), ESP8266_AT_OK) != 0){
		return ESP8266_AT_ERROR;
	}

	/* Get OK from esp8266 */
	if(strcmp(esp8266_send_command(ESP8266_AT), ESP8266_AT_OK) != 0)
		return ESP8266_AT_ERROR;

	/* Disconnect the esp8266 if it auto connects... */
	/* seems to break the module when ran quickly, not sure why so just
	 * leave it out. If the module does autoconnect, send ESP8266_AT_CWAUTOCONN.
	 * The autoconn command also seems to be problematic though...
	 *
	 *  if(strcmp(esp8266_send_command(ESP8266_AT_CWQAP), ESP8266_AT_OK) != 0)
	 *	  return ESP8266_AT_ERROR;
	 */

	/* Set the esp8266 to client mode */
	if(strcmp(esp8266_send_command(ESP8266_AT_CWMODE_STATION_MODE), ESP8266_AT_OK) != 0)
		return ESP8266_AT_ERROR;

	/* Verify that the esp8266 is configured as client */
	if(strcmp(esp8266_send_command(ESP8266_AT_CWMODE_TEST), ESP8266_AT_CWMODE_1) != 0)
		return ESP8266_AT_ERROR;

	/* Set the esp8266 to use single mode connection */
	if(strcmp(esp8266_send_command(ESP8266_AT_CIPMUX_SINGLE), ESP8266_AT_OK) != 0)
		return ESP8266_AT_ERROR;

	/* Verify that the esp8266 is configured as single mode*/
	if(strcmp(esp8266_send_command(ESP8266_AT_CIPMUX_TEST), ESP8266_AT_CIPMUX_0) != 0)
		return ESP8266_AT_ERROR;

	/* No errors, return OK */
	return ESP8266_AT_OK;
}

const char*
esp8266_wifi_init(void){

	/* We do a little waiting */
	HAL_Delay(100);

	/* Buffers */
	char wifi_command[256] = {0};

	/* Build the command */
	esp8266_get_wifi_command(wifi_command);

	/* Connect and return result */
	return esp8266_send_command(wifi_command);
}

void
esp8266_clear(void){
	rx_buffer_index = 0;
	error_flag = false;
	fail_flag = false;
	memset(rx_buffer, 0, RX_BUFFER_SIZE);
}

void
esp8266_get_wifi_command(char* ref){
	sprintf (ref, "%s\"%s\",\"%s\"\r\n", ESP8266_AT_CWJAP_SET, SSID, PWD);
}

void
esp8266_get_connection_command(char* ref, char* connection_type, char* remote_ip, char* remote_port){
	sprintf(ref, "%s\"%s\",\"%s\",%s\r\n", ESP8266_AT_START, connection_type, remote_ip, remote_port);
}

void
esp8266_get_at_send_command(char* ref, uint8_t len){
	sprintf(ref, "%s%d\r\n", ESP8266_AT_SEND, len);
}

uint8_t
esp8266_http_get_request(char* ref, const char* http_type, char* uri, char* host){
	sprintf(ref, "%s%s %s\r\n%s%s\r\n%s\r\n\r\n", http_type, uri, HTTP_VERSION, HTTP_HOST, host, HTTP_CONNECTION_CLOSE); // formatting and concatenating http request
	return (strlen(ref)); // return the length of the request, the length needs to be specified before data can be sent
}

/* Returns the ESP8266 response code that is in the rx_buffer as a string,
 * this makes debugging and verification through testing easier, at the
 * cost of simplicity.
 */
const char*
get_return(const char* command){

	/* Check for commands that might contain different data than predefined settings,
	 * such as commands that connect to an access point, holds a http request, etc
	 */
	if(strstr(command, ESP8266_AT_CWJAP_SET) != NULL)
		command = ESP8266_AT_CWJAP_SET;
	else if(strstr(command, ESP8266_AT_START) != NULL)
		command = ESP8266_AT_START;
	else if(strstr(command, ESP8266_AT_SEND) != NULL)
		command = ESP8266_AT_SEND;

	KEYS return_type = hash(command);
	switch (return_type) {

		case ESP8266_AT_KEY:

		case ESP8266_AT_GMR_KEY:

		case ESP8266_AT_RST_KEY:

		case ESP8266_AT_CWMODE_STATION_MODE_KEY:

		case ESP8266_AT_CIPMUX_KEY:

		case ESP8266_AT_CWQAP_KEY:
			return evaluate();

		case ESP8266_AT_CWMODE_TEST_KEY:
			if(error_flag || fail_flag)
				return ESP8266_AT_ERROR;
			else {
				if (strstr(rx_buffer, ESP8266_AT_CWMODE_1) != NULL)
					return ESP8266_AT_CWMODE_1;
				else if(strstr(rx_buffer, ESP8266_AT_CWMODE_2) != NULL)
					return ESP8266_AT_CWMODE_2;
				else if(strstr(rx_buffer, ESP8266_AT_CWMODE_3) != NULL)
					return ESP8266_AT_CWMODE_3;
				else
					return ESP8266_AT_UNKNOWN;
			}

		case ESP8266_AT_CWJAP_TEST_KEY:
			if(error_flag || fail_flag)
				return ESP8266_AT_ERROR;
			else {
				if(strstr(rx_buffer, ESP8266_AT_NO_AP))
					return ESP8266_AT_WIFI_DISCONNECTED;
				else
					return ESP8266_AT_WIFI_CONNECTED;
			}

		case ESP8266_AT_CWJAP_SET_KEY:
			if(fail_flag || error_flag){
				if (strstr(rx_buffer, ESP8266_AT_CWJAP_1) != NULL)
					return ESP8266_AT_TIMEOUT;
				else if((strstr(rx_buffer, ESP8266_AT_CWJAP_2) != NULL))
					return ESP8266_AT_WRONG_PWD;
				else if((strstr(rx_buffer, ESP8266_AT_CWJAP_3) != NULL))
					return ESP8266_AT_NO_TARGET;
				else if((strstr(rx_buffer, ESP8266_AT_CWJAP_4) != NULL))
					return ESP8266_AT_CONNECTION_FAIL;
				else
					return ESP8266_AT_ERROR;
			}
			else
				return ESP8266_AT_WIFI_CONNECTED;

		case ESP8266_AT_CIPMUX_TEST_KEY:
			if(error_flag || fail_flag)
				return ESP8266_AT_ERROR;
			else {
				if (strstr(rx_buffer, ESP8266_AT_CIPMUX_0) != NULL)
					return ESP8266_AT_CIPMUX_0;
				else
					return ESP8266_AT_CIPMUX_1;
			}

		case ESP8266_AT_START_KEY:
			if(error_flag || fail_flag)
				return ESP8266_AT_ERROR;
			return ESP8266_AT_CONNECT;

		case ESP8266_AT_SEND_KEY:
			if(error_flag || fail_flag)
				return ESP8266_AT_ERROR;
			return ESP8266_AT_SEND_OK;

		default:
			return ESP8266_NOT_IMPLEMENTED;
			break;
	}
}

const char*
evaluate(void){
	if(error_flag || fail_flag)
		return ESP8266_AT_ERROR;
	return ESP8266_AT_OK;
}


