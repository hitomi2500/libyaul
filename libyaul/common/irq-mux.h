/*
 * Copyright (c) 2012 Israel Jacques
 * See LICENSE for details.
 *
 * Israel Jacques <mrko@eecs.berkeley.edu>
 */

#ifndef _IRQ_MUX_H_
#define _IRQ_MUX_H_

#include <inttypes.h>

#include <sys/queue.h>

struct irq_mux_handle;

typedef TAILQ_HEAD(irq_mux_tq, irq_mux_handle) irq_mux_tq_t;

typedef struct {
        uint8_t im_total;
        irq_mux_tq_t im_tq;
} irq_mux_t;

typedef struct irq_mux_handle irq_mux_handle_t;

struct irq_mux_handle {
        irq_mux_t *imh;
        void (*imh_hdl)(irq_mux_handle_t *);
        void *imh_user_ptr;

        TAILQ_ENTRY(irq_mux_handle) handles;
};

void irq_mux_handle(irq_mux_t *);
void irq_mux_handle_add(irq_mux_t *, void (*)(irq_mux_handle_t *), void *);
void irq_mux_init(irq_mux_t *);
void irq_mux_handle_remove(irq_mux_t *, void (*)(irq_mux_handle_t *));

#endif /* !_IRQ_MUX_H_ */