#ifndef LCDLIB_H_
#define LCDLIB_H_

#include <F28x_Project.h>

void LCD_Init();
void Write_Command(Uint16 command);
void Write_String(char* string);
void Write_Number(int number);

#endif /* LCDLIB_H_ */
