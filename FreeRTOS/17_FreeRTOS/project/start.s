.global _start @ _start表明代码入口

.extern FreeRTOS_IRQ_Handler
.extern FreeRTOS_SWI_Handler

_start:
    ldr pc, =Reset_Handler @复位中断服务函数
    ldr pc, =Undefined_Handler @未定义指令中断服务函数
    ldr pc, =SVC_Handler @SVC
    ldr pc, =PreAbort_Handler @预取终止
    ldr pc, =DataAbort_Handler @数据终止
    ldr pc, =NotUsed_Handler @未使用
    ldr pc, =IRQ_Handler @IRQ中断
    ldr pc, =FIQ_Handler @FIQ中断

_swi:   .word FreeRTOS_SWI_Handler
_irq:   .word FreeRTOS_IRQ_Handler

/* 复位中断服务函数 */
Reset_Handler:
    cpsid i @关闭IRQ
    /* 关闭I、D Cache和MMU
     * 修改CP15协处理器的SCTLR寄存器，采用读改写的方式（为了不改其他位）
     */
    MRC p15, 0, r0, c1, c0, 0 @读取SCTLR寄存器数据到r0寄存器中
    bic r0, r0, #(1 << 12) @关闭I Cache
    bic r0, r0, #(1 << 11) @关闭分支预测
    bic r0, r0, #(1 << 2) @关闭D Cache
    bic r0, r0, #(1 << 1) @关闭对齐
    bic r0, r0, #(1 << 0) @关闭MMU
    MCR p15, 0, r0, c1, c0, 0 @r0寄存器中数据写入到SCTLR寄存器

#if 0
    /* 设置中断向量偏移（无论是汇编还是C，只要在中断未发生前设置好偏移就可以） */
    ldr r0, =0x87800000
    dsb @数据同步隔离，前面指令访问存储器操作完毕才会执行其后面指令
    isb @指令同步隔离，清洗流水线，保证前面指令全部完成才会执行其后面指令
    MCR p15, 0, r0, c12, c0, 0
    dsb
    isb
#endif

.global _bss_start @类似宏定义变量：_bss_start = __bss_start
_bss_start:
    .word __bss_start @bss的首地址

.global _bss_end @类似宏定义变量：_bss_end = __bss_end
_bss_end:
    .word __bss_end @bss的末地址

    /* 清除bss段 */
    ldr r0, _bss_start @r0保存bss起始地址
    ldr r1, _bss_end @r1保存bss结束地址
    mov r2, #0 @r2保存为0
bss_loop:
    stmia r0!, {r2} @stmia表示将r2里的值写到r0里存的地址上，然后r0里的地址自增一步
    cmp r0, r1 @比较r0和r1
    ble bss_loop @如果r0地址小于等于r1，继续清除bss段

    /* 设置处理器进入IRQ模式并设置sp指针 */
    mrs r0, cpsr @读取cpsr到r0
    bic r0, r0, #0x1f @清除cpsr的bit4-0
    orr r0, r0, #0x12 @按位或，将cpsr的bit4-0写成10010（IRQ模式）
    msr cpsr, r0 @将r0写入cpsr中
    ldr sp, =0x80600000 @设置sp指针，栈范围为0x80400000-0x80600000

    /* 设置处理器进入SYS模式并设置sp指针 */
    mrs r0, cpsr @读取cpsr到r0
    bic r0, r0, #0x1f @清除cpsr的bit4-0
    orr r0, r0, #0x1f @按位或，将cpsr的bit4-0写成11111（SYS模式）
    msr cpsr, r0 @将r0写入cpsr中
    ldr sp, =0x80400000 @设置sp指针，栈范围为0x80200000-0x80400000

    /* 设置处理器进入SVC模式并设置sp指针 */
    mrs r0, cpsr @读取cpsr到r0
    bic r0, r0, #0x1f @清除cpsr的bit4-0
    orr r0, r0, #0x13 @按位或，将cpsr的bit4-0写成10011（SVC模式）
    msr cpsr, r0 @将r0写入cpsr中
    ldr sp, =0x80200000 @设置sp指针，DDR内存起始地址为0x80000000，栈大小为0x00200000

    cpsie i @打开IRQ

    b main @跳转到C语言的main函数

/* 未定义指令中断服务函数 */
Undefined_Handler:
    ldr r0, =Undefined_Handler
    bx r0

/* SVC中断（FreeRTOS在portASM.s中已经定义好了SVC中断处理函数，将他与中断向量表关联起来） */
SVC_Handler:
    ldr   pc, _swi

/* 预取终止 */
PreAbort_Handler:
    ldr r0, =PreAbort_Handler
    bx r0

/* 数据终止 */
DataAbort_Handler:
    ldr r0, =DataAbort_Handler
    bx r0

/* 未使用 */
NotUsed_Handler:
    ldr r0, =NotUsed_Handler
    bx r0

/* IRQ中断（FreeRTOS在portASM.s中已经定义好了IRQ中断处理函数，将他与中断向量表关联起来） */
IRQ_Handler:
    ldr   pc, _irq

/* FIQ中断 */
FIQ_Handler:
    ldr r0, =FIQ_Handler
    bx r0

