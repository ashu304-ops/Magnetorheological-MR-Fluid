.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.global g_pfnVectors
.global Reset_Handler
.global Default_Handler

.extern main

.extern vPortSVCHandler
.extern xPortPendSVHandler
.extern xPortSysTickHandler

.extern _estack
.extern _sidata
.extern _sdata
.extern _edata
.extern _sbss
.extern _ebss


/* ============================================================
 * Cortex-M4 interrupt vector table
 * ============================================================ */

.section .isr_vector,"a",%progbits
.align 2
.type g_pfnVectors, %object

g_pfnVectors:
    .word   _estack
    .word   Reset_Handler

    /* Cortex-M exceptions */
    .word   Default_Handler          /* NMI */
    .word   HardFault_Handler        /* HardFault */
    .word   MemManage_Handler        /* MemManage */
    .word   BusFault_Handler         /* BusFault */
    .word   UsageFault_Handler       /* UsageFault */

    .word   0
    .word   0
    .word   0
    .word   0

    .word   vPortSVCHandler          /* SVCall */
    .word   Default_Handler          /* DebugMonitor */
    .word   0
    .word   xPortPendSVHandler       /* PendSV */
    .word   xPortSysTickHandler     /* SysTick */

.size g_pfnVectors, . - g_pfnVectors


/* ============================================================
 * Reset Handler
 * ============================================================ */

.section .text.Reset_Handler,"ax",%progbits
.align 2
.thumb_func
.type Reset_Handler, %function

Reset_Handler:

    /* --------------------------------------------------------
     * Enable Cortex-M4F FPU
     *
     * SCB->CPACR = 0xE000ED88
     *
     * CP10 = full access
     * CP11 = full access
     *
     * bits 20-23 = 1111
     * -------------------------------------------------------- */

    ldr     r0, =0xE000ED88
    ldr     r1, [r0]

    /* Set CP10 + CP11 to full access */
    orr     r1, r1, #(0xF << 20)

    str     r1, [r0]

    dsb
    isb


    /* --------------------------------------------------------
     * Set VTOR = vector table
     * -------------------------------------------------------- */

    ldr     r0, =0xE000ED08
    ldr     r1, =g_pfnVectors
    str     r1, [r0]

    dsb
    isb


    /* --------------------------------------------------------
     * Copy .data FLASH -> RAM
     * -------------------------------------------------------- */

    ldr     r0, =_sidata
    ldr     r1, =_sdata
    ldr     r2, =_edata

.Ldata_copy:

    cmp     r1, r2
    bcs     .Lbss_zero

    ldr     r3, [r0], #4
    str     r3, [r1], #4

    b       .Ldata_copy


    /* --------------------------------------------------------
     * Zero .bss
     * -------------------------------------------------------- */

.Lbss_zero:

    ldr     r1, =_sbss
    ldr     r2, =_ebss
    movs    r3, #0

.Lbss_loop:

    cmp     r1, r2
    bcs     .Lcall_main

    str     r3, [r1], #4

    b       .Lbss_loop


    /* --------------------------------------------------------
     * Call C/C++ main
     * -------------------------------------------------------- */

.Lcall_main:

    bl      main


    /* main should never return */

.Lhang:

    b       .Lhang

.size Reset_Handler, . - Reset_Handler


/* ============================================================
 * Fault handlers
 * ============================================================ */

.section .text.HardFault_Handler,"ax",%progbits
.align 2
.thumb_func
.global HardFault_Handler
.type HardFault_Handler, %function

HardFault_Handler:

    b       Default_Handler


.section .text.MemManage_Handler,"ax",%progbits
.align 2
.thumb_func
.global MemManage_Handler
.type MemManage_Handler, %function

MemManage_Handler:

    b       Default_Handler


.section .text.BusFault_Handler,"ax",%progbits
.align 2
.thumb_func
.global BusFault_Handler
.type BusFault_Handler, %function

BusFault_Handler:

    b       Default_Handler


.section .text.UsageFault_Handler,"ax",%progbits
.align 2
.thumb_func
.global UsageFault_Handler
.type UsageFault_Handler, %function

UsageFault_Handler:

    b       Default_Handler


/* ============================================================
 * Default Handler
 * ============================================================ */

.section .text.Default_Handler,"ax",%progbits
.align 2
.thumb_func
.type Default_Handler, %function
.global Default_Handler

Default_Handler:

    b       Default_Handler

.size Default_Handler, . - Default_Handler

.end
