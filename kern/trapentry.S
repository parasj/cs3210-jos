/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)                                                  \
        .data;                                                                  \
                .long name;                                                     \
        .text;                                                                  \
                .globl name;            /* define global symbol for 'name' */   \
                .type name, @function;  /* symbol type is function */           \
                .align 2;               /* align function definition */         \
                name:                   /* function starts here */              \
                pushl $(num);                                                   \
                jmp _alltraps;

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)                                             \
        .data;                                                                  \
                .long name;                                                     \
        .text;                                                                  \
                .globl name;                                                    \
                .type name, @function;                                          \
                .align 2;                                                       \
                name:                                                           \
                pushl $0;                                                       \
                pushl $(num);                                                   \
                jmp _alltraps;

/*
 * Vector nr.      Error code?     Name
 * =============================================================
 * 0 (0x0)         No              Divide-by-zero Error
 * 1 (0x1)         No              Debug
 * 2 (0x2)         No              Non-maskable Interrupt
 * 3 (0x3)         No              Breakpoint
 * 4 (0x4)         No              Overflow
 * 5 (0x5)         No              Bound Range Exceeded
 * 6 (0x6)         No              Invalid Opcode
 * 7 (0x7)         No              Device Not Available
 * 8 (0x8)         Yes (Zero)      Double Fault
 * 9 (0x9)         No              Coprocessor Segment Overrun
 * 10 (0xA)        Yes             Invalid TSS
 * 11 (0xB)        Yes             Segment Not Present
 * 12 (0xC)        Yes             Stack-Segment Fault
 * 13 (0xD)        Yes             General Protection Fault
 * 14 (0xE)        Yes             Page Fault
 * 15 (0xF)        No              Reserved
 * 16 (0x10)       No              x87 Floating-Point Exception
 * 17 (0x11)       Yes             Alignment Check
 * 18 (0x12)       No              Machine Check
 * 19 (0x13)       No              SIMD Floating-Point Exception
 * 20 (0x14)       No              Virtualization Exception
 * 21-29(0x15-0x1D)No              Reserved
 * 30 (0x1E)       Yes             Security Exception
 * 31 (0x1F)       No              Reserved
 * -               No              Triple Fault
 * IRQ 13          No              FPU Error Interrupt
 */

.data
.globl trapfuncs

trapfuncs:
        .text
        TRAPHANDLER_NOEC(trap_divide, T_DIVIDE)
        TRAPHANDLER_NOEC(trap_debug, T_DEBUG)
        TRAPHANDLER_NOEC(trap_nmi, T_NMI)
        TRAPHANDLER_NOEC(trap_brkpt, T_BRKPT)
        TRAPHANDLER_NOEC(trap_oflow, T_OFLOW)
        TRAPHANDLER_NOEC(trap_bound, T_BOUND)
        TRAPHANDLER_NOEC(trap_illop, T_ILLOP)
        TRAPHANDLER_NOEC(trap_device, T_DEVICE)
        TRAPHANDLER(trap_dblflt, T_DBLFLT)
        TRAPHANDLER_NOEC(trap_r9, 9)
        TRAPHANDLER(trap_tss, T_TSS)
        TRAPHANDLER(trap_segnp, T_SEGNP)
        TRAPHANDLER(trap_stack, T_STACK)
        TRAPHANDLER(trap_gpflt, T_GPFLT)
        TRAPHANDLER(trap_pgflt, T_PGFLT)
        TRAPHANDLER_NOEC(trap_r15, 15)
        TRAPHANDLER_NOEC(trap_fperr, T_FPERR)
        TRAPHANDLER(trap_align, T_ALIGN)
        TRAPHANDLER_NOEC(trap_mchk, T_MCHK)
        TRAPHANDLER_NOEC(trap_simderr, T_SIMDERR)
        TRAPHANDLER_NOEC(trap_r20, 20)
        TRAPHANDLER_NOEC(trap_r21, 21)
        TRAPHANDLER_NOEC(trap_r22, 22)
        TRAPHANDLER_NOEC(trap_r23, 23)
        TRAPHANDLER_NOEC(trap_r24, 24)
        TRAPHANDLER_NOEC(trap_r25, 25)
        TRAPHANDLER_NOEC(trap_r26, 26)
        TRAPHANDLER_NOEC(trap_r27, 27)
        TRAPHANDLER_NOEC(trap_r28, 28)
        TRAPHANDLER_NOEC(trap_r29, 29)
        TRAPHANDLER_NOEC(trap_r30, 30)
        TRAPHANDLER_NOEC(trap_r31, 31)
        

_alltraps:
        pushl %ds
        pushl %es
        pushal
        movw $GD_KD, %ax
        movw $GD_KD, %ax
        pushl %esp
        call trap
