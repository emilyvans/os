#ifndef INCLUDE_LIMINE_LIMINE_REQUESTS_HPP_
#define INCLUDE_LIMINE_LIMINE_REQUESTS_HPP_
#include <limine.h>
#include <stddef.h>

extern __attribute__((
	section(".limine_requests"))) volatile struct limine_framebuffer_request
	framebuffer_request;

extern __attribute__((section(
	".limine_requests"))) volatile struct limine_rsdp_request rsdp_request;

extern __attribute__((section(
	".limine_requests"))) volatile struct limine_smbios_request smbios_request;

extern __attribute__((section(
	".limine_requests"))) volatile struct limine_hhdm_request hhdm_request;

extern __attribute__((section(
	".limine_requests"))) volatile struct limine_memmap_request memmap_request;

extern __attribute__((
	section(".limine_requests"))) volatile struct limine_executable_file_request
	executable_file_request;

extern __attribute__((section(
	".limine_requests"))) volatile struct limine_executable_address_request
	executable_address_request;
#endif // INCLUDE_LIMINE_LIMINE_REQUESTS_HPP_
