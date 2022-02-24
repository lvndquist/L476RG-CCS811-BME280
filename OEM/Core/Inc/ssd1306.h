/*
 * disp.h
 *
 *  Created on: Apr 27, 2021
 *      Author: Sebastian Divander,       sdiv@kth.se
 *              Jonatan Lundqvist Silins, jonls@kth.se
 */

#ifndef INC_SSD1306_H_
#define INC_SSD1306_H_

#include "i2c.h"
#include "fonts.h"

/**
 * @brief defines for the display
 *
 * @var H - Max height of the display
 * @var W - Max width of the display
 * @var MAX_CHARS - Max amount of chars on one line
 * @var MAX_ROWS - Max amount of rows in display
 * @var BUFFERSIZE - Max buffersize of the display
 */
#define H 64
#define W 128
#define MAX_CHARS 18
#define MAX_ROWS 5
#define ROW_SIZE 12
#define BUFFERSIZE 1024

/*
 * @brief Enumeration of colours for the display¨: black or White
 */
typedef enum
{
	BLACK,
	WHITE
} Display_ColourDef;

/*
 * @brief Display object structs for information about the display
 * @var thisX - current Xposition in the display buffer
 * @var thisY - current Yposition in the display buffer
 */
typedef struct {
    uint16_t thisX;
    uint16_t thisY;
    HAL_StatusTypeDef Init_Status;
    HAL_StatusTypeDef Update_Status;
} Display_t;

/*
 * Function prototype declaration
 */
uint16_t display_get_y(void);
uint16_t display_get_x(void);
HAL_StatusTypeDef display_get_init_status(void);
HAL_StatusTypeDef display_get_update_status(void);
void display_init(void);
HAL_StatusTypeDef command(uint8_t);
void draw_pixel(uint8_t, uint8_t, Display_ColourDef);
void reset_screen_canvas(void);
void retry(void);
void display_update(void);
void display_write_char(char, FontDef, Display_ColourDef);
void display_write_string(const char*, Display_ColourDef);
void display_write_string_no_update(const char*, Display_ColourDef);
void display_set_position(uint16_t, uint16_t);
void display_error_message(void);
void display_string_on_line(const char*, Display_ColourDef, uint8_t);
void display_string_on_line_no_update(const char*, Display_ColourDef, uint8_t);

#endif /* INC_SSD1306_H_ */
