#ifndef SRAMLIB_H_
#define SRAMLIB_H_

#include <F28x_Project.h>

void WriteSRAM(uint32_t address, uint16_t data);
uint16_t ReadSRAM(uint32_t address);
uint16_t SpibTransmit(uint16_t transmitdata);
void InitSpibGpio(void);
void InitSpib(void);

#endif /* SRAMLIB_H_ */
