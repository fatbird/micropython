/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>

#include "py/nlr.h"
#include "py/obj.h"

#include "extmod/machine_mem.h"

#if MICROPY_PLAT_DEV_MEM
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#define MICROPY_PAGE_SIZE 4096
#define MICROPY_PAGE_MASK (MICROPY_PAGE_SIZE - 1)
#endif

#if MICROPY_PY_MACHINE

uintptr_t machine_mem_get_read_addr(mp_obj_t addr_o, uint align) {
    uintptr_t addr = mp_obj_int_get_truncated(addr_o);
    if ((addr & (align - 1)) != 0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "address %08x is not aligned to %d bytes", addr, align));
    }
    #if MICROPY_PLAT_DEV_MEM
    {
        // Not thread-safe
        static int fd;
        static uintptr_t last_base = (uintptr_t)-1;
        static uintptr_t map_page;
        if (!fd) {
            fd = open("/dev/mem", O_RDWR | O_SYNC);
            if (fd == -1) {
                nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(errno)));
            }
        }

        uintptr_t cur_base = addr & ~MICROPY_PAGE_MASK;
        if (cur_base != last_base) {
            map_page = (uintptr_t)mmap(NULL, MICROPY_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, cur_base);
            last_base = cur_base;
        }
        addr = map_page + (addr & MICROPY_PAGE_MASK);
    }
    #endif

    return addr;
}

uintptr_t machine_mem_get_write_addr(mp_obj_t addr_o, uint align) {
    return machine_mem_get_read_addr(addr_o, align);
}

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_machine) },

    { MP_ROM_QSTR(MP_QSTR_mem8), MP_ROM_PTR(&machine_mem8_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem16), MP_ROM_PTR(&machine_mem16_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem32), MP_ROM_PTR(&machine_mem32_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .name = MP_QSTR_machine,
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};

#endif // MICROPY_PY_MACHINE