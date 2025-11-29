#!/bin/env bash
qemu-system-x86_64 \
	-enable-kvm \
	-display gtk,gl=on,zoom-to-fit=off \
	-smp 1 \
	-cpu host,+smep,+smap,+pcid,+fsgsbase \
	-m 4096 \
	-monitor telnet:127.0.0.1:7777,server,nowait \
	-serial mon:stdio \
	-drive format=raw,file=drive.hdd \
	-device virtio-sound \
	-device virtio-gpu-pci \
	-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/x64/OVMF_CODE.4m.fd \
	$@
