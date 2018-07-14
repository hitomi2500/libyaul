/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _BIOS_H_
#define _BIOS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CPU_CLOCK_SPEED_26MHZ   0
#define CPU_CLOCK_SPEED_28MHZ   1

static inline const uint32_t * __attribute__ ((always_inline))
bios_int_priority_table_get(void)
{
        /* Priority table is 0x80 bytes */
        register uint32_t *bios_address;
        bios_address = (uint32_t *)0x06000A80;

        return bios_address;
}

static inline void __attribute__ ((always_inline))
bios_clock_speed_change(uint8_t mode)
{
        register uint32_t *bios_address;
        bios_address = (uint32_t *)0x06000320;

        ((void (*)(uint8_t))*bios_address)(mode);
}

static inline uint32_t __attribute__ ((always_inline))
bios_clock_speed_get(void)
{
        register uint32_t *bios_address;
        bios_address = (uint32_t *)0x06000324;

        return *bios_address;
}

static inline void __attribute__ ((noreturn, always_inline))
bios_cd_player_execute(void)
{
        register uint32_t *bios_address;
        bios_address = (uint32_t *)0x0600026C;

        ((void (*)(void))*bios_address)();

        __builtin_unreachable();
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_BIOS_H_ */