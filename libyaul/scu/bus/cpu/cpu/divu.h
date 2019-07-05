/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _CPU_DIVU_H_
#define _CPU_DIVU_H_

#include <cpu/instructions.h>
#include <cpu/map.h>

#include <fix16.h>

__BEGIN_DECLS

static inline bool __always_inline
cpu_divu_status_get(void)
{
        return MEMORY_READ(32, CPU(DVCR)) & 0x00000001;
}

static inline uint32_t __always_inline
cpu_divu_quotient_get(void)
{
        return MEMORY_READ(32, CPU(DVDNTL));
}

static inline uint32_t __always_inline
cpu_divu_remainder_get(void)
{
        return MEMORY_READ(32, CPU(DVDNTH));
}

static inline void __always_inline
cpu_divu_64_32_set(uint32_t dividendh, uint32_t dividendl, uint32_t divisor)
{
        MEMORY_WRITE(32, CPU(DVSR), divisor);
        MEMORY_WRITE(32, CPU(DVDNTH), dividendh);
        /* Writing to CPU(DVDNTL) starts the operation */
        MEMORY_WRITE(32, CPU(DVDNTL), dividendl);
}

static inline void __always_inline
cpu_divu_32_32_set(uint32_t dividend, uint32_t divisor)
{
        MEMORY_WRITE(32, CPU(DVSR), divisor);
        /* Writing to CPU(DVDNT) starts the operation */
        MEMORY_WRITE(32, CPU(DVDNT), dividend);
}

static inline void __always_inline
cpu_divu_fix16_set(fix16_t dividend, fix16_t divisor)
{
        uint32_t dh;
        dh = cpu_instr_swapw(dividend);
        dh = cpu_instr_extsw(dh);

        uint32_t dl;
        dl = dividend << 16;

        cpu_divu_64_32_set(dh, dl, divisor);
}

static inline void __always_inline
cpu_divu_interrupt_priority_set(uint8_t priority)
{
        register uint16_t ipra;
        ipra = MEMORY_READ(16, CPU(IPRA));

        ipra = (ipra & 0x0FFF) | ((priority & 0x0F) << 12);

        MEMORY_WRITE(16, CPU(IPRA), ipra);
}

#define cpu_divu_ovfi_clear() do {                                             \
        cpu_divu_ovfi_set(NULL);                                               \
} while (false)

extern void cpu_divu_init(void);
extern void cpu_divu_ovfi_set(void (*)(void));

__END_DECLS

#endif /* !_CPU_DIVU_H_ */
