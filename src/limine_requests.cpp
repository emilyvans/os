#include "limine_requests.hpp"

volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0, .response = NULL};

volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST, .revision = 0, .response = NULL};

volatile struct limine_smbios_request smbios_request = {
	.id = LIMINE_SMBIOS_REQUEST, .revision = 0, .response = NULL};

volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST, .revision = 0, .response = NULL};

volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST, .revision = 0, .response = NULL};

volatile struct limine_executable_file_request executable_file_request = {
	.id = LIMINE_EXECUTABLE_FILE_REQUEST, .revision = 0, .response = NULL};

volatile struct limine_executable_address_request executable_address_request = {
	.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 0, .response = NULL};
