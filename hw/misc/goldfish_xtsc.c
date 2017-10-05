/* Copyright (C) 2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/misc/goldfish_xtsc.h"
#include "hw/xtsc.h"
#include "exec/address-spaces.h"

#define GOLDFISH_XTSC(obj) OBJECT_CHECK(goldfish_xtsc, (obj), TYPE_GOLDFISH_XTSC)

/**
  A goldfish pstore is currently an area of reservered memory that survives reboots. The memory
  is persisted to disk if the filename parameter has been given. We persist the memory region
  when the device is unrealized.
*/
typedef struct goldfish_xtsc {
    DeviceState parent;

    void *ram_ptr;
    MemoryRegion memory;
    uint64_t size;   /* Size of the region in bytes, keep in mind the larger this
                        gets, the longer writes on exit take */
    hwaddr   addr;   /* Physical guest address where this memory shows up */
    char *name;
} goldfish_xtsc;

static Property goldfish_xtsc_properties[] = {
    DEFINE_PROP_UINT64(GOLDFISH_XTSC_ADDR_PROP, goldfish_xtsc, addr, 0),
    DEFINE_PROP_SIZE(GOLDFISH_XTSC_SIZE_PROP, goldfish_xtsc, size, 0),
    DEFINE_PROP_STRING(GOLDFISH_XTSC_NAME_PROP, goldfish_xtsc, name),
    DEFINE_PROP_END_OF_LIST(),
};

static Object *obj;

Object *goldfish_xtsc_device(void)
{
    return obj;
}

hwaddr goldfish_xtsc_get_addr(Object *obj)
{
  goldfish_xtsc *s = GOLDFISH_XTSC(obj);
  return s->addr;
}

uint64_t goldfish_xtsc_get_size(Object *obj)
{
  goldfish_xtsc *s = GOLDFISH_XTSC(obj);
  return s->size;
}

static void goldfish_xtsc_realize(DeviceState *dev, Error **errp) {
  goldfish_xtsc *s = GOLDFISH_XTSC(dev);

  s->ram_ptr = xtsc_open_shared_memory(s->name ? s->name : "SharedRAM_L",
				       s->size);
  // Reserve a slot of memory that we will use for XTSC shared memory.
  memory_region_init_ram_ptr(&s->memory, OBJECT(s), GOLDFISH_XTSC_DEV_ID, s->size,
			     s->ram_ptr);

  // Ok, now we just need to move it to the right physical address.
  memory_region_add_subregion(get_system_memory(), s->addr, &s->memory);
  obj = OBJECT(dev);
}

static void goldfish_xtsc_unrealize(DeviceState *dev, Error **errp) {
  goldfish_xtsc *s = GOLDFISH_XTSC(dev);

  memory_region_del_subregion(get_system_memory(), &s->memory);
  obj = NULL;
}

static void goldfish_dev_init(Object *obj) {
  // ID registration needs to happen before realization, otherwise
  // the device will show up with some unknown name in the dev tree
  // or with a name that has been handed through command line parameters.
  // We don't want any of that, so we set the device-id right here.
  goldfish_xtsc *s = GOLDFISH_XTSC(obj);
  s->parent.id = GOLDFISH_XTSC_DEV_ID;
}

static void goldfish_xtsc_class_init(ObjectClass *klass, void *data) {
  DeviceClass *dc = DEVICE_CLASS(klass);

  dc->realize = goldfish_xtsc_realize;
  dc->unrealize = goldfish_xtsc_unrealize;
  dc->desc = "goldfish XTSC connector";
  dc->props = goldfish_xtsc_properties;
}

static const TypeInfo goldfish_xtsc_info = {
    .name = TYPE_GOLDFISH_XTSC,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(goldfish_xtsc),
    .instance_init = goldfish_dev_init,
    .class_init = goldfish_xtsc_class_init,
};

static void goldfish_xtsc_register(void) {
  type_register_static(&goldfish_xtsc_info);
}

type_init(goldfish_xtsc_register);
