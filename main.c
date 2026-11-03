#include <stdint.h>

#define BRIDGE_BASE 0x09000000
#define UART_DR   (*(volatile uint32_t*)(BRIDGE_BASE + 0x00))
#define UART_FR   (*(volatile uint32_t*)(BRIDGE_BASE + 0x04))
#define WDT_BASE  (BRIDGE_BASE + 0x3000)
#define WDT_LOAD  (*(volatile uint32_t*)(WDT_BASE + 0x00))
#define WDT_CTRL  (*(volatile uint32_t*)(WDT_BASE + 0x08))
#define WDT_INTCLR (*(volatile uint32_t*)(WDT_BASE + 0x0C))

// GIC 寄存器（virt 机器）
#define GICD_BASE  0x08000000
#define GICC_BASE  0x08010000
#define GICD_CTLR       (*(volatile uint32_t*)(GICD_BASE + 0x000))
#define GICD_ISENABLER1 (*(volatile uint32_t*)(GICD_BASE + 0x104))
#define GICC_CTLR (*(volatile uint32_t*)(GICC_BASE + 0x000))
#define GICC_PMR  (*(volatile uint32_t*)(GICC_BASE + 0x004))
#define GICC_IAR  (*(volatile uint32_t*)(GICC_BASE + 0x00C))
#define GICC_EOIR (*(volatile uint32_t*)(GICC_BASE + 0x010))

#define WDT_IRQ_NUM 44

void uart_puts(const char *s) {
    while (*s) {
        while (UART_FR & (1 << 5)) { }
        UART_DR = *s++;
    }
}

void irq_handler(void) {
    uint32_t iar = GICC_IAR;
    uint32_t intid = iar & 0x3FF;
    
    if (intid == WDT_IRQ_NUM) {
        
        WDT_INTCLR = 1; // 清除中断
    }
    GICC_EOIR = iar;
}

void main(void) {
    uart_puts("WDT Test Start!\n");

    // 1. 初始化 GIC
    GICD_CTLR = 0;
    GICD_ISENABLER1 = (1 << (WDT_IRQ_NUM - 32));
    GICC_PMR = 0xFF;
    GICC_CTLR = 1;
    GICD_CTLR = 1;

    // 2. 开启 CPU 全局中断
    __asm__ volatile("cpsie i");

    // 3. 配置并启动 WDT (100000ns = 100us)
    WDT_LOAD = 100000;
    WDT_CTRL = 0x03; // EN | INT_EN

    uart_puts("WDT Started. Waiting for interrupts...\n");

    while (1) {
        __asm__ volatile("wfi"); // 等待中断
    }
}
