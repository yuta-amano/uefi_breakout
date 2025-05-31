#include "common.h"
#include "efi.h"
#include "graphics.h"
#include "stage.h"

#define TITLE_WIDTH 600
#define TITLE_HEIGHT 450
#define TITLE_IMAGE_BUF_SIZE 1080000 // 600px x 450px x 4(bgra)

#define STATE_TITLE 1
#define STATE_TITLE_WAIT 2
#define STATE_STAGE 3

unsigned char title_image[TITLE_IMAGE_BUF_SIZE];
unsigned short title_image_name[] = L"title.bgra";

unsigned int main_state = STATE_TITLE;

void load_title_image();

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
	efi_init(SystemTable);

	load_title_image();

	while(1) {
		switch (main_state) {
			case STATE_TITLE:
				struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL background = { 0, 0, 0, 0 };
  			clear(background);
				blt(title_image, TITLE_WIDTH, TITLE_HEIGHT, (GOP->Mode->Info->HorizontalResolution - TITLE_WIDTH) / 2, (GOP->Mode->Info->VerticalResolution - TITLE_HEIGHT) / 2);
				draw_frame_buffer();
				main_state = STATE_TITLE_WAIT;
				break;
			case STATE_TITLE_WAIT:
				if (getc_no_wait()) {
					init_stage();
					main_state = STATE_STAGE;
				}
				break;
			case STATE_STAGE:
				if (update_stage()) {
					main_state = STATE_TITLE;
				}
				break;
		}
	}
}

void load_title_image()
{
	load_image(TITLE_WIDTH, TITLE_HEIGHT, title_image_name, title_image);
}
