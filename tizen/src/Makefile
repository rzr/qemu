EMUL_DIR=../Emulator

qemu:
	cd ../../ && $(MAKE)
skin_client:
	ant -buildfile skin/client/build.xml
all: qemu skin_client
clean:
	cd ../../ && $(MAKE) clean
distclean:
	cd ../../ && $(MAKE) distclean
install: all
	mkdir -p $(EMUL_DIR)/bin
	mkdir -p $(EMUL_DIR)/etc
	mkdir -p $(EMUL_DIR)/x86
	mkdir -p $(EMUL_DIR)/arm
	cp ../../i386-softmmu/qemu-system-i386 $(EMUL_DIR)/bin/emulator-x86
	cp skin/client/emulator-skin.jar $(EMUL_DIR)/bin
	cp ../../qemu-img $(EMUL_DIR)/bin
	cp -dpr skin/client/skins $(EMUL_DIR)
	cp -dpr ../data $(EMUL_DIR)/x86
	mkdir -p $(EMUL_DIR)/x86/data/pc-bios
	cp -dpr ../../pc-bios/bios.bin $(EMUL_DIR)/x86/data/pc-bios
	cp -dpr ../../pc-bios/linuxboot.bin $(EMUL_DIR)/x86/data/pc-bios
	cp -dpr ../../pc-bios/pxe-rtl8139.rom $(EMUL_DIR)/x86/data/pc-bios
	cp -dpr ../../pc-bios/pxe-virtio.rom $(EMUL_DIR)/x86/data/pc-bios
	cp -dpr ../../pc-bios/vgabios-maruvga.bin $(EMUL_DIR)/x86/data/pc-bios
	cp -dpr ../license $(EMUL_DIR)
