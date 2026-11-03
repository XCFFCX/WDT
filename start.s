.syntax unified
.arch armv7-a

.section .vectors, "ax"
.global _start
_start:
    b reset_handler          @ Reset
    b .                      @ Undefined
    b .                      @ SVC
    b .                      @ Prefetch Abort
    b .                      @ Data Abort
    b .                      @ Unused
    b irq_entry              @ IRQ
    b .                      @ FIQ

.section .text
reset_handler:
    @ 1. 设置 VBAR (异常向量表基地址)
    ldr r0, =_start
    mcr p15, 0, r0, c12, c0, 0
    isb

    @ 2. 设置 SVC 模式栈
    ldr sp, =0x48000000

    @ 3. 切换到 IRQ 模式，设置 IRQ 栈
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x12        @ IRQ mode (0x12)
    msr cpsr_c, r0
    ldr sp, =0x47000000

    @ 4. 切回 SVC 模式，暂时保持 IRQ 屏蔽
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x13        @ SVC mode (0x13)
    orr r0, r0, #0x80        @ 屏蔽 IRQ
    msr cpsr_c, r0

    bl main
hang:
    b hang

irq_entry:
    sub lr, lr, #4
    push {r0-r3, r12, lr}
    bl irq_handler
    pop {r0-r3, r12, lr}
    subs pc, lr, #4
