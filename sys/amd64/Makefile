# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = boot.c \
	  context.c \
	  copy.S \
	  evec.S \
	  interrupt.c \
	  pmap.c \
	  rootdev.c \
	  signal.c \
	  start.S \
	  switch.S \
	  thread.c

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

DISK_IMAGE := $(TOPDIR)/disk.img

SECTOR_SIZE := 512

# 550 MiB is the recommended ESP size.
ESP_SIZE := 576716800

# LBA of the first sector.
ESP_START := 2048

# Number of sectors composing ESP.
ESP_SECTOR_COUNT := $(shell echo '$(ESP_SIZE) / $(SECTOR_SIZE)' | bc)

# LBA of the last sector.
ESP_END := $(shell echo '$(ESP_START) + $(ESP_SIZE) / $(SECTOR_SIZE) - 1' | bc)

# ESP image.
ESP_IMAGE := $(CURDIR)/esp.img

# Kernel image.
KERNEL_IMAGE := $(TOPDIR)/sys/mimiker.elf

# Initial RAM disk.
INITRD := $(TOPDIR)/initrd.cpio

# Bootloader UEFI application.
UEFI_APP := $(CURDIR)/BOOTX64.EFI

# Bootloader configuration.
CONFIG := $(CURDIR)/grub.cfg

# Startup UEFI shell script.
STARTUP := $(CURDIR)/startup.nsh

disk-init:
	@echo "[DISK] Create ESP"
	sgdisk -z $(DISK_IMAGE)
	sgdisk -n 1:$(ESP_START):$(ESP_END) \
	       -t 1:ef00 \
	       $(DISK_IMAGE)

$(UEFI_APP): $(CONFIG)
	@echo "[DISK] Creata a standalone GRUB binary"
	grub-mkstandalone -O x86_64-efi -o $(UEFI_APP) \
			  "boot/grub/grub.cfg=$(CONFIG)"

disk-boot-image: $(UEFI_APP)
	@echo "[DISK] Create an ESP image"
	dd if=/dev/zero of=$(ESP_IMAGE) bs=$(SECTOR_SIZE) \
	   count=$(ESP_SECTOR_COUNT)

	@echo "[DISK] Format the partition with FAT32"
	mkdosfs -F 32 $(ESP_IMAGE)

	@echo "[DISK] Copy kernel image and initrd to the partition"
	mmd -i $(ESP_IMAGE) ::/mimiker
	mcopy -i $(ESP_IMAGE) $(KERNEL_IMAGE) ::/mimiker
	mcopy -i $(ESP_IMAGE) $(INITRD) ::/mimiker

	@echo "[DISK] Place the bootloader at the default UEFI " \
	      "application load path"
	mmd -i $(ESP_IMAGE) ::/EFI
	mmd -i $(ESP_IMAGE) ::/EFI/BOOT
	mcopy -i $(ESP_IMAGE) $(UEFI_APP) ::/EFI/BOOT

	@echo "[DISK] Supply a default UEFI shell script"
	mcopy -i $(ESP_IMAGE) $(STARTUP) ::

disk: disk-init disk-boot-image
	@echo "[DISK] Save the ESP image on the disk"
	dd if=$(ESP_IMAGE) of=$(DISK_IMAGE) bs=$(SECTOR_SIZE) \
	   count=$(ESP_SECTOR_COUNT) seek=$(ESP_START) conv=notrunc

PHONY-TARGETS += disk-init disk-boot-image disk

clean-here:
	$(RM) -r $(ESP_IMAGE)

distclean-here:
	$(RM) -r $(UEFI_APP)
