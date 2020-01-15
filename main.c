#include <F28x_Project.h>
#include <AIC23.h>
#include "interrupt.h"
#include "math.h"
#include "LCD.h"
#include "fpu_rfft.h"
#include "dsp.h"
#include "stdbool.h"

#pragma DATA_SECTION(ping, "ramgs1");
#pragma DATA_SECTION(pong, "ramgs2");
#pragma DATA_SECTION(RFFTinBuff, "ramgs3");
#pragma DATA_SECTION(RFFToutBuff, "ramgs4");
#pragma DATA_SECTION(RFFTmagBuff, "ramgs5");
#pragma DATA_SECTION(RFFTF32Coef, "ramgs6");

#define BURST6 1
#define TRANSFER6 N-1
#define BURST5 1
#define TRANSFER5 N-1

#define N 512
#define fs 48000
#define pi 3.14

#define RFFT_STAGES     8
#define RFFT_SIZE       (1 << RFFT_STAGES)

int16_t ping[N];
int16_t pong[N];
float RFFTinBuff[RFFT_SIZE];
float RFFToutBuff[RFFT_SIZE];
float RFFTmagBuff[RFFT_SIZE];
float RFFTF32Coef[RFFT_SIZE];
float H[N] = {0.08, 0.0801, 0.0806, 0.0813, 0.0822, 0.0835, 0.085,  0.0868, 0.0889, 0.0913, 0.0939, 0.0968, 0.1,    0.1034, 0.1071, 0.1111, 0.1153, 0.1198, 0.1245, 0.1295, 0.1347, 0.1402, 0.1459, 0.1519, 0.1581, 0.1645, 0.1712, 0.1781, 0.1852, 0.1925, 0.2001, 0.2078, 0.2157, 0.2239, 0.2322, 0.2407, 0.2494, 0.2583, 0.2673, 0.2765, 0.2859, 0.2954, 0.3051, 0.3149, 0.3249, 0.335,  0.3452, 0.3555, 0.3659, 0.3765, 0.3871, 0.3979, 0.4087, 0.4196, 0.4305, 0.4416, 0.4527, 0.4638, 0.475,  0.4863, 0.4976, 0.5089, 0.5202, 0.5315, 0.5428, 0.5542, 0.5655, 0.5768, 0.5881, 0.5993, 0.6106, 0.6217, 0.6329, 0.6439, 0.6549, 0.6659, 0.6767, 0.6875, 0.6982, 0.7088, 0.7193, 0.7297, 0.74,   0.7501, 0.7601, 0.77,   0.7797, 0.7893, 0.7988, 0.8081, 0.8172, 0.8262, 0.835,  0.8436, 0.852,  0.8602, 0.8683, 0.8761, 0.8837, 0.8912, 0.8984, 0.9054, 0.9121, 0.9187, 0.925,  0.9311, 0.9369, 0.9426, 0.9479, 0.953,  0.9579, 0.9625, 0.9669, 0.971,  0.9748, 0.9784, 0.9817, 0.9847, 0.9875, 0.9899, 0.9922, 0.9941, 0.9958, 0.9972, 0.9983, 0.9991, 0.9997, 1,  1,  0.9997, 0.9991, 0.9983, 0.9972, 0.9958, 0.9941, 0.9922, 0.9899, 0.9875, 0.9847, 0.9817, 0.9784, 0.9748, 0.971,  0.9669, 0.9625, 0.9579, 0.953,  0.9479, 0.9426, 0.9369, 0.9311, 0.925,  0.9187, 0.9121, 0.9054, 0.8984, 0.8912, 0.8837, 0.8761, 0.8683, 0.8602, 0.852,  0.8436, 0.835,  0.8262, 0.8172, 0.8081, 0.7988, 0.7893, 0.7797, 0.77,   0.7601, 0.7501, 0.74,   0.7297, 0.7193, 0.7088, 0.6982, 0.6875, 0.6767, 0.6659, 0.6549, 0.6439, 0.6329, 0.6217, 0.6106, 0.5993, 0.5881, 0.5768, 0.5655, 0.5542, 0.5428, 0.5315, 0.5202, 0.5089, 0.4976, 0.4863, 0.475,  0.4638, 0.4527, 0.4416, 0.4305, 0.4196, 0.4087, 0.3979, 0.3871, 0.3765, 0.3659, 0.3555, 0.3452, 0.335,  0.3249, 0.3149, 0.3051, 0.2954, 0.2859, 0.2765, 0.2673, 0.2583, 0.2494, 0.2407, 0.2322, 0.2239, 0.2157, 0.2078, 0.2001, 0.1925, 0.1852, 0.1781, 0.1712, 0.1645, 0.1581, 0.1519, 0.1459, 0.1402, 0.1347, 0.1295, 0.1245, 0.1198, 0.1153, 0.1111, 0.1071, 0.1034, 0.1,    0.0968, 0.0939, 0.0913, 0.0889, 0.0868, 0.085,  0.0835, 0.0822, 0.0813, 0.0806, 0.0801, 0.08};

RFFT_F32_STRUCT rfft;
RFFT_F32_STRUCT_Handle hnd_rfft = &rfft;

volatile int16 *DMA_CH6_Dest;
volatile int16 *DMA_CH6_Source;
volatile int16 *DMA_CH5_Dest;
volatile int16 *DMA_CH5_Source;

__interrupt void local_D_INTCH6_ISR(void);
__interrupt void local_D_INTCH5_ISR(void);

volatile bool done;


int main(void)
{
    InitSysCtrl();
    InitPieCtrl();
    InitPieVectTable();

    EALLOW;

    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;
    GpioDataRegs.GPASET.bit.GPIO0 = 1;

    hnd_rfft->FFTSize   = RFFT_SIZE;
    hnd_rfft->FFTStages = RFFT_STAGES;
    hnd_rfft->OutBuf    = &RFFToutBuff[0];  //Output buffer
    hnd_rfft->MagBuf    = &RFFTmagBuff[0];  //Magnitude buffer
    hnd_rfft->CosSinBuf = &RFFTF32Coef[0];
    RFFT_f32_sincostable(hnd_rfft);

    LCD_Init();

    //pie vector -> stolen from TI code
    EALLOW;  // This is needed to write to EALLOW protected registers
    PieVectTable.DMA_CH6_INT = &local_D_INTCH6_ISR;
    PieVectTable.DMA_CH5_INT = &local_D_INTCH5_ISR;
    EDIS;    // This is needed to disable write to EALLOW protected registers

    //Initialize DMA -> stolen from TI code
    DMAInitialize();

    // source and destination pointers
    DMA_CH6_Source = (volatile int16 *)&McbspbRegs.DRR1;
    DMA_CH6_Dest = (volatile int16 *)&ping;
    DMA_CH5_Source = (volatile int16 *)&pong;
    DMA_CH5_Dest = (volatile int16 *)&McbspbRegs.DXR1;


    // configure DMA CH6 -> modified from TI code
    DMACH6AddrConfig(DMA_CH6_Dest, DMA_CH6_Source);
    DMACH6BurstConfig(BURST6,-1,0);
    DMACH6TransferConfig(TRANSFER6,1,1);
    DMACH6ModeConfig(DMA_MREVTB,PERINT_ENABLE,ONESHOT_DISABLE,CONT_DISABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_ENABLE);

    DMACH5AddrConfig(DMA_CH5_Dest, DMA_CH5_Source);
    DMACH5BurstConfig(BURST5,0,-1);
    DMACH5TransferConfig(TRANSFER5,1,1);
    DMACH5ModeConfig(DMA_MXEVTB,PERINT_ENABLE,ONESHOT_DISABLE,CONT_DISABLE,
                        SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                        CHINT_END,CHINT_ENABLE);

    //something about a bandgap voltage -> stolen from TI code
    EALLOW;
    CpuSysRegs.SECMSEL.bit.PF2SEL = 1;
    EDIS;

    //interrupt enabling
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;   // Enable the PIE block
    PieCtrlRegs.PIEIER7.bit.INTx6 = 1;   // Enable PIE Group 7, INT 2 (DMA CH2)
    PieCtrlRegs.PIEIER7.bit.INTx5 = 1;
    IER |= M_INT7;                         // Enable CPU INT6
    EINT;                                // Enable Global Interrupts

    EnableInterrupts();

    StartDMACH6();      // Start DMA channel -> stolen from TI code
    StartDMACH5();

    InitBigBangedCodecSPI();
    InitMcBSPb();
    InitAIC23();

    //Init DSP board: -> replace this with your code
    EALLOW;

    uint16_t maxBin;
    uint16_t i;
    uint16_t j;

    while(1)
    {
        maxBin = 0;


        while(!done);

        if (DmaRegs.CH5.SRC_BEG_ADDR_ACTIVE == (Uint32)(&ping))
        {
            j = 0;
            for (i=0; i < N; i+=2)
            {
                RFFTinBuff[j++] = (float)((ping[i]>>1)+((ping[i+1])>>1));
            }
        }
        else
        {
            j = 0;
            for (i=0; i < N; i+=2)
            {
                RFFTinBuff[j++] = (float)((pong[i]>>1)+((pong[i+1])>>1));
            }
        }

        hnd_rfft->InBuf = &RFFTinBuff[0];

        for (i=0; i < RFFT_SIZE; i++){
             RFFToutBuff[i] = 0;               //Clean up output buffer
        }

        for (i=0; i <= RFFT_SIZE/2; i++){
             RFFTmagBuff[i] = 0;                //Clean up magnitude buffer
        }

        GpioDataRegs.GPATOGGLE.bit.GPIO0 = 1;
        RFFT_f32(hnd_rfft);
        RFFT_f32_mag(hnd_rfft);
        GpioDataRegs.GPATOGGLE.bit.GPIO0 = 1;

        for (int k = 0; k < N/2; k++)
        {
            if (RFFTmagBuff[k] > RFFTmagBuff[maxBin])
                maxBin = k;
        }

        Write_Command(0x80);
        Write_String("Max Freq:");
        Write_Number(((maxBin * fs)/N) * 2);
        Write_String("Hz");
        Write_Command(0xC0);
        Write_String("Max Mag:");
        Write_Number(10*log10(RFFTmagBuff[maxBin]));
        Write_String("dBs");

        done = false;
    }

}

//DMA interrupt -> modified from TI code
__interrupt void local_D_INTCH6_ISR(void)
{
    EALLOW;

    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7; // ACK to receive more interrupts
                                            // from this PIE group

    if (DmaRegs.CH6.DST_BEG_ADDR_ACTIVE == (Uint32)(&ping))
    {
        DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32)(&pong);
        DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32)(&pong);
    }
    else
    {
        DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32)(&ping);
        DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32)(&ping);
    }

    EDIS;

    //restart DMA -> Dave added this :}
    StartDMACH6();

}

__interrupt void local_D_INTCH5_ISR(void)
{
    EALLOW;

    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7; // ACK to receive more interrupts
                                            // from this PIE group

    if (DmaRegs.CH6.DST_BEG_ADDR_ACTIVE == (Uint32)(&ping))
    {
        DmaRegs.CH5.SRC_BEG_ADDR_SHADOW = (Uint32)(&pong);
        DmaRegs.CH5.SRC_ADDR_SHADOW = (Uint32)(&pong);
    }
    else
    {
        DmaRegs.CH5.SRC_BEG_ADDR_SHADOW = (Uint32)(&ping);
        DmaRegs.CH5.SRC_ADDR_SHADOW = (Uint32)(&ping);
    }

    EDIS;

    done = true;

    //restart DMA -> Dave added this :}
    StartDMACH5();

}
