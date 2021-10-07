#include "fpu.h"

void imx6ul_hardfpu_enable(void)
{
    uint32_t cpacr;
    uint32_t fpexc;

    /* 使能NEON和FPU */
    cpacr = __get_CPACR();
    cpacr = (cpacr & ~(CPACR_ASEDIS_Msk | CPACR_D32DIS_Msk)) | (3UL << CPACR_cp10_Pos) | (3UL << CPACR_cp11_Pos);
    __set_CPACR(cpacr);

    fpexc = __get_FPEXC();
    fpexc |= 0x40000000UL;
    __set_FPEXC(fpexc);
}