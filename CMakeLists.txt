cmake_minimum_required(VERSION 3.29.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_FLAGS -Wall -Wextra -ffreestanding -fno-stack-protector -fno-stack-check -fno-lto -fno-PIC -ffunction-sections -fdata-sections -m64 -march=x86-64 -mabi=sysv -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -Wno-c99-designator)

project(myos CXX ASM_NASM)

set(SOURCES 
	./src/cpu/GDT.asm
	./src/cpu/interrupts.asm
	./src/main.cpp
	./src/memory/physical_memory.cpp
	./src/memory/virtual_memory.cpp
	./src/utils.cpp
	./src/limine/limine_requests.cpp
	./src/cpu/interrupts.cpp
	./src/cpu/GDT.cpp
	./src/panic.cpp
	./src/driver/console.cpp
	./src/driver/screen.cpp
	./src/driver/init.cpp
	./src/driver/acpi.cpp
	./src/driver/pic.cpp
	./src/cpu/asm.cpp
	./src/driver/keyboard/keyboard.cpp
	./src/cpu/spinlock.cpp
)

add_executable(myos ${SOURCES})

target_include_directories(myos PRIVATE include/)
target_include_directories(myos PRIVATE external/limine-protocol/include/)

target_compile_options(myos PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${COMMON_FLAGS}>)

# add custom command for a raw disk file with fully installed limine for better incremental builds 

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd
	COMMAND ${CMAKE_COMMAND} -E rm -f -- ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd
	COMMAND dd if=/dev/null bs=1M count=0 seek=512 of=${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd
	COMMAND sgdisk ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd -n 1:2048 -t 1:ef00 -m 1
	COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/external/Limine
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/limine bios-install 
		${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd
	COMMAND mformat -i ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd@@1M
	COMMAND mmd -i ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	COMMAND mcopy -i ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd@@1M 
		${CMAKE_CURRENT_SOURCE_DIR}/limine.conf 
		${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/limine-bios.sys 
		::/boot/limine
	COMMAND mcopy -i ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd@@1M
		${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/BOOTIA32.EFI
		::/EFI/BOOT
	COMMAND mcopy -i ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd@@1M
		${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/BOOTX64.EFI
		::/EFI/BOOT
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/limine.conf 
			${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/limine.c 
			${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/limine-bios.sys 
			${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/BOOTIA32.EFI
			${CMAKE_CURRENT_SOURCE_DIR}/external/Limine/BOOTX64.EFI
	COMMENT making limine_drive.hdd
)

# add custom command for make a raw disk with the os from a copy of the limine disk

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/drive.hdd
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd ${CMAKE_CURRENT_BINARY_DIR}/drive.hdd
	COMMAND mcopy -o -i drive.hdd@@1M ${CMAKE_CURRENT_BINARY_DIR}/myos ::/boot
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/limine_drive.hdd
			myos
	COMMENT making drive.hdd
)

# add target for making the raw disk file

add_custom_target(image ALL
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/drive.hdd
)
