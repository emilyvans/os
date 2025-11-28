cmake_minimum_required(VERSION 3.29.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_FLAGS -Wall -Wextra -ffreestanding -fno-stack-protector -fno-stack-check -fno-lto -fno-PIC -ffunction-sections -fdata-sections -m64 -march=x86-64 -mabi=sysv -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -Wno-c99-designator)

project(myos CXX ASM_NASM)

set(SOURCES 
	./src/GDT.asm
	./src/interrupts.asm
	./src/main.cpp
	./src/physical_memory.cpp
	./src/virtual_memory.cpp
	./src/utils.cpp
	./src/limine_requests.cpp
	./src/interrupts.cpp
	./src/GDT.cpp
	./src/panic.cpp
	./src/console.cpp
	./src/screen.cpp
	./src/init.cpp
	./src/acpi.cpp
	./src/pic.cpp
	./src/asm.cpp
	./src/keyboard.cpp
	./src/spinlock.cpp
)
#set_property(SOURCE ./src/GDT.asm APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
include_directories(./include/)

add_executable(myos ${SOURCES})

target_compile_options(myos PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${COMMON_FLAGS}>)
