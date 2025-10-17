#include "acpi.hpp"
#include "console.hpp"
#include "limine_requests.hpp"
#include "panic.hpp"
#include "physical_memory.hpp"
#include "screen.hpp"
#include "utils.hpp"
#include <stdint.h>

void init_ACPI() {
	uint64_t memory_offset = hhdm_request.response->offset;

	RSDP *rsdp = (RSDP *)(rsdp_request.response->address + memory_offset);

	if (rsdp->Revision != 2) {
		printf("rsdp revision must be 2\n");
		hcf();
	}

	XSDP *xsdp = (XSDP *)(rsdp);

	XSDT *root_xsdt = (XSDT *)(xsdp->XsdtAddress + memory_offset);

	uint64_t entries =
		(root_xsdt->header.Length - sizeof(root_xsdt->header)) / 8;
	for (uint64_t i = 0; i < entries; i++) {
		ACPISDTHeader *header =
			(ACPISDTHeader *)(root_xsdt->pointer_to_other_sdt[i] +
		                      memory_offset);

		const char signature[5] = {0, 0, 0, 0, 0};
		memcpy((void *)signature, (void *)header->Signature, 4);
		// FACP APIC HPET WAET(could ignore) BGRT
		if (memcmp(header->Signature, "FACP", 4) == 0) {
			printf("%s %u %u\n", signature, header->Revision, header->Length);
		} else if (memcmp(header->Signature, "APIC", 4) == 0) {
			printf("%s %u %u\n", signature, header->Revision, header->Length);
			MADT *madt = (MADT *)header;
			(void)madt;
			uint64_t interrupt_controler_array_start =
				(uint64_t)header + sizeof(MADT);
			uint64_t max_address = (uint64_t)header + header->Length;
			uint64_t interrupt_controler_array_entry =
				interrupt_controler_array_start;
			while (interrupt_controler_array_entry < max_address) {
				InteruptController *controller =
					(InteruptController *)interrupt_controler_array_entry;
				printf("interupt type: %x, size: %u\n", controller->type,
				       controller->size);
				interrupt_controler_array_entry += controller->size;
			}
		} else if (memcmp(header->Signature, "HPET", 4) == 0) {
			printf("%s %u %u\n", signature, header->Revision, header->Length);
		} else if (memcmp(header->Signature, "BGRT", 4) == 0) {
			BGRT *bgrt = (BGRT *)header;
			display_BGRT(bgrt);
		} else if (memcmp(header->Signature, "WAET", 4) == 0) {
		} else {
			printf("%s %u %u\n", signature, header->Revision, header->Length);
		}
	}
}

void display_BGRT(BGRT *bgrt) {
	BitmapHeader *bitmap_ptr =
		(BitmapHeader *)(bgrt->image_address + hhdm_request.response->offset);
	/*printf("x offset: %u\ny offset: %u\ntype: %u\nstatus: %b\naddress: "
	       "%x\n",
	       bgrt->image_x_offset, bgrt->image_y_offset, bgrt->image_type,
	       bgrt->status, (uint64_t)bitmap_ptr);

	printf("%c%c\n", bitmap_ptr->magic[0], bitmap_ptr->magic[1]);
	printf("DIB size: %u\n", bitmap_ptr->info_header.size);*/

	if (bitmap_ptr->info_header.size != 40) {
		printf("Only bitmaps with DIB size of 40 is supported!\n");
		return;
	}

	/*printf("width: %u\nheight: %u\nbpp: %u\ncompress: %u\ncolors: %u\naddress:
	   "
	       "%x\n size: %uBytes\n img addr: %x\n",
	       bitmap_ptr->info_header.width, bitmap_ptr->info_header.height,
	       bitmap_ptr->info_header.bit_per_pixel,
	       bitmap_ptr->info_header.compression_method,
	       bitmap_ptr->info_header.color_count_in_color_palette,
	       (uint64_t)bitmap_ptr, bitmap_ptr->file_size,
	       bitmap_ptr->pixel_byte_offset);*/

	if (bitmap_ptr->info_header.bit_per_pixel != 24) {
		printf("bitmap support is only for 24bits per pixel for now!\n");
		return;
	}

	uint8_t *pixels =
		(uint8_t *)(bgrt->image_address + bitmap_ptr->pixel_byte_offset +
	                hhdm_request.response->offset);

	uint64_t row_size = ((bitmap_ptr->info_header.bit_per_pixel *
	                          bitmap_ptr->info_header.width +
	                      31) /
	                     32) *
	                    4;

	BitmapDIB info_header = bitmap_ptr->info_header;

	uint64_t image_page_count = ALIGN_UP(
		info_header.width * info_header.height * sizeof(uint32_t), 4096);
	void *physical_address = physicalmemory::kalloc(image_page_count);
	uint32_t *buffer = (uint32_t *)((uint64_t)physical_address +
	                                hhdm_request.response->offset);

	for (int32_t y = 0; y < info_header.height; y++) {
		for (int32_t x = 0; x < info_header.width; x++) {
			uint32_t index = y * info_header.width + x;
			uint64_t pixel_index =
				row_size * (info_header.height - y - 1) + (x * 3);
			uint8_t r = pixels[pixel_index + 2];
			uint8_t g = pixels[pixel_index + 1];
			uint8_t b = pixels[pixel_index + 0];
			buffer[index] = r << 16 | g << 8 | b << 0;
		}
	}

	image_blit(buffer, bgrt->image_x_offset, bgrt->image_y_offset,
	           info_header.width, info_header.height);

	physicalmemory::kfree(physical_address, image_page_count);
}
