#include "efi.h"
#include "graphics.h"
#include "common.h"

unsigned char back_buffer[14745600]; // メモリアロケーションをしたくないので、とりあえずでかい（2560×1440x4）を割り当て

void clear(struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color)
{
  unsigned int width = GOP->Mode->Info->HorizontalResolution;
  unsigned int height = GOP->Mode->Info->VerticalResolution;
  unsigned int buffer_size = width * height;
  unsigned char *frame_buffer = back_buffer;
  for (unsigned int i = 0; i < buffer_size; i++) {
    *frame_buffer++ = color.Blue;
    *frame_buffer++ = color.Green;
    *frame_buffer++ = color.Red;
    *frame_buffer++ = color.Reserved;
  }
}

void blt(unsigned char img[], unsigned int img_width, unsigned int img_height, unsigned int x, unsigned int y)
{
	unsigned char *fb;
	unsigned int i, j, k, vr, hr, ofs = 0;

	fb = back_buffer;
	vr = GOP->Mode->Info->VerticalResolution;
	hr = GOP->Mode->Info->HorizontalResolution;

	for (i = 0; i < y + img_height; i++) {
    if (i <= y) {
      // 画面幅分のbgra分スキップ
      fb += hr * 4;
      continue;
    }
		for (j = 0; j < hr; j++) {
      if (j <= x) {
        // bgra分スキップ
        fb += 4;
        continue;
      }
      if (j > x + img_width) {
        // 残りの幅のバッファを飛ばして次の行へ
        fb += (hr - (x + img_width) - 1) * 4;
        break;
      }
			for (k = 0; k < 4; k++)
				*fb++ = img[ofs++];
		}
	}
}

void draw_frame_buffer(void)
{
  unsigned int width = GOP->Mode->Info->HorizontalResolution;
  unsigned int height = GOP->Mode->Info->VerticalResolution;
  unsigned int buffer_size = width * height * 4;
  unsigned char *back_buffer_ptr = back_buffer;
  unsigned char *frame_buffer = (unsigned char *)GOP->Mode->FrameBufferBase;
  for (unsigned int i = 0; i < buffer_size; i++) {
    *frame_buffer++ = *back_buffer_ptr++;
  }
}

void load_image(unsigned int width, unsigned int height, unsigned short image_name[], unsigned char *image)
{
  struct EFI_FILE_PROTOCOL *root;
  struct EFI_FILE_PROTOCOL *file;
  unsigned long long status;
  unsigned long long image_buf_size = width * height * 4;
  status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"error: SFSP->OpenVolume");
  status = root->Open(root, &file, image_name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
  assert(status, L"error: root->Open");
  status = file->Read(file, &image_buf_size, (void *)image);
  assert(status, L"error: file->Read");
  status = file->Close(file);
	status = root->Close(root);
}
