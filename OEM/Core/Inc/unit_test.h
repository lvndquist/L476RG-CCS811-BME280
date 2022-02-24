/**
******************************************************************************
@brief Header for the project unit test
@file unit_test.h
@author Jonatan Lundqvist Silins, jonls@kth.se
******************************************************************************
*/

#include "unity.h"
#include "stdio.h"
#include "usart.h"

void unit_test(void);
void setUp(void);
void tearDown(void);
void test_BME280_init(void);
void test_CCS811_init(void);
void test_esp8266_init(void);
void test_esp8266_at_cwjap_verify(void);
void test_esp8266_wifi_connect(void);
void test_esp8266_web_connection(void);
void test_esp8266_web_request(void);
void test_esp8266_at_send(char*);
void test_esp8266_send_data(char*);
void test_display_init(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef*);

