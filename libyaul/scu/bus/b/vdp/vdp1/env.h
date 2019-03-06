/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _VDP1_ENV_H_
#define _VDP1_ENV_H_

#include <stdint.h>
#include <stdbool.h>

#include <color.h>
#include <int16.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VDP1_ENV_ROTATION_0  0
#define VDP1_ENV_ROTATION_90 1

#define VDP1_ENV_BPP_16      0
#define VDP1_ENV_BPP_8       1

#define VDP1_ENV_COLOR_MODE_PALETTE     0
#define VDP1_ENV_COLOR_MODE_RGB_PALETTE 1

struct vdp1_env {
        unsigned int :8;

        struct {
                unsigned int :1;
                unsigned int bpp:1;
                unsigned int rotation:1;
                unsigned int color_mode:1;
                unsigned int sprite_type:4;
        } __packed;

        color_rgb555_t erase_color;

        int16_vector2_t erase_points[2];
};

extern void vdp1_env_default_set(void);
extern void vdp1_env_set(const struct vdp1_env *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_VDP1_ENV_H_ */