/*
 * QEMU Bridge Device - QEMU MMIO <-> SystemC TLM
 */

#ifndef QEMU_BRIDGE_DEVICE_H
#define QEMU_BRIDGE_DEVICE_H

#include "hw/sysbus.h"
#include "hw/irq.h"
#include "qom/object.h"
#include "qemu/timer.h"   // L6: QEMUTimer 类型

#define TYPE_BRIDGE_DEVICE "bridge-device"
#define BRIDGE_DEVICE(obj) \
    OBJECT_CHECK(BridgeDeviceState, (obj), TYPE_BRIDGE_DEVICE)

#define BRIDGE_MMIO_SIZE  0x4000

typedef struct BridgeDeviceState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    qemu_irq irq[3];

    // L6 新增：时间同步定时器
    QEMUTimer *sync_timer;
    int64_t last_sync_time;
} BridgeDeviceState;

struct CPUState;
void joint_step(struct CPUState *cpu, int64_t ns);

#endif