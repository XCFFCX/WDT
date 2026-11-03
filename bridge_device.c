#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "qapi/error.h"
#include "qemu/main-loop.h"
#include "qemu/timer.h"
#include "hw/misc/bridge_device.h"
#include <stdint.h>

#define BRIDGE_TIMER_SPI  10
#define BRIDGE_GPIO_SPI   11
#define SYNC_INTERVAL_NS  10000

// 外部 C 接口声明
extern void sc_bridge_init(void);
extern uint32_t sc_bridge_read(uint64_t addr);
extern void sc_bridge_write(uint64_t addr, uint32_t val);
extern void sc_advance_time(uint64_t ns);
extern int  sc_bridge_check_irq(int* out_source, int* out_state);

static bool g_systemc_initialized;

// ---------------- 关键修复：补回 joint_step ----------------
void joint_step(CPUState *cpu, int64_t ns) {
    // L6 中时间同步由 QEMU Timer 驱动，这里留空即可
}
// -----------------------------------------------------------

static uint64_t bridge_read(void *opaque, hwaddr addr, unsigned size) {
    return sc_bridge_read(addr);
}

static void bridge_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
    sc_bridge_write(addr, (uint32_t)val);
}

static const MemoryRegionOps bridge_mmio_ops = {
    .read = bridge_read,
    .write = bridge_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = { .min_access_size = 4, .max_access_size = 4 },
};

static void bridge_sync_timer_cb(void *opaque) {
    BridgeDeviceState *s = (BridgeDeviceState *)opaque;
    int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    int64_t delta = now - s->last_sync_time;

    // 限制单步最大跳跃
    if (delta > SYNC_INTERVAL_NS) {
        delta = SYNC_INTERVAL_NS;
    }

    if (delta > 0) {
        sc_advance_time((uint64_t)delta);
        s->last_sync_time += delta; // 必须用 +=
    }

    int irq_source, irq_state;
    while (sc_bridge_check_irq(&irq_source, &irq_state)) {
        if (irq_source >= 0 && irq_source < 3) {
            qemu_set_irq(s->irq[irq_source], irq_state);
            // 保留这一行，因为它证明了中断注入的成功
            fprintf(stderr, "[Bridge] IRQ Inject: source=%d, state=%d\n", irq_source, irq_state);
        }
    }

    timer_mod(s->sync_timer, now + SYNC_INTERVAL_NS);
}
static void bridge_init(Object *obj) {
    BridgeDeviceState *s = (BridgeDeviceState *)obj;

    if (!g_systemc_initialized) {
        sc_bridge_init();
        g_systemc_initialized = true;
    }

    memory_region_init_io(&s->mmio, obj, &bridge_mmio_ops, s, "bridge-mmio", BRIDGE_MMIO_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void bridge_realize(DeviceState *dev, Error **errp) {
    BridgeDeviceState *s = (BridgeDeviceState *)dev;
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq[0]);
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq[1]);
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq[2]);
    s->sync_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, bridge_sync_timer_cb, s);
    s->last_sync_time = 0;
    timer_mod(s->sync_timer, SYNC_INTERVAL_NS);
    
    fprintf(stderr, "[bridge] L6: time sync timer created, interval=%d ns\n", SYNC_INTERVAL_NS);
}

static void bridge_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = bridge_realize;
}

static const TypeInfo bridge_info = {
    .name = TYPE_BRIDGE_DEVICE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BridgeDeviceState),
    .instance_init = bridge_init,
    .class_init = bridge_class_init,
};

static void bridge_register_types(void) {
    type_register_static(&bridge_info);
}
type_init(bridge_register_types)