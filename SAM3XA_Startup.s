/*****************************************************************************
 * Copyright (c) 2012 Rowley Associates Limited.                             *
 *                                                                           *
 * This file may be distributed under the terms of the License Agreement     *
 * provided with this software.                                              *
 *                                                                           *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE   *
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. *
 *****************************************************************************/

/*****************************************************************************
 *                           Preprocessor Definitions
 *                           ------------------------
 *
 * STARTUP_FROM_RESET
 *
 *   If defined, the program will startup from power-on/reset. If not defined
 *   the program will just loop endlessly from power-on/reset.
 *
 *   This definition is not defined by default on this target because the
 *   debugger is unable to reset this target and maintain control of it over the
 *   JTAG interface. The advantage of doing this is that it allows the debugger
 *   to reset the CPU and run programs from a known reset CPU state on each run.
 *   It also acts as a safety net if you accidently download a program in FLASH
 *   that crashes and prevents the debugger from taking control over JTAG
 *   rendering the target unusable over JTAG. The obvious disadvantage of doing
 *   this is that your application will not startup without the debugger.
 *
 *   We advise that on this target you keep STARTUP_FROM_RESET undefined whilst
 *   you are developing and only define STARTUP_FROM_RESET when development is
 *   complete.
 *
 * NO_SYSTEM_INIT
 *
 *   If defined, the SystemInit() function will NOT be called. By default 
 *   SystemInit() is called after reset to enable the clocks and memories to
 *   be initialised prior to any C startup initialisation.
 *
 * NO_WATCHDOG_DISABLE
 *
 *   If defined, watchdog is not disabled.
 *
 * NO_USER_RESET
 *
 *   If defined, user resets are not enabled.
 *
 *****************************************************************************/

#define WDT_MR_WDDIS 0x8000
#define WDT_BASE_ADDRESS 0x400E1A50
#define WDT_MR_BASE_OFFSET 0x4
#define RSTC_BASE_ADDRESS 0x400E1A00
#define RSTC_MR_BASE_OFFSET 0x8



  .global reset_handler

//  .extern xPortPendSVHandler
//  .extern xPortSysTickHandler
//  .extern vPortSVCHandler

  .syntax unified

  .section .vectors, "ax"
  .code 16
  .align 0
  .global _vectors

.macro DEFAULT_ISR_HANDLER name=
  .thumb_func
  .weak \name
\name:
1: b 1b /* endless loop */
.endm

_vectors:
  .word __stack_end__
#ifdef STARTUP_FROM_RESET

  .word reset_handler
#else
  .word reset_wait
#endif /* STARTUP_FROM_RESET */
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0 // Reserved
  .word 0 // Reserved
  .word 0 // Reserved
  .word 0 // Reserved
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0 // Reserved
  .word PendSV_Handler
  .word SysTick_Handler
//  .word vPortSVCHandler
//  .word DebugMon_Handler
//  .word 0 // Reserved
//  .word xPortPendSVHandler
//  .word xPortSysTickHandler
  .word SUPC_Handler
  .word RSTC_Handler
  .word RTC_Handler
  .word RTT_Handler
  .word WDT_Handler
  .word PMC_Handler
  .word EFC0_Handler
  .word EFC1_Handler
  .word UART_Handler
  .word SMC_Handler
  .word SDRAMC_Handler
  .word PIOA_Handler
  .word PIOB_Handler
  .word PIOC_Handler
  .word PIOD_Handler
  .word PIOE_Handler
  .word PIOF_Handler
  .word USART0_Handler
  .word USART1_Handler
  .word USART2_Handler
  .word USART3_Handler
  .word HSMCI_Handler
  .word TWI0_Handler
  .word TWI1_Handler
  .word SPI0_Handler
  .word SPI1_Handler
  .word SSC_Handler
  .word TC0_Handler
  .word TC1_Handler
  .word TC2_Handler
  .word TC3_Handler
  .word TC4_Handler
  .word TC5_Handler
  .word TC6_Handler
  .word TC7_Handler
  .word TC8_Handler
  .word PWM_Handler
  .word ADC_Handler
  .word DACC_Handler
  .word DMAC_Handler
  .word UOTGHS_Handler
  .word TRNG_Handler
  .word EMAC_Handler
  .word CAN0_Handler
  .word CAN1_Handler

  .section .init, "ax"
  .thumb_func

  reset_handler:
#ifndef NO_SYSTEM_INIT
  ldr sp, =__RAM_segment_end__
  ldr r0, =SystemInit
  blx r0
#endif

#ifndef NO_WATCHDOG_DISABLE
  /* Disable Watchdog */
  ldr r1, =WDT_BASE_ADDRESS
  ldr r0, =WDT_MR_WDDIS
  str r0, [r1, #WDT_MR_BASE_OFFSET]
#endif

#ifndef NO_USER_RESET
  /* Enable user reset */
  ldr r1, =RSTC_BASE_ADDRESS
  ldr r0, =0xA5000001
  str r0, [r1, #RSTC_MR_BASE_OFFSET]
#endif

  /* Configure vector table offset register */
  ldr r0, =0xE000ED08
  ldr r1, =_vectors
  str r1, [r0]

  b _start

DEFAULT_ISR_HANDLER NMI_Handler
DEFAULT_ISR_HANDLER HardFault_Handler
DEFAULT_ISR_HANDLER MemManage_Handler
DEFAULT_ISR_HANDLER BusFault_Handler
DEFAULT_ISR_HANDLER UsageFault_Handler
//DEFAULT_ISR_HANDLER vPortSVCHandler 
//DEFAULT_ISR_HANDLER DebugMon_Handler
//DEFAULT_ISR_HANDLER xPortPendSVHandler
//DEFAULT_ISR_HANDLER xPortSysTickHandler 
DEFAULT_ISR_HANDLER SVC_Handler 
DEFAULT_ISR_HANDLER DebugMon_Handler
DEFAULT_ISR_HANDLER PendSV_Handler
DEFAULT_ISR_HANDLER SysTick_Handler 
DEFAULT_ISR_HANDLER SUPC_Handler
DEFAULT_ISR_HANDLER RSTC_Handler
DEFAULT_ISR_HANDLER RTC_Handler
DEFAULT_ISR_HANDLER RTT_Handler
DEFAULT_ISR_HANDLER WDT_Handler
DEFAULT_ISR_HANDLER PMC_Handler
DEFAULT_ISR_HANDLER EFC0_Handler
DEFAULT_ISR_HANDLER EFC1_Handler
DEFAULT_ISR_HANDLER UART_Handler
DEFAULT_ISR_HANDLER SMC_Handler
DEFAULT_ISR_HANDLER SDRAMC_Handler
DEFAULT_ISR_HANDLER PIOA_Handler
DEFAULT_ISR_HANDLER PIOB_Handler
DEFAULT_ISR_HANDLER PIOC_Handler
DEFAULT_ISR_HANDLER PIOD_Handler
DEFAULT_ISR_HANDLER PIOE_Handler
DEFAULT_ISR_HANDLER PIOF_Handler
DEFAULT_ISR_HANDLER USART0_Handler
DEFAULT_ISR_HANDLER USART1_Handler
DEFAULT_ISR_HANDLER USART2_Handler
DEFAULT_ISR_HANDLER USART3_Handler
DEFAULT_ISR_HANDLER HSMCI_Handler
DEFAULT_ISR_HANDLER TWI0_Handler
DEFAULT_ISR_HANDLER TWI1_Handler
DEFAULT_ISR_HANDLER SPI0_Handler
DEFAULT_ISR_HANDLER SPI1_Handler
DEFAULT_ISR_HANDLER SSC_Handler
DEFAULT_ISR_HANDLER TC0_Handler
DEFAULT_ISR_HANDLER TC1_Handler
DEFAULT_ISR_HANDLER TC2_Handler
DEFAULT_ISR_HANDLER TC3_Handler
DEFAULT_ISR_HANDLER TC4_Handler
DEFAULT_ISR_HANDLER TC5_Handler
DEFAULT_ISR_HANDLER TC6_Handler
DEFAULT_ISR_HANDLER TC7_Handler
DEFAULT_ISR_HANDLER TC8_Handler
DEFAULT_ISR_HANDLER PWM_Handler
DEFAULT_ISR_HANDLER ADC_Handler
DEFAULT_ISR_HANDLER DACC_Handler
DEFAULT_ISR_HANDLER DMAC_Handler
DEFAULT_ISR_HANDLER UOTGHS_Handler
DEFAULT_ISR_HANDLER TRNG_Handler
DEFAULT_ISR_HANDLER EMAC_Handler
DEFAULT_ISR_HANDLER CAN0_Handler
DEFAULT_ISR_HANDLER CAN1_Handler

#ifndef STARTUP_FROM_RESET
DEFAULT_ISR_HANDLER reset_wait
#endif /* STARTUP_FROM_RESET */

