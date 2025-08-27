set(CMAKE_SYSTEM_NAME Generic)

set(triple x86_64-unknown-none-elf)

set(COMMON_FLAGS "-Wall -Wextra -ffreestanding -fno-stack-protector -fno-stack-check -fno-lto -fno-PIC -ffunction-sections -fdata-sections -m64 -march=x86-64 -mabi=sysv -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel")

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS}")

add_link_options(
    -nostdlib
    -static
    -z max-page-size=0x1000
	-T ${CMAKE_CURRENT_LIST_DIR}/linker.ld
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
