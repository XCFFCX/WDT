#include "wdt_module.h"
#include <tlm_utils/simple_initiator_socket.h>

SC_MODULE(TestWdtInitiator) {
    tlm_utils::simple_initiator_socket<TestWdtInitiator> socket;
    sc_in<bool> irq_in;

    SC_CTOR(TestWdtInitiator) : socket("socket") {
        SC_THREAD(run);
        SC_METHOD(on_irq);
        sensitive << irq_in;
        dont_initialize();
    }

    void write_reg(uint64_t addr, uint32_t val) {
        tlm::tlm_generic_payload gp;
        sc_time delay = SC_ZERO_TIME;
        gp.set_command(tlm::TLM_WRITE_COMMAND);
        gp.set_address(addr);
        gp.set_data_ptr(reinterpret_cast<unsigned char*>(&val));
        gp.set_data_length(4);
        socket->b_transport(gp, delay);
    }

    void run() {
        std::cout << "[Test] Configuring WDT..." << std::endl;
        write_reg(WDT_LOAD_OFFSET, 1000);
        write_reg(WDT_CTRL_OFFSET, WDT_CTRL_EN | WDT_CTRL_INT_EN);
        
        wait(1500, SC_NS); // 等 1500ns，看是否触发
        std::cout << "[Test] Simulation End." << std::endl;
        sc_stop();
    }

    void on_irq() {
        if (irq_in.read()) {
            std::cout << "[Test] Caught WDT IRQ at " << sc_time_stamp() << std::endl;
            write_reg(WDT_INTCLR_OFFSET, 1); // 清中断
        }
    }
};

int sc_main(int argc, char* argv[]) {
    WdtDevice wdt("wdt");
    TestWdtInitiator tb("tb");
    sc_signal<bool> irq_sig;

    tb.socket.bind(wdt.socket);
    wdt.irq(irq_sig);
    tb.irq_in(irq_sig);

    sc_start();
    return 0;
}