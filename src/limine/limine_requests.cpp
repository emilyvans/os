#include "limine/limine_requests.hpp"
#include "limine.h"

volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0, .response = NULL};

volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST_ID, .revision = 0, .response = NULL};

volatile struct limine_smbios_request smbios_request = {
	.id = LIMINE_SMBIOS_REQUEST_ID, .revision = 0, .response = NULL};

volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST_ID, .revision = 0, .response = NULL};

volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0, .response = NULL};

volatile struct limine_executable_file_request executable_file_request = {
	.id = LIMINE_EXECUTABLE_FILE_REQUEST_ID, .revision = 0, .response = NULL};

volatile struct limine_executable_address_request executable_address_request = {
	.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
	.revision = 0,
	.response = NULL};
