/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <cpu/cache.h>

#include <vdp2.h>

#include <sys/dma-queue.h>

#include "cons.h"

#include "vdp2_font.inc"

#define STATE_IDLE              0x00
#define STATE_BUFFER_DIRTY      0x01
#define STATE_BUFFER_FLUSHING   0x02

static void _buffer_clear(void);
static void _buffer_area_clear(int32_t, int32_t, int32_t, int32_t);
static void _buffer_line_clear(int32_t, int32_t, int32_t);
static void _buffer_write(int32_t, int32_t, uint8_t);

static void _dma_font_handler(void *);
static void _dma_handler(void *);

typedef struct {
        uint32_t dma_reg_buffer[DMA_REG_BUFFER_WORD_COUNT];

        struct scrn_cell_format cell_format;

        uint16_t page_size;
        uint16_t page_width;
        uint16_t page_height;
        uint16_t *page_pnd;
        uint16_t page_clear_pnd;

        uint8_t state;

        uint8_t cols;
        uint8_t rows;
} dev_state_t;

static dev_state_t *_dev_state;

/* Remove { */
void cons_vdp2_init(void);
void cons_vdp2_buffer(const char *);
void cons_vdp2_flush(void);
/* } */

void
cons_vdp2_init(void)
{
        struct {
                /* A bit of a hack in order to align the indirect
                 * transfer table to a 32-byte boundary */
                unsigned int : 32;
                unsigned int : 32;
                unsigned int : 32;
                unsigned int : 32;
                unsigned int : 32;
                unsigned int : 32;
                unsigned int : 32;

                /* Holds transfers for font CPD and PAL */
                struct dma_xfer xfer_tbl[2];

                uint32_t reg_buffer[DMA_REG_BUFFER_WORD_COUNT];
        } *dma_font;

        static const cons_ops_t cons_op = {
                .clear = _buffer_clear,
                .area_clear = _buffer_area_clear,
                .line_clear = _buffer_line_clear,
                .write = _buffer_write
        };

        _dev_state = malloc(sizeof(dev_state_t));
        assert(_dev_state != NULL);

        _dev_state->cell_format.scf_scroll_screen = SCRN_NBG3;
        _dev_state->cell_format.scf_cc_count = SCRN_CCC_PALETTE_16;
        _dev_state->cell_format.scf_character_size = 1 * 1;
        _dev_state->cell_format.scf_pnd_size = 1; /* 1-word */
        _dev_state->cell_format.scf_auxiliary_mode = 0;
        _dev_state->cell_format.scf_cp_table = VRAM_ADDR_4MBIT(3, 0x00000);
        _dev_state->cell_format.scf_color_palette = CRAM_MODE_1_OFFSET(0, 0, 0);
        _dev_state->cell_format.scf_plane_size = 1 * 1;
        _dev_state->cell_format.scf_map.plane_a = VRAM_ADDR_4MBIT(3, 0x04000);
        _dev_state->cell_format.scf_map.plane_b = VRAM_ADDR_4MBIT(3, 0x08000);
        _dev_state->cell_format.scf_map.plane_c = VRAM_ADDR_4MBIT(3, 0x04000);
        _dev_state->cell_format.scf_map.plane_d = VRAM_ADDR_4MBIT(3, 0x08000);

        vdp2_scrn_cell_format_set(&_dev_state->cell_format);
        vdp2_scrn_priority_set(SCRN_NBG3, 7);
        vdp2_scrn_display_set(SCRN_NBG3, /* transparent = */ true);

        struct vram_cycp vram_cycp;

        vram_cycp.pt[0].t0 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t1 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[1].t0 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t1 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[2].t0 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t1 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[3].t0 = VRAM_CYCP_PNDR_NBG3;
        vram_cycp.pt[3].t1 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t4 = VRAM_CYCP_CHPNDR_NBG3;
        vram_cycp.pt[3].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t7 = VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_set(&vram_cycp);

        _dev_state->page_size = SCRN_CALCULATE_PAGE_SIZE(&_dev_state->cell_format);
        _dev_state->page_width = SCRN_CALCULATE_PAGE_WIDTH(&_dev_state->cell_format);
        _dev_state->page_height = SCRN_CALCULATE_PAGE_HEIGHT(&_dev_state->cell_format);

        /* Restricting the page to 64x32 avoids wasting space */
        _dev_state->page_size /= 2;

        /* PND value used to clear pages */
        _dev_state->page_clear_pnd = SCRN_PND_CONFIG_0(
                _dev_state->cell_format.scf_cp_table,
                _dev_state->cell_format.scf_color_palette,
                /* vf = */ 0,
                /* hf = */ 0);

        _dev_state->page_pnd = malloc(_dev_state->page_size);
        assert(_dev_state->page_pnd != NULL);

        struct dma_level_cfg dma_level_cfg;

        dma_font = malloc(sizeof(*dma_font));
        assert(dma_font != NULL);

        dma_level_cfg.dlc_mode = DMA_MODE_INDIRECT;
        dma_level_cfg.dlc_xfer.indirect = &dma_font->xfer_tbl[0];
        dma_level_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        dma_level_cfg.dlc_update = DMA_UPDATE_NONE;

        /* Font CPD */
        dma_font->xfer_tbl[0].len = FONT_SIZE;
        dma_font->xfer_tbl[0].dst = (uint32_t)_dev_state->cell_format.scf_cp_table;
        dma_font->xfer_tbl[0].src = CPU_CACHE_THROUGH | (uint32_t)&_font_cpd[0];

        /* Font PAL */
        dma_font->xfer_tbl[1].len = FONT_COLOR_COUNT * sizeof(color_rgb888_t);
        dma_font->xfer_tbl[1].dst = _dev_state->cell_format.scf_color_palette;
        dma_font->xfer_tbl[1].src = DMA_INDIRECT_TBL_END | CPU_CACHE_THROUGH | (uint32_t)&_font_pal[0];

        scu_dma_config_buffer(dma_font->reg_buffer, &dma_level_cfg);

        dma_queue_enqueue(dma_font->reg_buffer, DMA_QUEUE_TAG_VBLANK_IN,
            _dma_font_handler, dma_font);

        /* 64x32 page PND */
        dma_level_cfg.dlc_mode = DMA_MODE_DIRECT;
        dma_level_cfg.dlc_xfer.direct.len = _dev_state->page_size;
        dma_level_cfg.dlc_xfer.direct.dst = (uint32_t)_dev_state->cell_format.scf_map.plane_a;
        dma_level_cfg.dlc_xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)&_dev_state->page_pnd[0];
        dma_level_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        dma_level_cfg.dlc_update = DMA_UPDATE_NONE;

        scu_dma_config_buffer(_dev_state->dma_reg_buffer, &dma_level_cfg);

        _dev_state->state = STATE_BUFFER_DIRTY;
        _dev_state->cols = 40;
        _dev_state->rows = 28;

        cons_init(&cons_op, _dev_state->cols, _dev_state->rows);
}

void
cons_vdp2_buffer(const char *buffer)
{
        /* If we're flushing, we have to block */
        while ((_dev_state->state & STATE_BUFFER_FLUSHING) == STATE_BUFFER_FLUSHING) {
        }

        cons_buffer(buffer);
}

void
cons_vdp2_flush(void)
{
        if (_dev_state->state != STATE_BUFFER_DIRTY) {
                return;
        }

        _dev_state->state |= STATE_BUFFER_FLUSHING;

        dma_queue_enqueue(_dev_state->dma_reg_buffer, DMA_QUEUE_TAG_VBLANK_IN,
            _dma_handler, NULL);
}

static void
_buffer_clear(void)
{
        _dev_state->state |= STATE_BUFFER_DIRTY;

        (void)memset(_dev_state->page_pnd, 0x00, _dev_state->page_size);
}

static void
_buffer_area_clear(int32_t col_start, int32_t col_end, int32_t row_start,
    int32_t row_end)
{
        _dev_state->state |= STATE_BUFFER_DIRTY;

        int32_t row;
        for (row = row_start; row < row_end; row++) {
                int32_t col;
                for (col = col_start; col < col_end; col++) {
                        _dev_state->page_pnd[col + (row * _dev_state->page_width)] = _dev_state->page_clear_pnd;
                }
        }
}

static void
_buffer_line_clear(int32_t col_start, int32_t col_end, int32_t row)
{
        _dev_state->state |= STATE_BUFFER_DIRTY;

        int32_t col;
        for (col = col_start; col < col_end; col++) {
                _dev_state->page_pnd[col + (row * _dev_state->page_width)] = _dev_state->page_clear_pnd;
        }
}

static void
_buffer_write(int32_t col, int32_t row, uint8_t ch)
{
        _dev_state->state |= STATE_BUFFER_DIRTY;

        uint16_t pnd;
        pnd = SCRN_PND_CONFIG_0(
                _dev_state->cell_format.scf_cp_table + (ch * 0x20),
                _dev_state->cell_format.scf_color_palette,
                /* vf = */ 0,
                /* hf = */ 0);

        _dev_state->page_pnd[col + (row * _dev_state->page_width)] = pnd;
}

static void
_dma_font_handler(void *work)
{
        free(work);
}

static void
_dma_handler(void *work __unused)
{
        _dev_state->state &= ~(STATE_BUFFER_DIRTY | STATE_BUFFER_FLUSHING);
}
