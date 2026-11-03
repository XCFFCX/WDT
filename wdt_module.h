// wdt_module.h
#ifndef WDT_MODULE_H
#define WDT_MODULE_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <iostream>

// 寄存器偏移
#define WDT_LOAD_OFFSET    0x00
#define WDT_VALUE_OFFSET   0x04
#define WDT_CTRL_OFFSET    0x08
#define WDT_INTCLR_OFFSET  0x0C
#define WDT_RIS_OFFSET     0x10

// 控制位
#define WDT_CTRL_EN        (1 << 0)
#define WDT_CTRL_INT_EN    (1 << 1)
#define WDT_CTRL_RESET_EN  (1 << 2)

class WdtDevice : public sc_module {
public:
    // TLM 目标 socket，供 Bridge 连接
    tlm_utils::simple_target_socket<WdtDevice> socket;
    
    // 中断输出端口（连接到 Bridge 的 IRQ 收集器）
    sc_out<bool> irq;

    SC_CTOR(WdtDevice) : socket("socket"), irq("irq") {
        socket.register_b_transport(this, &WdtDevice::b_transport);
        
        // 用于倒计时的线程
        SC_THREAD(wdt_thread);
        
        // 用于更新中断信号的单一驱动进程（避免多写者报错）
        SC_METHOD(irq_driver);
        sensitive << irq_update_event;
        dont_initialize();
    }

private:
    uint32_t m_load = 0;
    uint32_t m_value = 0;
    uint32_t m_ctrl = 0;
    uint32_t m_ris = 0; // 超时标志

    sc_event irq_update_event;

    void b_transport(tlm::tlm_generic_payload& gp, sc_time& delay);
    void wdt_thread();
    void irq_driver();
};

#endif