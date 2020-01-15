#include <SRAM.h>

void WriteSRAM(uint32_t address1, uint16_t data)
{
    if (address1 < (uint32_t)0x20000)
    {
        address1 = address1 << 1;
        GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;         //CS0 low
        SpibTransmit(0x0200);                        //write instruction
        SpibTransmit((uint16_t)(address1 >> 8));                        //24 bit address, first 7 bits dont care
        SpibTransmit((uint16_t)(address1));
        SpibTransmit((uint16_t)(address1 << 8));
        SpibTransmit(data << 8);
        SpibTransmit(data);
        GpioDataRegs.GPCSET.bit.GPIO66 = 1;         //CS0 high
    }
    else
    {
        address1 -= (uint32_t)0x20000;
        address1 = address1 << 1;
        GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;          //CS0 low
        SpibTransmit(0x0200);                        //write instruction
        SpibTransmit((uint16_t)(address1 >> 8));                        //24 bit address, first 7 bits dont care
        SpibTransmit((uint16_t)(address1));
        SpibTransmit((uint16_t)(address1 << 8));
        SpibTransmit(data << 8);
        SpibTransmit(data);
        GpioDataRegs.GPCSET.bit.GPIO67 = 1;           //CS0 high
    }
}

uint16_t ReadSRAM(uint32_t address2)
{

    uint16_t temp1;
    uint16_t temp2;

    if (address2 < (uint32_t)0x20000)
    {
        address2 = address2 << 1;
        GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;          //CS0 low
        SpibTransmit(0x0300);                        //write instruction
        SpibTransmit((uint16_t)(address2 >> 8));                        //24 bit address, first 7 bits dont care
        SpibTransmit((uint16_t)(address2));
        SpibTransmit((uint16_t)(address2 << 8));
        SpibTransmit(0xFF00);
        temp1 = SpibTransmit(0xFF00);
        temp2 = SpibTransmit(0xFF00);
        GpioDataRegs.GPCSET.bit.GPIO66 = 1;         //CS0 high
    }
    else
    {
        address2 -= (uint32_t)0x20000;
        address2 = address2 << 1;
        GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;          //CS0 low
        SpibTransmit(0x0300);                        //write instruction
        SpibTransmit((uint16_t)(address2 >> 8));                        //24 bit address, first 7 bits dont care
        SpibTransmit((uint16_t)(address2));
        SpibTransmit((uint16_t)(address2 << 8));
        SpibTransmit(0xFF00);
        temp1 = SpibTransmit(0xFF00);
        temp2 = SpibTransmit(0xFF00);
        GpioDataRegs.GPCSET.bit.GPIO67 = 1;         //CS0 high
    }
    return temp1 | (temp2 << 8);
}

uint16_t SpibTransmit(uint16_t transmitdata)
{
    //send data to SPI register
    SpibRegs.SPITXBUF = transmitdata;
    //wait until the data has been sent
    while(!SpibRegs.SPISTS.bit.INT_FLAG);
    //return the data received
    uint16_t receiveddata = SpibRegs.SPIRXBUF;
    //make sure to flush any buffers and clear any flags

    return receiveddata;
}

void InitSpibGpio(void)
{
    EALLOW;

    GpioCtrlRegs.GPBPUD.bit.GPIO63 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO64 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO65 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO66 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO67 = 0;

    GpioCtrlRegs.GPBGMUX2.bit.GPIO63 = 3;
    GpioCtrlRegs.GPCGMUX1.bit.GPIO64 = 3;
    GpioCtrlRegs.GPCGMUX1.bit.GPIO65 = 3;
    GpioCtrlRegs.GPCGMUX1.bit.GPIO66 = 0;
    GpioCtrlRegs.GPCGMUX1.bit.GPIO67 = 0;

    GpioCtrlRegs.GPBMUX2.bit.GPIO63 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO64 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO65 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO66 = 0;
    GpioCtrlRegs.GPCMUX1.bit.GPIO67 = 0;

    GpioCtrlRegs.GPCDIR.bit.GPIO66 = 1;
    GpioCtrlRegs.GPCDIR.bit.GPIO67 = 1;

    GpioDataRegs.GPCSET.bit.GPIO66 = 1;
    GpioDataRegs.GPCSET.bit.GPIO67 = 1;
}


void InitSpib(void)
{
    EALLOW;

    // Set reset low before configuration changes
    // Clock polarity (0 == rising, 1 == falling)
    // 8-bit character
    SpibRegs.SPICCR.bit.SPISWRESET = 0;
    SpibRegs.SPICCR.bit.HS_MODE = 1;
    SpibRegs.SPICCR.bit.CLKPOLARITY = 0;
    SpibRegs.SPICCR.bit.SPICHAR = (8-1);

    // Enable master (0 == slave, 1 == master)
    // Enable transmission (Talk)
    // Clock phase (0 == normal, 1 == delayed)
    // SPI interrupts are disabled
    SpibRegs.SPICTL.bit.MASTER_SLAVE = 1;
    SpibRegs.SPICTL.bit.TALK = 1;
    SpibRegs.SPICTL.bit.CLK_PHASE = 0;

    // Set the baud rate
    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0;      //LSPCLK = 200 MHz
    SpibRegs.SPIBRR.bit.SPI_BIT_RATE = 9;     //SPI Baud Rate = 200/9+1 = 20 MHz
//    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 1;            //LSPCLK = 100 MHz
//    SpibRegs.SPIBRR.bit.SPI_BIT_RATE = 99;          //SPI Baud Rate = 100/100 = 1 MHz

    // Set FREE bit
    // Halting on a breakpoint will not halt the SPI
    SpibRegs.SPIPRI.bit.FREE = 1;

    // Release the SPI from reset
    SpibRegs.SPICCR.bit.SPISWRESET = 1;
}
