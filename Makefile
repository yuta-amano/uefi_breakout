IMGS_DIR := imgs
DST_DIR := fs
PNGS := $(wildcard $(IMGS_DIR)/*.png)
BGRAS := $(patsubst $(IMGS_DIR)/%.png, $(DST_DIR)/%.bgra, $(PNGS))

all: fs/EFI/BOOT/BOOTX64.EFI $(BGRAS)

fs/EFI/BOOT/BOOTX64.EFI: file.c efi.c common.c graphics.c stage.c main.c
	mkdir -p fs/EFI/BOOT
	x86_64-w64-mingw32-gcc -Wall -Wextra -g -e efi_main -nostdinc -nostdlib -fno-builtin -Wl,--subsystem,10 -o $@ $+

fs/%.bgra: $(IMGS_DIR)/%.png | $(DST_DIR)
	convert $< -depth 8 $@

$(DST_DIR):
	mkdir -p $(DST_DIR)

run: fs/EFI/BOOT/BOOTX64.EFI
	qemu-system-x86_64 -bios OVMF.fd -drive file=fat:rw:fs,format=raw,media=disk

debug: fs/EFI/BOOT/BOOTX64.EFI
	qemu-system-x86_64 -bios OVMF.fd -drive file=fat:rw:fs,format=raw,media=disk -s -S

clean:
	rm -rf *~ fs

.PHONY: clean
