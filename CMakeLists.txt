cmake_minimum_required(VERSION 3.29.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#$(CMAKE_CURRENT_LIST_DIR)

project(myos CXX ASM_NASM)

set(SOURCES 
	./src/GDT.asm
	./src/interupts.asm
	./src/main.cpp
	./src/physicalmem.cpp
	./src/virtualmem.cpp
	./src/utils.cpp
	./src/limine_requests.cpp
	./src/interupts.cpp
	./src/GDT.cpp
)
#set_property(SOURCE ./src/GDT.asm APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

add_executable(myos ${SOURCES})
