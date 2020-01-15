#include <LCD.h>
#include <OneToOneI2CDriver.h>

/*******************************************************
 * Initializes LCD
 *******************************************************/
void LCD_Init()
{
    I2C_O2O_Master_Init(0x3f, 200.0, 270.0);

    Write_Command(0x33);
    Write_Command(0x32);
    Write_Command(0x28);
    Write_Command(0x0f);
    Write_Command(0x01);

    DELAY_US(1520);
}


/********************************************************
 * Writes commands to LCD
 ********************************************************/
void Write_Command(uint16_t command)
{
    uint16_t command_nibbles[4];

    command_nibbles[0] = (command & 0xf0) | 0x0c;
    command_nibbles[1] = (command & 0xf0) | 0x08;
    command_nibbles[2] = (command << 4) | 0x0c;
    command_nibbles[3] = (command << 4) | 0x08;

    I2C_O2O_SendBytes(command_nibbles, 4);
}


/********************************************************
 * Writes null terminated string to LCD
 ********************************************************/
void Write_String(char* string)
{
    int i = 0;
    uint16_t data_nibbles[4];

    while (string[i] != '\0')
    {
        data_nibbles[0] = (string[i] & 0xf0) | 0x0d;
        data_nibbles[1] = (string[i] & 0xf0) | 0x09;
        data_nibbles[2] = (string[i] << 4) | 0x0d;
        data_nibbles[3] = (string[i] << 4) | 0x09;
        I2C_O2O_SendBytes(data_nibbles, 4);
        i++;
    }
}

void Write_Number(int number)
{
    int temp;
    int divisor = 1000;
    uint16_t ascii;
    uint16_t data_nibbles[4];

    if (number < 0)
    {
        Write_String("-");
    }
    else
    {
        Write_String("+");
    }

    for (int i = 0; i < 4; i++)
    {
        temp = (number/divisor) % 10;
        ascii = 0x30 + temp;
        data_nibbles[0] = (ascii & 0xf0) | 0x0d;
        data_nibbles[1] = (ascii & 0xf0) | 0x09;
        data_nibbles[2] = (ascii << 4) | 0x0d;
        data_nibbles[3] = (ascii << 4) | 0x09;
        I2C_O2O_SendBytes(data_nibbles, 4);
        divisor = divisor/10;
    }
}
