/**
******************************************************************************
@brief functions for the environmental sensors, CCS811 & BME280.
@details The functions implemented here should be everything needed to
		 use the CCS811 & BME280 sensors with the Nucleo 476RG board.
		 The designed functions are to be used in the office environment
		 project, but could also be implemented in other projects.
@file CCS811_BME280.c
@author  Sebastian Divander,       sdiv@kth.se
@author  Jonatan Lundqvist Silins, jonls@kth.se
@date 06-04-2021
@version 1.0
******************************************************************************
*/

#include "CCS811_BME280.h"

/* Global variables */
static uint16_t CO2;
static uint16_t tVOC;

/* Compensation register values based on BME280 datasheet */
static uint16_t dig_T1_val;
static int16_t  dig_T2_val;
static int16_t  dig_T3_val;
static uint8_t  dig_H1_val;
static int16_t  dig_H2_val;
static uint8_t  dig_H3_val;
static int16_t  dig_H4_val;
static int16_t  dig_H5_val;
static int8_t   dig_H6_val;
static int32_t  t_fine;

/**********************************************************************
 **********************************************************************
 ***																***
 ***					CCS811 FUNCTIONS							***
 ***																***
 **********************************************************************
 **********************************************************************/

ENV_SENSOR_STATUS
CCS811_init(void){

	uint8_t register_value = 0;
	ENV_SENSOR_STATUS status = CCS811_SUCCESS;
	HAL_StatusTypeDef temp = HAL_OK;

	RETRY:
	HAL_Delay(100);
	register_value = 0;
	status = CCS811_SUCCESS;
	temp = HAL_OK;

	/* Read the HW ID register to make sure the sensor is responsive */
	// mem read here, otherwise i2c seems to break
	temp = HAL_I2C_Mem_Read(&hi2c1, CCS811_ADDR, 0x20, 1, &register_value, 1, 500);

	/* If we fail here, try again. If the module is connected correctly it should work eventually, might be some internal problem... */
	if((temp != HAL_OK) ||  (register_value != 0x81)){
		HAL_Delay(100);
		goto RETRY;
	}

	/* Reset the device & wait a bit */
	status = CCS811_reset();
	if(status != CCS811_SUCCESS)
		return status;
	HAL_Delay(30);

	/* Check for sensor errors */
	if(CCS811_read_status_error() != 0){
		//uint8_t err = CCS811_read_error_id();
		return CCS811_ERROR;
	}
	HAL_Delay(30);

	/* Check if app is valid */
	if(CCS811_read_app_valid() != 1)
		return CCS811_ERROR;
	HAL_Delay(30);

	/* Write to app start register to start */
	status = CCS811_app_start();
	if(status != CCS811_SUCCESS)
		return status;
	HAL_Delay(30);

	/* Set drive mode to 1; measurement each second
	 * mode 2; measurement every 10 seconds
	 * mode 3; measurement every 60 seconds
	 * mode 4; measurement every 250ms
	 * */
	status = CCS811_write_mode(1);
	if(status != CCS811_SUCCESS)
		return status;
	HAL_Delay(30);

	/* Check for sensor errors before exiting */
	if(CCS811_read_status_error() != 0){
		//uint8_t err = CCS811_read_error_id();
		return CCS811_ERROR;
	}
	HAL_Delay(30);

	return status;
}

/* Read a register using I2C */
ENV_SENSOR_STATUS
CCS811_read_register(uint8_t reg_addr, uint8_t* buffer, uint8_t size)
{
	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_I2C_Mem_Read(&hi2c1, CCS811_ADDR, (uint8_t) reg_addr, I2C_MEMADD_SIZE_8BIT, buffer, size, HAL_MAX_DELAY);
	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);
	if(status != HAL_OK)
		 return CCS811_I2C_ERROR;
	return CCS811_SUCCESS;
}

/* Write to a register using I2C */
ENV_SENSOR_STATUS
CCS811_write_register(uint8_t reg_addr, uint8_t* buffer, uint8_t size){

	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_I2C_Mem_Write(&hi2c1, CCS811_ADDR, (uint8_t) reg_addr, I2C_MEMADD_SIZE_8BIT, buffer, size, HAL_MAX_DELAY);
	if(status != HAL_OK)
		 return CCS811_I2C_ERROR;
	return CCS811_SUCCESS;
}

/* Read bit 0, which is the status error bit, if 1 is returned there was an error
   if 0 is returned no errors have occurred.	 	 							   */
uint8_t
CCS811_read_status_error(void){
	uint8_t register_value;
	CCS811_read_register(STATUS_REG, &register_value, 1);
	return (register_value & 0x01);
}

/* Read error id register and return error bits */
uint8_t
CCS811_read_error_id(void){
	uint8_t register_value;
	CCS811_read_register(ERROR_ID, &register_value, 1);
	return register_value;
}

/* Check that the app is valid */
uint8_t
CCS811_read_app_valid(void){
	uint8_t register_value;
	CCS811_read_register(STATUS_REG, &register_value, 1);
	register_value = (register_value >> 4) & 0x01;
	return register_value;
}

/* Start the application, it shouldnt send any data so this uses master transmit... */
ENV_SENSOR_STATUS
CCS811_app_start(void){
	uint8_t app_start = APP_START;
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_I2C_Master_Transmit(&hi2c1, CCS811_ADDR, &app_start, 1, HAL_MAX_DELAY);
	if(status != HAL_OK)
		return CCS811_I2C_ERROR;
	return CCS811_SUCCESS;
}

/* Set mode, changes the interval of measurements */
ENV_SENSOR_STATUS
CCS811_write_mode(uint8_t mode){
	uint8_t register_value = 0;
	ENV_SENSOR_STATUS status = CCS811_SUCCESS;

	if(mode > 4 || mode < 0)
		return CCS811_ERROR;

	/* Check what's in the register */
	status = CCS811_read_register(MEAS_MODE, &register_value, 1);
	if(status != CCS811_SUCCESS)
		return CCS811_I2C_ERROR;

	/* Clear current, and add new mode that should be set */
	register_value = register_value & ~(0x70);
	register_value = register_value | (mode << 4);

	/* Write the mode */
	status = CCS811_write_register(MEAS_MODE, &register_value, 1);
	if(status != CCS811_SUCCESS)
		return status;

	return status;
}

/* Reset the sensor */
ENV_SENSOR_STATUS
CCS811_reset(void){
	uint8_t reset_key[4] = {0x11, 0xE5, 0x72, 0x8A};
	if(CCS811_write_register(SW_RESET, reset_key, 4) != CCS811_SUCCESS)
		return CCS811_I2C_ERROR;
	return CCS811_SUCCESS;
}

ENV_SENSOR_STATUS
CCS811_data_available(void){

	uint8_t register_value = 0;
	ENV_SENSOR_STATUS status = CCS811_SUCCESS;

	/* Check what's in the register */
	status = CCS811_read_register(STATUS_REG, &register_value, 1);
	if(status != CCS811_SUCCESS)
		return CCS811_I2C_ERROR;

	register_value = (register_value & 0x08) >> 3;
	if(register_value == 0)
		return CCS811_NO_NEW_DATA;

	return CCS811_NEW_DATA;
}

/* Set environmental values taken from BME280 sensor */
ENV_SENSOR_STATUS
CCS811_set_temp_hum(float temp, float hum){

	ENV_SENSOR_STATUS status = CCS811_SUCCESS;

	/* values larger or smaller than this will not fit into the registers */
	if(temp < -25 || temp > 50 || hum < 0 || hum > 100)
		return CCS811_ERROR;

	uint32_t hum_t = hum * 1024;
	uint32_t temp_t = temp * 1000;

	uint8_t data[4] = {};				/* signed or not signed ???*/
	data[0] = (hum_t + 250) / 500;
	data[1] = 0x00;
	data[2] = (temp_t + 25250) / 500;
	data[3] = 0x00;

	status = CCS811_write_register(ENV_DATA, data, 4);
	if(status != CCS811_SUCCESS)
		return CCS811_I2C_ERROR;
	return status;
}

/* Read CO2 and tVoc values */
ENV_SENSOR_STATUS
CCS811_read_alg_res(void){

	uint8_t data[4];
	ENV_SENSOR_STATUS status = CCS811_SUCCESS;

	status = CCS811_read_register(ALG_RES_DATA, data, 4);
	if(status != CCS811_SUCCESS)
		return CCS811_I2C_ERROR;

	/* data[0]: eCO2 High Byte
	 * data[1]: eCO2 Low Byte
	 * data[2]: TVOC High Byte
	 * data[3]: TVOC Low Byte 	*/
	CO2 = ((uint16_t)data[0] << 8) | data[1];
	tVOC = ((uint16_t)data[2] << 8) | data[3];

	return status;
}

uint16_t
CCS811_get_co2(void){
	return CO2;
}

uint16_t
CCS811_get_tvoc(void){
	return tVOC;
}

/*****************************************************************************************
 *****************************************************************************************
 ***																				   ***
 ***								BME FUNCTIONS									   ***
 ***																			   	   ***
 *****************************************************************************************
 *****************************************************************************************/

ENV_SENSOR_STATUS
BME280_init(void){

	ENV_SENSOR_STATUS status = BME280_SUCCESS;
	uint8_t register_value = 0;

	/* Read the ID register to make sure the sensor is responsive */
	status = BME280_read_register8(ID_REG, &register_value, 1);
	if(status != BME280_SUCCESS)
		return status;
	if(register_value != 0x60)
		return BME280_ID_ERR;

	/* Read calibration data for humidity and temperature */
	status = BME280_read_calibration();
	if(status != BME280_SUCCESS)
		return status;

	/* Standard config for filter and rate */
	status = BME280_config();
	if(status != BME280_SUCCESS)
		return status;

	/* Set mode to 0 */
	status = BME280_set_mode(0);
	if(status != BME280_SUCCESS)
		return status;

	/* Set humidity oversample */
	status = BME280_set_hum_os();
	if(status != BME280_SUCCESS)
		return status;

	/* Set temperature oversample  ****last for humidity control register changes to be applied*****/
	status = BME280_set_temp_os();
	if(status != BME280_SUCCESS)
		return status;

	/* Set normal operation */
	status = BME280_set_mode(3);
	if(status != BME280_SUCCESS)
		return status;

	return status;
}

ENV_SENSOR_STATUS
BME280_read_register8(uint8_t reg_addr, uint8_t* buffer, uint8_t size)
{
	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_I2C_Mem_Read(&hi2c1, BME280_ADDR, (uint8_t) reg_addr, I2C_MEMADD_SIZE_8BIT, buffer, size, HAL_MAX_DELAY);
	if(status != HAL_OK)
		 return BME280_I2C_ERROR;
	return BME280_SUCCESS;
}

ENV_SENSOR_STATUS
BME280_read_register16(uint8_t reg_addr, uint16_t* buffer)
{
	uint8_t buf[2];
	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_I2C_Mem_Read(&hi2c1, BME280_ADDR, (uint8_t) reg_addr, I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
	if(status != HAL_OK)
		 return BME280_I2C_ERROR;

	/* Convert the 8bit array to 16 bit value */
	*buffer = (uint16_t) ((buf[1] << 8) | buf[0]);
	return BME280_SUCCESS;
}

ENV_SENSOR_STATUS
BME280_write_register(uint8_t reg_addr, uint8_t* buffer, uint8_t size){

	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_I2C_Mem_Write(&hi2c1, BME280_ADDR, (uint8_t) reg_addr, I2C_MEMADD_SIZE_8BIT, buffer, size, HAL_MAX_DELAY);
	if(status != HAL_OK)
		 return BME280_I2C_ERROR;
	return BME280_SUCCESS;

}

ENV_SENSOR_STATUS
BME280_read_calibration(void){

	uint16_t dig_H4_temp; // [11:4]
	uint16_t dig_H5_temp; // [7:4]

	// TODO: refactor this? --> && all statuses in one if?
	/* Signed variables need to be casted to unsigned when using the read functions */
	ENV_SENSOR_STATUS status = BME280_SUCCESS;
	status = BME280_read_register16(dig_T1_reg, 			&dig_T1_val);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register16(dig_T2_reg, (uint16_t*) &dig_T2_val);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register16(dig_T3_reg, (uint16_t*) &dig_T3_val);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register8 (dig_H1_reg, 			&dig_H1_val, 1);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register16(dig_H2_reg, (uint16_t*) &dig_H2_val);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register8 (dig_H3_reg, 			&dig_H3_val, 1);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register16(dig_H4_reg, 			&dig_H4_temp);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register16(dig_H5_reg, 			&dig_H5_temp);
	if(status != BME280_SUCCESS)
		return status;

	status = BME280_read_register8 (dig_H6_reg, (uint8_t*)	&dig_H6_val, 1);
	if(status != BME280_SUCCESS)
		return status;

	/* Move h4 and h5 to correct positions :))))
	   could probably be done nicer  	 	 	 */
	dig_H4_val = ((dig_H4_temp & 0x00FF) << 4);
	dig_H4_val = (dig_H4_val | ((dig_H4_temp & 0x0F00) >> 8));
	dig_H5_val = dig_H5_temp >> 4;

	return status;
}

ENV_SENSOR_STATUS
BME280_set_mode(uint8_t mode){

	if(mode > 3 || mode < 0)
		return BME280_ERROR;

	uint8_t register_value = 0;
	ENV_SENSOR_STATUS status = BME280_SUCCESS;

	status = BME280_read_register8 (CTRL_MEAS, &register_value, 1);
	if(status != BME280_SUCCESS)
		return status;
	register_value = register_value & 0xFC;
	register_value = register_value | mode;

	status = BME280_write_register(CTRL_MEAS, &register_value, 1);

	return status;
}

uint8_t
BME280_get_mode(void){
	uint8_t register_value = 0;
	BME280_read_register8 (CTRL_MEAS, &register_value, 1);
	return (register_value & 0x03);
}

/* Set standard config for filter and rate */
ENV_SENSOR_STATUS
BME280_config(void){

	uint8_t register_value = 0;
	ENV_SENSOR_STATUS status = BME280_SUCCESS;

	status = BME280_read_register8 (CONFIG_REG, &register_value, 1);
	if(status != BME280_SUCCESS)
		return status;
	register_value = register_value & 0b00000010;
	register_value = register_value | std_cnf;

	status = BME280_write_register(CONFIG_REG, &register_value, 1);

	return status;
}

/* Set humidity oversampling to 1x */
ENV_SENSOR_STATUS
BME280_set_hum_os(void){

	ENV_SENSOR_STATUS status = BME280_SUCCESS;
	uint8_t register_value = 0;

	status = BME280_read_register8 (CTRL_HUM, &register_value, 1);
	register_value = register_value & 0b11111000;
	register_value = register_value | std_hum;

	status = BME280_write_register(CTRL_HUM, &register_value, 1);

	uint8_t temp;
	status = BME280_read_register8 (CTRL_HUM, &temp, 1);

	return status;

}

/* Set temperature oversampling to 1x */
ENV_SENSOR_STATUS
BME280_set_temp_os(void){

	ENV_SENSOR_STATUS status = BME280_SUCCESS;
	uint8_t register_value = 0;

	status = BME280_read_register8 (CTRL_MEAS, &register_value, 1);
	register_value = register_value & 0b00011111;
	register_value = register_value | std_temp;

	status = BME280_write_register(CTRL_MEAS, &register_value, 1);

	return status;
}

float
BME280_read_temp(void){

	uint8_t buf[8];
	BME280_read_register8(0xF7, buf, 8);
	int32_t adc_Temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((buf[5] >> 4) & 0x0F);

	/* TEMPERATURE CONVERSION FROM DATASHEET */
	int64_t var1;
	int64_t var2;

	var1 = ((((adc_Temp>>3) - ((int32_t)dig_T1_val<<1))) * ((int32_t)dig_T2_val)) >> 11;
	var2 = (((((adc_Temp>>4) - ((int32_t)dig_T1_val)) * ((adc_Temp>>4) - ((int32_t)dig_T1_val))) >> 12) * ((int32_t)dig_T3_val)) >> 14;
	t_fine = var1 + var2;
	float output = (t_fine * 5 + 128) >> 8;

	output = output / 100 + 0.f;

	return output;
}

float
BME280_read_hum(void){

	uint8_t buf[8];
	BME280_read_register8(0xF7, buf, 8);

	/* HUMIDITY CONVERSION FROM DATASHEET */
	int32_t adc_H = ((uint32_t)buf[6] << 8) | ((uint32_t)buf[7]);

	int32_t var1;
	var1 = (t_fine - ((int32_t)76800));
	var1 = (((((adc_H << 14) - (((int32_t)dig_H4_val) << 20) - (((int32_t)dig_H5_val) * var1)) +
	((int32_t)16384)) >> 15) * (((((((var1 * ((int32_t)dig_H6_val)) >> 10) * (((var1 * ((int32_t)dig_H3_val)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
	((int32_t)dig_H2_val) + 8192) >> 14));
	var1 = (var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) * ((int32_t)dig_H1_val)) >> 4));
	var1 = (var1 < 0 ? 0 : var1);
	var1 = (var1 > 419430400 ? 419430400 : var1);

	return (float)(var1>>12) / 1024.0;
}
