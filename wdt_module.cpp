// wdt_module.cpp
#include "wdt_module.h"
#include "sc_irq_bridge.h"

void WdtDevice::b_transport(tlm::tlm_generic_payload& gp, sc_time& delay) {
    uint64_t addr = gp.get_address();
    uint32_t* data = reinterpret_cast<uint32_t*>(gp.get_data_ptr());

    if (gp.get_data_length() != 4 || data == nullptr) {
        gp.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return;
    }

    if (gp.get_command() == tlm::TLM_WRITE_COMMAND) {
        switch (addr) {
        case WDT_LOAD_OFFSET:
            m_load = *data;
            if (!(m_ctrl & WDT_CTRL_EN)) {
                m_value = m_load;
            }
            
            break;
        case WDT_CTRL_OFFSET:
            m_ctrl = *data;
            if (m_ctrl & WDT_CTRL_EN) {
                m_value = m_load;
                
            } else {
                
            }
            break;
        case WDT_INTCLR_OFFSET:
            if (*data & 0x1) {
                m_ris = 0;
                irq_update_event.notify(SC_ZERO_TIME);
                
                if (sc_get_irq_callback() != nullptr) {
                    sc_get_irq_callback()(2, 0);
                }
                
                
            }
            break;
        default:
            break;
        }
    } else if (gp.get_command() == tlm::TLM_READ_COMMAND) {
        switch (addr) {
        case WDT_VALUE_OFFSET:
            *data = m_value;
            break;
        case WDT_RIS_OFFSET:
            *data = m_ris;
            break;
        case WDT_CTRL_OFFSET:
            *data = m_ctrl;
            break;
        default:
            *data = 0;
            break;
        }
    }

    gp.set_response_status(tlm::TLM_OK_RESPONSE);
    delay += sc_time(5, SC_NS);
}

void WdtDevice::wdt_thread() {
    while (true) {
        wait(1, SC_NS);
        
        if ((m_ctrl & WDT_CTRL_EN) && m_value > 0) {
            m_value--;
            
            if (m_value == 0) {
                m_ctrl &= ~WDT_CTRL_EN;
                if (m_ctrl & WDT_CTRL_INT_EN) {
                    m_ris = 1;
                    irq_update_event.notify(SC_ZERO_TIME);
                    
                    if (sc_get_irq_callback() != nullptr) {
                        sc_get_irq_callback()(2, 1);
                    }
                    
                    std::cout << "[WDT] Timeout! IRQ Raised at " << sc_time_stamp() << std::endl;
                }
                if (m_ctrl & WDT_CTRL_RESET_EN) {
                    std::cout << "[WDT] Timeout! System Reset Triggered (Not implemented yet)" << std::endl;
                }
            }
        }
    }
}

void WdtDevice::irq_driver() {
    irq.write(m_ris != 0);
}
