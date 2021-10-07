.global _start @全局标号

_start:
    /*使能所有外设时钟 */
    ldr r0, =0x020c4068 @CCGR0的地址，是立即数
    ldr r1, =0xffffffff @全部使能
    str r1, [r0] @将r1里的值写入r0中值解析出来对应地址中

    ldr r0, =0x020c406c @CCGR1的地址，是立即数
    str r1, [r0] @将r1里的值写入r0中值解析出来对应地址中

    ldr r0, =0x020c4070 @CCGR2的地址，是立即数
    str r1, [r0] @将r1里的值写入r0中值解析出来对应地址中

    ldr r0, =0x020c4074 @CCGR3的地址，是立即数
    str r1, [r0] @将r1里的值写入r0中值解析出来对应地址中

    ldr r0, =0x020c4078 @CCGR4的地址，是立即数
    str r1, [r0] @将r1里的值写入r0中值解析出来对应地址中

    ldr r0, =0x020c407c @CCGR5的地址，是立即数
    str r1, [r0] @将r1里的值写入r0中值解析出来对应地址中

    ldr r0, =0x020c4080 @CCGR6的地址，是立即数
    str r1, [r0] @将r1里的值写入r0中值解析出来对应地址中

    /*IO复用：GPIO1_IO03复用为GPIO */
    ldr r0, =0x020e0068 @GPIO1_IO03复用配置寄存器地址
    ldr r1, =0x5 @GPIO复用
    str r1, [r0]

    /*配置电气属性 */
    ldr r0, =0x020e02f4 @GPIO1_IO03复用配置寄存器地址
    ldr r1, =0x10b0
    str r1, [r0]

    /*设置GPIO */
    ldr r0, =0x0209c004 @GPIO输入输出寄存器地址
    ldr r1, =0x8 @bit3为输出
    str r1, [r0]

    /*点亮LED，设置IO03输出为低电平 */
    ldr r0, =0x0209c000 @GPIO数据寄存器地址
    ldr r1, =0 @全部为低电平
    str r1, [r0]



loop:
    b loop
