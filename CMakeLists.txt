cmake_minimum_required(VERSION 3.29.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#$(CMAKE_CURRENT_LIST_DIR)

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
)
#set_property(SOURCE ./src/GDT.asm APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

add_executable(myos ${SOURCES})
