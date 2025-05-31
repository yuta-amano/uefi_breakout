#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "efi.h"

struct RECT {
	unsigned int x, y;
	unsigned int w, h;
};

void clear(struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
void blt(unsigned char img[], unsigned int img_width, unsigned int img_height, unsigned int x, unsigned int y);
void draw_frame_buffer(void);
void load_image(unsigned int width, unsigned int height, unsigned short image_name[], unsigned char *image);

#endif
