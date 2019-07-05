/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <dram-cart.h>

#include "dram-cart-internal.h"

static uint8_t _id = 0x00;
static void *_base = NULL;

static bool _detect_dram_cart(void);

void
dram_cart_init(void)
{
        /* Write to A-Bus "dummy" area */
        MEMORY_WRITE(16, DUMMY(UNKNOWN), 0x0001);

        /* Set the SCU wait */
        /* Don't ask about this magic constant */
        uint32_t asr0_bits;
        asr0_bits = MEMORY_READ(32, SCU(ASR0));
        MEMORY_WRITE(32, SCU(ASR0), 0x23301FF0);

        /* Write to A-Bus refresh */
        uint32_t aref_bits;
        aref_bits = MEMORY_READ(32, SCU(AREF));
        MEMORY_WRITE(32, SCU(AREF), 0x00000013);

        /* Determine ID and base address */
        if (!(_detect_dram_cart())) {
                /* Restore values in case we can't detect DRAM
                 * cartridge */
                MEMORY_WRITE(32, SCU(ASR0), asr0_bits);
                MEMORY_WRITE(32, SCU(AREF), aref_bits);
        }
}

void *
dram_cart_area_get(void)
{
        return _base;
}

uint8_t
dram_cart_id_get(void)
{
        return _id;
}

size_t
dram_cart_size_get(void)
{
        switch (_id) {
        case DRAM_CART_ID_1MIB:
                return 0x00100000;
        case DRAM_CART_ID_4MIB:
                return 0x00400000;
        default:
                return 0;
        }
}

static bool
_detect_dram_cart(void)
{
        /*
         *             8-Mbit DRAM            32-Mbit DRAM
         *             +--------------------+ +--------------------+
         * 0x224000000 | DRAM #0            | | DRAM #0            |
         *             +--------------------+ |                    |
         * 0x224800000 | DRAM #0 (mirrored) | |                    |
         *             +--------------------+ |                    |
         * 0x225000000 | DRAM #0 (mirrored) | |                    |
         *             +--------------------+ |                    |
         * 0x225800000 | DRAM #0 (mirrored) | |                    |
         *             +--------------------+ +--------------------+
         * 0x226000000 | DRAM #1            | | DRAM #1            |
         *             +--------------------+ |                    |
         * 0x226800000 | DRAM #1 (mirrored) | |                    |
         *             +--------------------+ |                    |
         * 0x227000000 | DRAM #1 (mirrored) | |                    |
         *             +--------------------+ |                    |
         * 0x227800000 | DRAM #1 (mirrored) | |                    |
         * 0x227FFFFFF +--------------------+ +--------------------+
         */

        /* Check the ID */
        _id = MEMORY_READ(8, CS0(ID));
        _id &= 0xFF;

        if ((_id != DRAM_CART_ID_1MIB) && (_id != DRAM_CART_ID_4MIB)) {
                _id = 0x00;
                _base = NULL;

                return false;
        }

        uint32_t b;
        for (b = 0; b < DRAM_CART_BANKS; b++) {
                MEMORY_WRITE(32, DRAM(0, b, 0x00000000), 0x00000000);
                MEMORY_WRITE(32, DRAM(1, b, 0x00000000), 0x00000000);
        }

        /* Check DRAM #0 for mirrored banks */
        uint32_t write;
        write = 0x5A5A5A5A;
        MEMORY_WRITE(32, DRAM(0, 0, 0x00000000), write);

        for (b = 1; b < DRAM_CART_BANKS; b++) {
                uint32_t read;
                read = MEMORY_READ(32, DRAM(0, b, 0x00000000));

                /* Is it mirrored? */
                if (read != write) {
                        continue;
                }

                /* Thanks to Joe Fenton or the suggestion to return the
                 * last mirrored DRAM #0 bank in order to get a
                 * contiguous address space */
                if (_id != DRAM_CART_ID_1MIB) {
                        _id = DRAM_CART_ID_1MIB;
                }

                _base = (void *)DRAM(0, 3, 0x00000000);

                return true;
        }

        if (_id != DRAM_CART_ID_4MIB) {
                _id = DRAM_CART_ID_4MIB;
        }

        _base = (void *)DRAM(0, 0, 0x00000000);

        return true;
}
