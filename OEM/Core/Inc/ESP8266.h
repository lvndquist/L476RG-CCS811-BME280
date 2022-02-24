/**
******************************************************************************
@brief header for the ESP8266 wifi-module functions
@details This header contains documentation for the functions found in the
 	 	 corresponding c file. All used AT commands can be found here, with
 	 	 a short description of their function.

 	 	 Note that for connecting the ESP8266 to wifi, you need to add two
 	 	 variables called "SSID" and "PWD", these should be of type char*.
 	 	 Preferably these can be put in a header file, which is added to
 	 	 your .gitignore if the project is public on github.
 	 	 The header could look something like this:

 	 	 #ifndef INC_LOGIN_H_
		 #define INC_LOGIN_H_

		 static const char SSID[] = "One Plus 5";
		 static const char PWD[]  = "password";

		 #endif

@file ESP8266.h
@author  Jonatan Lundqvist Silins, jonls@kth.se
@date 06-04-2021
@version 1.0
*******************************************************************************/

#include <usart.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <login.h>

#define RX_BUFFER_SIZE 			4096

/* ESP8266 response codes as strings.
   These are all the implemented statuses that can
   be returned when issuing a command to the ESP8266 */

static const char ESP8266_NOT_IMPLEMENTED[]		 = "NOT IMPLEMENTED";
static const char ESP8266_AT_OK_TERMINATOR[]     = "OK\r\n";
static const char ESP8266_AT_OK[] 				 = "OK";
static const char ESP8266_AT_ERROR[] 			 = "ERROR";
static const char ESP8266_AT_FAIL[] 			 = "FAIL";
static const char ESP8266_AT_READY[] 			 = "ready\r\n";
static const char ESP8266_AT_GOT_IP[] 			 = "WIFI GOT IP";
static const char ESP8266_AT_WIFI_CONNECTED[] 	 = "WIFI CONNECTED";
static const char ESP8266_AT_WIFI_DISCONNECTED[] = "WIFI DISCONNECTED";
static const char ESP8266_AT_CONNECT[] 		 	 = "CONNECT";
static const char ESP8266_AT_CLOSED[] 			 = "CLOSED";
static const char ESP8266_AT_SEND_OK[] 			 = "SEND OK";
static const char ESP8266_AT_NO_AP[] 			 = "No AP\r\n";
static const char ESP8266_AT_UNKNOWN[]			 = "UNKNOWN";
static const char ESP8266_AT_CWMODE_1[]			 = "CWMODE_CUR:1";
static const char ESP8266_AT_CWMODE_2[]			 = "CWMODE_CUR:2";
static const char ESP8266_AT_CWMODE_3[]			 = "CWMODE_CUR:3";
static const char ESP8266_AT_CWJAP_1[]			 = "CWJAP:1";
static const char ESP8266_AT_CWJAP_2[]			 = "CWJAP:2";
static const char ESP8266_AT_CWJAP_3[]			 = "CWJAP:3";
static const char ESP8266_AT_CWJAP_4[]			 = "CWJAP:4";
static const char ESP8266_AT_TIMEOUT[]			 = "connection timeout";
static const char ESP8266_AT_WRONG_PWD[]		 = "wrong password";
static const char ESP8266_AT_NO_TARGET[]	     = "cannot find AP";
static const char ESP8266_AT_CONNECTION_FAIL[]	 = "connection failed";
static const char ESP8266_AT_CIPMUX_0[]	 		 = "CIPMUX:0";
static const char ESP8266_AT_CIPMUX_1[]	 		 = "CIPMUX:1";

/* HTTP request strings*/
static const char HTTP_GET[]	 		 		 = "GET ";
static const char HTTP_POST[]	 		 		 = "POST ";
static const char HTTP_VERSION[]	 		     = "HTTP/1.1";
static const char HTTP_HOST[]	 		         = "Host: ";
static const char HTTP_CONNECTION_CLOSE[]	     = "Connection: close";
static const char CRLF[] 						 = "\r\n";

/* djb2 hash keys
 * Each key maps to corresponding AT command, see below for these
 */
typedef enum {
	ESP8266_AT_KEY 						= 2088901425,
	ESP8266_AT_RST_KEY	 				= 617536853,
	ESP8266_AT_GMR_KEY	 				= 604273922,
	ESP8266_AT_CWMODE_STATION_MODE_KEY 	= 608151977,
	ESP8266_AT_CWMODE_TEST_KEY			= 4116713283,
	ESP8266_AT_CWQAP_KEY				= 445513592,
	ESP8266_AT_CWJAP_TEST_KEY			= 1543153456,
	ESP8266_AT_CWJAP_SET_KEY 			= 2616259383,
	ESP8266_AT_CIPMUX_KEY				= 423755967,
	ESP8266_AT_CIPMUX_TEST_KEY			= 3657056785,
	ESP8266_AT_START_KEY				= 3889879756,
	ESP8266_AT_SEND_KEY					= 898252904
} KEYS;

/* AT Commands for the ESP8266, see
 * https://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_en.pdf
 *
 * Commands are case sensitive and should end with /r/n
 * Commands may use 1 or more of these types:
 * Set = AT+<x>=<...> - Sets the value
 * Inquiry = AT+<x>? - See what the value is set at
 * Test = AT+<x>=? - See the possible options
 * Execute = AT+<x> - Execute a command
 *
 * Some commands seem to be outdated, and COMMAND_CUR and COMMAND_DEF should be used instead.
 * CUR will not write the value to flash, DEF will write the value to flash and be used as the default in the future.
 *
 **/

/* Tests AT startup.
 *
 * Returns: OK
 */
static const char ESP8266_AT[]						= "AT\r\n";


/* Restarts the module.
 *
 * Returns: OK
 */
static const char ESP8266_AT_RST[]					= "AT+RST\r\n";

/* Checks version information. */
static const char ESP8266_AT_GMR[]					= "AT+GMR\r\n";

/*Checks current wifi-mode.
 *
 * Returns: <mode>
 * 1: Station Mode
 * 2: SoftAP Mode
 * 3: SoftAP+Station Mode
 */
static const char ESP8266_AT_CWMODE_TEST[]			= "AT+CWMODE_CUR?\r\n";

/*Sets the wifi-mode to station.
 * The module will work as client.
 * Note: setting not saved in flash... so this should be configured 
 * after a restart
 */
static const char ESP8266_AT_CWMODE_STATION_MODE[]	= "AT+CWMODE=1\r\n";

/*Query the AP for current connection */
static const char ESP8266_AT_CWJAP_TEST[]			= "AT+CWJAP?\r\n";

/*Sets a connection to an Access point
 *
 * Command format: AT+CWJAP_CUR=<ssid>,<pwd>
 * <ssid>: the SSID of the target AP.
 * <pwd>: password, MAX: 64-byte ASCII.
 * Note: the command needs Station Mode to be enabled.
 *
 * The command returns an error if:
 * connection times out
 * wrong password
 * cannot find the target AP
 * connection failed
 *
 */
static const char ESP8266_AT_CWJAP_SET[]			= "AT+CWJAP="; // add "ssid","pwd" + CRLF

/* Disconnect connected AP */
static const char ESP8266_AT_CWQAP[]				= "AT+CWQAP\r\n";

/* Disable auto connect to AP
 * Writes to flash...
 * This command seems somewhat broken when running it 
 * on the L476rg. Not sure if it is even needed, but 
 * it can be used to prevent auto connections when 
 * initializing the module. 
 */
static const char ESP8266_AT_CWAUTOCONN[]			= "AT+CWAUTOCONN=0";

/* Set single connection */
static const char ESP8266_AT_CIPMUX_SINGLE[]		= "AT+CIPMUX=0\r\n";

/* Query CIPMUX setting 
 * Used to make sure we have the right setting...
 */
static const char ESP8266_AT_CIPMUX_TEST[]			= "AT+CIPMUX?\r\n";

/* Establishes TCP connection
 *
 * Assumes AT+CIPMUX=0
 * Use following format:
 * AT+CIPSTART=<type>,<remote	IP>,<remote	port>[,<TCP	keep alive>]
 *
 * Example: AT+CIPSTART="TCP","iot.espressif.cn",8000
 * 
 * Note, the quotation marks are important...
 */
static const char ESP8266_AT_START[]				= "AT+CIPSTART=";

/* Disconnect a connection */
static const char ESP8266_AT_STOP[]					= "AT+CIPCLOSE=0";

/* Send data of desired length,
 * this command should be followed by the request
 * that you want to send. 
 * 
 * 1. make connection
 * 2. calculate length of request/data to send 
 * 3. call cipsend with the length
 * 4. send data
 *
 */
static const char ESP8266_AT_SEND[]					= "AT+CIPSEND=";



/*============================================================================
							FUNCTIONS FOR ESP8266
==============================================================================*/


/**
 * @brief assemble the command for connection to AP. Uses the SSID and PWD variables
 * 		  stored in the login.h header.
 * @param char* buffer, where the command is stored into
 * @return void
 */
void
esp8266_get_wifi_command(char* buffer);

/**
 * @brief assemble the command for connection to a website
 * @param char* buffer, where the command is stored into
 * @param char* connection_type, type of connection "TCP", "UDP" or "SSL"
 * @param char* remote_ip, the ip to connect to, can also be a url
 * @param char* remote_port, port to connect
 * @return void
 */
void
esp8266_get_connection_command(char* buffer, char* connection_type,
							   char* remote_ip, char* remote_port);

/**
 * @brief assemble the CIPSEND command with length of request.
 * 		  The esp8266 should be connected to some website before using this.
 *
 * 		  Usage:
 *		  esp8266_get_at_send_command(a_buffer_for_the_command, length_of_your_http_request);
 * 		  esp8266_at_send(a_buffer_for_the_command);
 * 		  esp8266_send_data(buffer_with_http_request);
 *
 * @param char* buffer, where the command is stored
 * @param uint8_t len, length of the command
 * @return void
 */
void
esp8266_get_at_send_command(char* buffer, uint8_t len);


/**
 * @brief assemble the HTTP request to send
 * @param char* buffer, where the command is stored
 * @param const char*, type of the HTTP request,   EXAMPLE: POST or GET
 * @param char* uri, URI for the request, 		   EXAMPLE: google.com/index
 * @param char* host, host adress for the request, EXAMPLE: google.com
 * @return uint8_t, length of the request
 */
uint8_t
esp8266_http_get_request(char* buffer, const char* http_type, char* uri, char* host);

/**
 * @brief start RX interrupt for UART4
 * @param void
 * @return void
 */
void
init_uart_interrupt(void);

/**
 * @brief callback for UART4 RX interrupt
 * @param UART_HandleTypeDef* huart handle
 * @return void
 */
void
HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

/**
 * @brief send command to ESP8266
 * @param char* command to send
 * @return const char*, ESP8266 response string
 *
 * Usage: if(strcmp(esp8266_send_command(ESP8266_AT), ESP8266_AT) != 0))
 * 		  	{ error handling }
 */
const char*
esp8266_send_command(const char*);

/**
 * @brief send data to ESP8266, this is used after calling cipsend
 * where the length of the data that will be sent has been specified.
 * @param char* data to send
 * @return const char*, ESP8266 response string
 */
const char*
esp8266_send_data(const char*);

/**
 * @brief initiate the ESP8266, performs all necessary commands to start using the
 * 		  device. It also verifies that the settings were set.
 * 		  Settings are: station mode (cwmode=1), single connection mode (cipmux=0)
 * @param void
 * @return const char*, ESP8266 response string, either "OK" or "ERROR"
 */
const char*
esp8266_init(void);

/**
 * @brief initiate a wifi connection, uses the esp8266_get_wifi_command function, so make sure
 * 		  that SSID and PWD variables are present and correct.
 * @param void
 * @return const char*, ESP8266 response string.
 * Possible return strings:
 * 		 					"WIFI CONNECTED"
 *	 						"wrong password"
 * 							"cannot find AP"
 * 							"connection failed"
 * 							"error"
 */
const char*
esp8266_wifi_init(void);

/**
 * @brief get hash number for string. The hash number corresponds to a
 * 		  specific command. This is used to determine the possible return values for the command
 * 		  that was sent. There is generally no need for the user to call this function.
 * 		  The hash keys that are implemented can be found in the typedef enum initialized as KEYS.
 * @param const char* string to get hash number for
 * @return const unsigned long the hash number
 */
const unsigned long
hash(const char*);

/**
 * @brief Evaluate ESP8266 response, if any global flags were set return "ERROR" else "OK".
 * Used for applicable AT commands that only need to return basic responses.
 * @return char* "OK" or "ERROR"
 */
const char*
evaluate(void);

/**
 * @brief matches command to ESP8266 return type. Looks up the hash of the command, and then looks for
 * the ESP8266 response which should be returned.
 * @param char* command to match to a return type. The command needs to be in the typedef enum (KEYS)
 * @return char* return ESP8266 response depending on command and its outcome
 */
const char*
get_return(const char*);

/**
 * @brief clear all flags, and the rx buffer
 * @param void
 * @return void
 */
void
esp8266_clear(void);

