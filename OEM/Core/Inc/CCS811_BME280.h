/**
******************************************************************************
@brief header for the CCS811 & BME280 sensor functions.
@details all supporting documentation for the ccs811 and bme280 function set
		 is found in this header.
@file CCS811_BME280.h
@author  Sebastian Divander,       sdiv@kth.se
@author  Jonatan Lundqvist Silins, jonls@kth.se
@date 06-04-2021
@version 1.0
******************************************************************************
*/


#ifndef INC_CCS811_BME280_H_
#define INC_CCS811_BME280_H_


#include "i2c.h"
#include "stdio.h"

/* CCS881 registers */
#define CCS811_ADDR 	0xB6 	// Default I2C Address, shifted 1 bit to the left because HAL
#define STATUS_REG 		0x00	// Status register, R, 1 byte
#define MEAS_MODE 		0x01	// Measurement mode and conditions register, R/W, 1 byte
#define ALG_RES_DATA 	0x02	// Algorithm result, R, 8 bytes
#define RAWDATAREG 		0x03	// ?
#define HW_ID 			0x20 	// Hardware ID, R, 1 byte, should be 0x81
#define APP_START		0xF4	// Application start
#define ERROR_ID		0xE0    // Reported errors, R, 1 byte
#define MEAS_MODE_1 	0x10	// Set to measure each second
#define SW_RESET		0xFF	// Register for resetting the device, W, 4 bytes
#define ENV_DATA		0x05	// Set current humidity and temperature, W, 4 bytes

/* BME280 registers */
#define BME280_ADDR		0xEE	// 0x77 shifted to the left 1 bit, because HAL
#define ID_REG			0xD0	// Read id, should be 0x60
#define CTRL_MEAS		0xF4 	// Control register for measurement, also temp oversample
#define BME280_STATUS	0xF3	// Status register
#define HUM_LSB			0xFE
#define HUM_MSB			0xFD
#define TEMP_XLSB		0xFC	// Temp bits 7-4
#define TEMP_LSB		0xFB
#define TEMP_MSB		0xFA
#define dig_T1_reg		0x88
#define dig_T2_reg		0x8A
#define dig_T3_reg		0x8C
#define dig_H1_reg		0xA1
#define dig_H2_reg		0xE1
#define dig_H3_reg		0xE3
#define dig_H4_reg		0xE4
#define dig_H5_reg		0xE5
#define dig_H6_reg		0xE7
#define CONFIG_REG		0xF5	// Config for filters and rates
#define std_cnf			0x00	// filter = off and rate = 0.5ms
#define std_hum			0x01	// humidity oversample x1 oversampling
#define std_temp		0x20	// temperature oversample x1 oversampling
#define CTRL_HUM  		0xF2

/* Environmental sensor return codes */
typedef enum
{
	CCS811_SUCCESS = 0,		// Success status
	CCS811_ERROR, 			// Some internal sensor error, error status set
	CCS811_NOT_READY,
	CCS811_NEW_DATA,
	CCS811_NO_NEW_DATA,
	CCS811_I2C_ERROR, 		// error when writing/reading a register with i2c
	BME280_SUCCESS,
	BME280_ERROR,
	BME280_ID_ERR,
	BME280_I2C_ERROR
} ENV_SENSOR_STATUS;


/**
 * @brief read a register of the ccs811 using i2c
 * @param uint8_t reg_addr, register adress
 * @param uint8_t* buffer, data from the read register
 * @param size, how many bytes to read
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS or CCS811_I2C_ERROR.
 */
ENV_SENSOR_STATUS
CCS811_read_register(uint8_t reg_addr, uint8_t* buffer, uint8_t size);

/**
 * @brief write to a register of the ccs811 using i2c
 * @param uint8_t reg_addr, register adress
 * @param uint8_t* buffer, data to write
 * @param size, how many bytes to write
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS or CCS811_I2C_ERROR
 */
ENV_SENSOR_STATUS
CCS811_write_register(uint8_t reg_addr, uint8_t* buffer, uint8_t size);

/**
 * @brief initiate the ccs811, uses all necessary functions to start measuring environmental data.
 * @param void
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS, CCS811_ERROR or CCS811_I2C_ERROR
 */
ENV_SENSOR_STATUS
CCS811_init(void);

/**
 * @brief reads the status error bit in the status register of the ccs811
 * @param void
 * @return uint8_t, either 1 or 0.
 * 		   1 -> an error has occurred and the error id register should be read
 * 		   0 -> no error has occurred
 */
uint8_t
CCS811_read_status_error(void);

/**
 * @brief read the error id register, should generally be done when an error has been marked in the status register.
 * @param void
 * @return uint8_t, an 8 bit code. Consult the ccs811 datasheet (figure 20) to see the what the error code is
 */
uint8_t
CCS811_read_error_id(void);

/**
 * @brief read the app valid register to ensure the app firmware is loaded
 * @param void
 * @return uint8_t, either 1 or 0.
 * 					1 -> no app firmware loaded
 * 					0 -> app firmware loaded
 */
uint8_t
CCS811_read_app_valid(void);

/**
 * @brief switches the sensor state from boot to running, before using this you should verify that there is an application loaded
 * @param void
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS or CCS811_I2C_ERROR
 */
ENV_SENSOR_STATUS
CCS811_app_start(void);

/**
* @brief change the current mode of the ccs811 sensor.
* 		 mode 1; measurement each second
* 		 mode 2; measurement every 10 seconds
* 	  	 mode 3; measurement every 60 seconds
*    	 mode 4; measurement every 250ms (intended for systems where an external host system)
*		 When a sensor operating mode is changed to a new mode with
*		 a lower sample rate (e.g. from Mode 1 to Mode 3),
*		 it should be placed in Mode 0 (Idle) for at least 10 minutes before enabling
*	     the new mode. When a sensor operating mode is changed to a new mode with a
*	     higher sample rate (e.g. from Mode 3 to Mode 1), there is no requirement to wait before enabling the new
*		 mode.
*
* @param uint8_t mode, the mode to use, valid values are 1-4
* @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS, CCS811_ERROR or CCS811_I2C_ERROR
*/
ENV_SENSOR_STATUS
CCS811_write_mode(uint8_t mode);

/**
 * @brief software reset for the ccs811, the sensor will be started in boot mode
 * @param void
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS or CCS811_I2C_ERROR.
 */
ENV_SENSOR_STATUS
CCS811_reset(void);

/**
 * @brief check if new environmental data is available
 * @param void
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS, CCS811_I2C_ERROR, CCS811_NO_NEW_DATA or CCS811_NEW_DATA
 */
ENV_SENSOR_STATUS
CCS811_data_available(void);

/**
 * @brief set the current temperature and humidity, which are used to compensate gas reading.
 * @param float temp, the current temperature
 * @param float hum, the current humidity
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS, CCS811_ERROR or CCS811_I2C_ERROR
 */
ENV_SENSOR_STATUS
CCS811_set_temp_hum(float temp, float hum);

/**
 * @brief reads the raw data for the co2 and volatile gases. These values need to be converted further.
 * @param void
 * @return ENV_SENSOR_STATUS, either returns CCS811_SUCCESS or CCS811_I2C_ERROR
 */
ENV_SENSOR_STATUS
CCS811_read_alg_res(void);

/**
 * @brief converts raw co2 data to co2 data in ppm. CCS811_read_alg_res should be run before this function.
 * @param void
 * @return uint16_t, the co2 data in ppm
 */
uint16_t
CCS811_get_co2(void);

/**
 * @brief converts raw tVoc data to tVoc data in ppb. CCS811_read_alg_res should be run before this function.
 * @param void
 * @return uint16_t, the tVoc data in ppb
 */
uint16_t
CCS811_get_tvoc(void);

/**
 * @brief initiates the BME280 temperature and humidity sensor, performs all necessary functions to start measuring temperature and humidity.
 * @param void
 * @return ENV_SENSOR_STATUS, either returns BME280_ID_ERR, BME280_I2C_ERROR, BME280_ERROR or BME280_SUCCESS
 */
ENV_SENSOR_STATUS
BME280_init(void);

/**
 * @brief read an bme280 8 bit register
 * @param uint8_t reg_addr, register address to read
 * @param uint8_t* buffer, value at the register address
 * @param uint8_t size, bytes to read
 * @return
 */
ENV_SENSOR_STATUS
BME280_read_register8(uint8_t reg_addr, uint8_t* buffer, uint8_t size);

/**
 * @brief read an bme280 16 bit register. This can be done with the 8 bit variant, but it simplifies when we need to assign
 * 		  values to uint16_t variables, mainly when reading calibration.
 * @param uint8_t reg_addr, register address to read
 * @param uint16_t* buffer, value at the register address
 * @return ENV_SENSOR_STATUS, either returns BME280_I2C_ERROR or BME280_SUCCESS
 */
ENV_SENSOR_STATUS
BME280_read_register16(uint8_t reg_addr, uint16_t* buffer);

/**
 * @brief write an bme280 register
 * @param uint8_t reg_addr, register address to write to
 * @param uint16_t* buffer, value to write
 * @param uint8_t size, size of value to write in bytes
 * @return ENV_SENSOR_STATUS, either returns BME280_I2C_ERROR or BME280_SUCCESS
 */
ENV_SENSOR_STATUS
BME280_write_register(uint8_t reg_addr, uint8_t* buffer, uint8_t size);

/**
 * @brief read calibration values of the BME280. The calibration variables that are assigned are static global variables
 * 		  used when calculating the humidity and temperature.
 * @param void
 * @return ENV_SENSOR_STATUS, either returns BME280_I2C_ERROR or BME280_SUCCESS
 */
ENV_SENSOR_STATUS
BME280_read_calibration(void);

/**
 * @brief set the mode of the BME280, there are 3 valid modes:
 * 		  mode 1 -> sleep mode
 * 		  mode 2 -> forced mode
 * 		  mode 3 -> normal mode
 * 		  For further information see the datasheet, chapter 3.3 (https://www.mouser.com/datasheet/2/783/BST-BME280-DS002-1509607.pdf)
 * @param uint8_t mode, the mode to set
 * @return ENV_SENSOR_STATUS, either returns BME280_ERROR, BME280_I2C_ERROR or BME280_SUCCESS
 */
ENV_SENSOR_STATUS
BME280_set_mode(uint8_t mode);

/**
 * @brief get the currently set mode of the BME280.
 * @param void
 * @return uint8_t, a 2 bit value, between 0-3
 */
uint8_t
BME280_get_mode(void);

/**
 * @brief configure the BME280, this turns off filtering and sets the rate of measurement to 0.5ms. The config is an 8 bit value,
 * 		  to change the config, change the std_cnf define.
 * @param void
 * @return ENV_SENSOR_STATUS, either returns BME280_I2C_ERROR or BME280_SUCCESS.
 */
ENV_SENSOR_STATUS
BME280_config(void);

/**
 * @brief set the humidity oversampling to 1x. To change what oversampling is set, change the std_hum define.
 * @param void
 * @return ENV_SENSOR_STATUS, either returns BME280_I2C_ERROR or BME280_SUCCESS.
 */
ENV_SENSOR_STATUS
BME280_set_hum_os(void);

/**
 * @brief set the temperature oversampling to 1x. To change what oversampling is set, change the std_temp define.
 * @param void
 * @return ENV_SENSOR_STATUS, either returns BME280_I2C_ERROR or BME280_SUCCESS.
 */
ENV_SENSOR_STATUS
BME280_set_temp_os(void);

/**
 * @brief reads the raw temperature registers and does all necessary calculations. The temperature is stored in a static global variable.
 * @param void
 * @return float, the calculated temperature in degrees celsius.
 */
float
BME280_read_temp(void);

/**
 * @brief reads the raw humidity registers and does all necessary calculations. The humidity is stored in a static global variable.
 * @param void
 * @return float, the calculated humidity percentage.
 */
float
BME280_read_hum(void);

#endif /* INC_CCS811_BME280_H_ */
