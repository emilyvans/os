set(CMAKE_SYSTEM_NAME Generic)

set(triple x86_64-unknown-none-elf)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})

add_link_options(
    -nostdlib
    -static
    -z max-page-size=0x1000
	-T ${CMAKE_CURRENT_LIST_DIR}/linker.ld
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
