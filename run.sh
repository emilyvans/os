#!/bin/env bash
qemu-system-x86_64 \
	-enable-kvm \
	-machine q35 \
	-display gtk,gl=on,zoom-to-fit=off \
	-smp 1 \
	-cpu host,+smep,+smap,+pcid,+fsgsbase \
	-m 4096 \
	-monitor telnet:127.0.0.1:7777,server,nowait \
	-serial mon:stdio \
	-drive format=raw,file=drive.hdd,if=none,id=disk0 \
	-device virtio-blk-pci,drive=disk0 \
	-device virtio-sound \
	-device VGA \
	-device virtio-gpu-pci \
	-boot order=c \
	-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/x64/OVMF_CODE.4m.fd \
	-drive if=pflash,format=raw,file=OVMF_VARS.4m.fd \
	$@
