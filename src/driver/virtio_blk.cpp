#include "driver/virtio_blk.hpp"
#include "driver/console.hpp"
#include "driver/pci.hpp"
#include "limine/limine_requests.hpp"
#include "memory/virtual_memory.hpp"
#include "panic.hpp"
#include "utils.hpp"
#include <stdint.h>

void print_pci_device(volatile pci_header *pci_device, uint8_t function = 0) {
	if (pci_device->header_type & 0x80) {
		printf("-----------------------------------%u\n", (uint64_t)function);
	} else {
		printf("------------------------------------\n");
	}
	printf("vendor: 0x%x\ndevice: 0x%x\ncommand: 0x%x\nstatus: "
	       "%b\nrevision_id: 0x%x\nprog_if: 0x%x\nclass: 0x%x\n"
	       "subclass: 0x%x\ncacheline_size: 0x%x\nlatency_timer: "
	       "0x%x\ntype: 0x%x\nBIST: 0x%x\n",
	       (uint64_t)pci_device->Vendor_ID, (uint64_t)pci_device->device_ID,
	       (uint64_t)pci_device->command, (uint64_t)pci_device->status,
	       (uint64_t)pci_device->revision_ID, (uint64_t)pci_device->prog_IF,
	       (uint64_t)pci_device->class_code, (uint64_t)pci_device->subclass,
	       (uint64_t)pci_device->cache_line_size,
	       (uint64_t)pci_device->latency_timer,
	       (uint64_t)pci_device->header_type, (uint64_t)pci_device->BIST);

	if ((pci_device->header_type & 0x7F) == 0x0) {
		printf("bar0: 0x%x\nbar1: 0x%x\nbar2: 0x%x\nbar3: 0x%x\nbar4: "
		       "0x%x\nbar5: 0x%x\ncardbus_CIS_pointer: 0x%x\n"
		       "subsystem_vendor: 0x%x\nsubsystem: 0x%x\n"
		       "expansion_rom: 0x%x\ncapabilities_pointer: 0x%x\n"
		       "interrupt_line: 0x%x\ninterrupt_pin: 0x%x\nmin_grant: "
		       "0x%x\nmax_latency: 0x%x\n",
		       (uint64_t)pci_device->type_0.BAR0,
		       (uint64_t)pci_device->type_0.BAR1,
		       (uint64_t)pci_device->type_0.BAR2,
		       (uint64_t)pci_device->type_0.BAR3,
		       (uint64_t)pci_device->type_0.BAR4,
		       (uint64_t)pci_device->type_0.BAR5,
		       (uint64_t)pci_device->type_0.Cardbus_CIS_pointer,
		       (uint64_t)pci_device->type_0.subsystem_vendor_id,
		       (uint64_t)pci_device->type_0.subsystem_id,
		       (uint64_t)pci_device->type_0.expansion_ROM_base_address,
		       (uint64_t)pci_device->type_0.capabilities_pointer,
		       (uint64_t)pci_device->type_0.interrupt_line,
		       (uint64_t)pci_device->type_0.interrupt_pin,
		       (uint64_t)pci_device->type_0.min_grant,
		       (uint64_t)pci_device->type_0.max_latency);
	}
}

PCIDeviceID id_table[] = {{.vendor_id = 0x1AF4, .device_id = 0x1001}, {0}};

inline bool is_feature_available(volatile VirtioPciCommonCfg *cfg, int bit) {
	cfg->device_feature_select = bit / 32;
	uint32_t bit_mask = 1u << (bit % 32);
	return (cfg->device_feature & bit_mask) != 0;
}

inline void set_feature(volatile VirtioPciCommonCfg *cfg, int bit) {
	cfg->driver_feature_select = bit / 32;
	uint32_t bit_mask = 1u << (bit % 32);
	cfg->driver_feature |= bit_mask;
}

void print_features(VirtioPciCommonCfg *cfg) {
	if (is_feature_available(cfg, 0)) {
		printf("BARRIER ");
	}
	if (is_feature_available(cfg, 1)) {
		printf("SIZE_MAX ");
	}
	if (is_feature_available(cfg, 2)) {
		printf("SEG_MAX ");
	}
	if (is_feature_available(cfg, 4)) {
		printf("GEOMETRY ");
	}
	if (is_feature_available(cfg, 5)) {
		printf("RO ");
	}
	if (is_feature_available(cfg, 6)) {
		printf("BLK_SIZE ");
	}
	if (is_feature_available(cfg, 7)) {
		printf("SCSI ");
	}
	if (is_feature_available(cfg, 9)) {
		printf("FLUSH ");
	}
	if (is_feature_available(cfg, 10)) {
		printf("TOPOLOGY ");
	}
	if (is_feature_available(cfg, 11)) {
		printf("CONFIG_WCE ");
	}
	if (is_feature_available(cfg, 12)) {
		printf("MQ ");
	}
	if (is_feature_available(cfg, 13)) {
		printf("DISCARD ");
	}
	if (is_feature_available(cfg, 14)) {
		printf("WRITE_ZEROES ");
	}
	if (is_feature_available(cfg, 15)) {
		printf("LIFETIME ");
	}
	if (is_feature_available(cfg, 16)) {
		printf("SECURE_ERASE ");
	}
	if (is_feature_available(cfg, 28)) {
		printf("INDIRECT_DESC ");
	}
	if (is_feature_available(cfg, 29)) {
		printf("EVENT_IDX ");
	}
	if (is_feature_available(cfg, 32)) {
		printf("VERSION_1 ");
	}
	if (is_feature_available(cfg, 33)) {
		printf("ACCESS_PLATFORM");
	}
	if (is_feature_available(cfg, 34)) {
		printf("RING_PACKED ");
	}
	if (is_feature_available(cfg, 35)) {
		printf("IN_ORDER");
	}
	if (is_feature_available(cfg, 36)) {
		printf("ORDER_PLATFORM ");
	}
	if (is_feature_available(cfg, 37)) {
		printf("SR_IOV ");
	}
	if (is_feature_available(cfg, 38)) {
		printf("NOTIFICATION_DATA ");
	}
	if (is_feature_available(cfg, 39)) {
		printf("NOTIF_CONFIG_DATA");
	}
	if (is_feature_available(cfg, 40)) {
		printf("RING_RESET ");
	}
	if (is_feature_available(cfg, 24)) {
		printf("NOTIFY_ON_EMPTY ");
	}
	if (is_feature_available(cfg, 27)) {
		printf("ANY_LAYOUT ");
	}
	if (is_feature_available(cfg, 30)) {
		printf("UNUSED QEMU");
	}
}

typedef struct VirtQueueDescriptor_s {
	uint64_t address;
	uint32_t length;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed)) VirtQueueDescriptor;

typedef struct VirtQueueAvailable_s {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[];
} __attribute__((packed)) VirtQueueAvailable;

typedef struct VirtQueueUsedElement_s {
	uint32_t id;
	uint32_t length;
} __attribute__((packed)) VirtQueueUsedElement;

typedef struct VirtQueueUsed_s {
	uint16_t flags;
	uint16_t idx;
	VirtQueueUsedElement ring[];
} __attribute__((packed)) VirtQueueUsed;

typedef struct VirtQueue_s {
	uint16_t number;
	uint16_t size;
	uint64_t notify_offset;
	VirtQueueDescriptor *descriptor_table;
	VirtQueueAvailable *available_ring;
	VirtQueueUsed *used_ring;
} VirtQueue;

typedef struct VirtioBlkReq_s {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
	uint8_t data[];
} __attribute__((packed)) VirtioBlkReq;

VirtQueue *create_virtqueue(volatile VirtioPciCommonCfg *cfg, uint32_t index) {
	cfg->queue_select = index;
	uint64_t descriptor_table_size = 16 * cfg->queue_size;
	uint64_t available_ring_size = 6 + (2 * cfg->queue_size);
	uint64_t used_ring_size = 6 + (8 * cfg->queue_size);
	uint64_t total_size =
		descriptor_table_size + available_ring_size + used_ring_size;
	VirtQueue *virt_queue =
		(VirtQueue *)page_alloc(DIV_ROUNDUP(total_size, 4096));
	if (virt_queue == nullptr) {
		return nullptr;
	}
	memset(virt_queue, 0, ALIGN_UP(total_size, 4096));

	virt_queue->size = cfg->queue_size;
	virt_queue->number = index;

	virt_queue->descriptor_table = (VirtQueueDescriptor *)ALIGN_UP(
		(uint64_t)virt_queue + sizeof(VirtQueue), 16);
	virt_queue->available_ring = (VirtQueueAvailable *)ALIGN_UP(
		(uint64_t)virt_queue->descriptor_table + descriptor_table_size, 2);
	virt_queue->used_ring = (VirtQueueUsed *)ALIGN_UP(
		(uint64_t)virt_queue->available_ring + available_ring_size, 4);
	virt_queue->notify_offset = cfg->queue_notify_off;

	cfg->queue_desc =
		(uint64_t)virt_queue->descriptor_table - hhdm_request.response->offset;
	cfg->queue_driver =
		(uint64_t)virt_queue->available_ring - hhdm_request.response->offset;
	cfg->queue_device =
		(uint64_t)virt_queue->used_ring - hhdm_request.response->offset;

	cfg->queue_enable = 1;
	__sync_synchronize();

	printf("sel: %u, size: %u, msix: %u, enable: %u, notify offset: %u, desc: "
	       "0x%x, driver: 0x%x, device: 0x%x, desc_size: %u, avail_size: %u, "
	       "used_size: %u, status: %b, fail: %b \n",
	       (uint16_t)cfg->queue_select, (uint16_t)cfg->queue_size,
	       (uint16_t)cfg->queue_msix_vector, (uint16_t)cfg->queue_enable,
	       (uint16_t)cfg->queue_notify_off, (uint64_t)cfg->queue_desc,
	       (uint64_t)cfg->queue_driver, (uint64_t)cfg->queue_device,
	       descriptor_table_size, available_ring_size, used_ring_size,
	       (uint64_t)(uint8_t)cfg->device_status, 128);
	return virt_queue;
}

int virtio_blk_probe(PCIDevice *device) {
	pci_header *virtio_block_device = (pci_header *)device->config_address;
	print_pci_device(virtio_block_device);
	pci_capability *capability =
		(pci_capability *)((uint64_t)virtio_block_device +
	                       (virtio_block_device->type_0.capabilities_pointer &
	                        ~0x3));
	// 64-bit bar 4: bar5[63:32] bar4[31:0]
	bool mapped_bar[] = {false, false, false, false, false};
	VirtioPciCommonCfg *common_config;
	VirtioBlkConfig *device_config;
	VirtioPciNotifyCfg *notify_config;
	uint8_t *isr_config;
	virtio_block_device->status;
	printf("------------------------------------\n");
	while (capability != nullptr) {
		printf("address: 0x%x\n", capability);
		if (capability->vendor == 0x9) {
			VirtioPciCapability *virtio_capability =
				(VirtioPciCapability *)capability;

			BarAddress addr = get_address_from_bar(virtio_block_device,
			                                       virtio_capability->bar);

			printf("addr: 0x%x, type: %s, size: 0x%x\n",
			       addr.address + virtio_capability->offset,
			       addr.is_memory_space ? "memory" : "I/O", addr.size);

			if (addr.is_memory_space) {
				printf(
					"vendor: 0x%x\nnext: 0x%x\nlength: 0x%x\nconfig_type: "
					"0x%x\nbar: 0x%x\nid: 0x%x\noffset: 0x%x\nstruct_length: "
					"0x%x\n",
					(uint64_t)virtio_capability->capability.vendor,
					(uint64_t)virtio_capability->capability.next,
					(uint64_t)virtio_capability->length,
					(uint64_t)virtio_capability->config_type,
					(uint64_t)virtio_capability->bar,
					(uint64_t)virtio_capability->id,
					(uint64_t)virtio_capability->offset,
					(uint64_t)virtio_capability->struct_length);

				if (!mapped_bar[virtio_capability->bar]) {
					for (uint64_t offset = 0; offset < addr.size;
					     offset += 4096) {
						uint64_t phys = addr.address + offset;
						uint64_t virt = phys + hhdm_request.response->offset;

						virtualmemory::map_kernel_page(
							virt, phys,
							virtualmemory::present_flag |
								virtualmemory::Cache_disable_flag |
								virtualmemory::readwrite_flag);
					}
					mapped_bar[virtio_capability->bar] = true;
				}

				if (virtio_capability->config_type ==
				    VIRTIO_PCI_CAP_DEVICE_CFG) {
					device_config =
						(VirtioBlkConfig *)(addr.address +
					                        hhdm_request.response->offset +
					                        virtio_capability->offset);

				} else if (virtio_capability->config_type ==
				           VIRTIO_PCI_CAP_COMMON_CFG) {
					common_config =
						(VirtioPciCommonCfg *)(addr.address +
					                           hhdm_request.response->offset +
					                           virtio_capability->offset);
				} else if (virtio_capability->config_type ==
				           VIRTIO_PCI_CAP_NOTIFY_CFG) {
					notify_config = (VirtioPciNotifyCfg *)capability;
				} else if (virtio_capability->config_type ==
				           VIRTIO_PCI_CAP_ISR_CFG) {
					isr_config =
						(uint8_t *)(addr.address + virtio_capability->offset +
					                hhdm_request.response->offset);
				}
			}
		} else {
			printf("vendor: %x next: %x\n", (uint64_t)capability->vendor,
			       (uint64_t)capability->next);
		}
		printf("------------------------------------\n");
		if (capability->next == 0)
			break;

		capability = (pci_capability *)((uint64_t)virtio_block_device +
		                                (capability->next & ~0x3));
	}

#define DEVICE_RESET 0
#define DEVICE_ACKNOWLEDGE 1
#define DEVICE_DRIVER 2
#define DEVICE_DRIVER_OK 4
#define DEVICE_FEATURES_OK 8
#define DEVICE_SUSPEND 16
#define DEVICE_DEVICE_NEEDS_RESET 64
#define DEVICE_FAILED 128

	common_config->device_status = DEVICE_RESET;
	common_config->device_status |= DEVICE_ACKNOWLEDGE;
	common_config->device_status |= DEVICE_DRIVER;

	common_config->driver_feature_select = 0;
	common_config->device_feature_select = 0;
	printf("dev_feaures_select: %b\n",
	       (uint64_t)common_config->device_feature_select);
	printf("dev_feaures: %b\n", (uint16_t)common_config->device_feature);
	printf("drv_feaures_select: %b\n",
	       (uint64_t)common_config->driver_feature_select);
	printf("drv_feaures: %b\n", (uint16_t)common_config->driver_feature);
	print_features(common_config);
	// SEG_MAX(2) GEOMETRY(4) BLK_SIZE(6) EVENT_IDX(29) VERSION_1(32)

	if (is_feature_available(common_config, 2)) {
		set_feature(common_config, 2);
	}
	if (is_feature_available(common_config, 4)) {
		set_feature(common_config, 4);
	}
	if (is_feature_available(common_config, 6)) {
		set_feature(common_config, 6);
	}
	if (is_feature_available(common_config, 29)) {
		set_feature(common_config, 29);
	}
	if (is_feature_available(common_config, 32)) {
		set_feature(common_config, 32);
	}
	common_config->device_status |= DEVICE_FEATURES_OK;

	common_config->driver_feature_select = 0;
	common_config->device_feature_select = 0;
	printf("dev_feaures_select: %b\n",
	       (uint64_t)common_config->device_feature_select);
	printf("dev_feaures: %b\n", (uint16_t)common_config->device_feature);
	printf("drv_feaures_select: %b\n",
	       (uint64_t)common_config->driver_feature_select);
	printf("drv_feaures: %b\n", (uint16_t)common_config->driver_feature);
	print_features(common_config);

	if (common_config->device_status & DEVICE_FAILED) {
		printf("failed\n");
		hcf();
	}

	printf("size: %uMiB\ncylinders: %u\nheads: %u\nsectors: %u\nblk_size: "
	       "%uB\nqueues: %u\n",
	       (device_config->capacity * 512) / 1024 / 1024,
	       device_config->geometry.cylinders, device_config->geometry.heads,
	       device_config->geometry.sectors, device_config->blk_size,
	       device_config->num_queues);

	VirtQueue *virt_queue = create_virtqueue(common_config, 0);
	common_config->device_status |= 4;
	// FIXME: remove the code for real use
	uint64_t request_size = sizeof(VirtioBlkReq);
	VirtioBlkReq *data = (VirtioBlkReq *)kzalloc(request_size + 513);
	data->sector = 0;
	uint64_t base_phys = (uint64_t)data - hhdm_request.response->offset;
	virt_queue->descriptor_table->address = base_phys;
	virt_queue->descriptor_table->length = request_size;
	virt_queue->descriptor_table->flags = 1;
	virt_queue->descriptor_table->next = 1;

	virt_queue->descriptor_table[1].address = base_phys + sizeof(VirtioBlkReq);
	virt_queue->descriptor_table[1].length = 512;
	virt_queue->descriptor_table[1].flags = 3;
	virt_queue->descriptor_table[1].next = 2;

	virt_queue->descriptor_table[2].address =
		base_phys + sizeof(VirtioBlkReq) + 512;
	virt_queue->descriptor_table[2].length = 1;
	virt_queue->descriptor_table[2].flags = 2;

	virt_queue->available_ring->ring[0] = 0;
	__sync_synchronize();
	virt_queue->available_ring->idx++;
	__sync_synchronize();
	// write 0 to notify
	BarAddress addr =
		get_address_from_bar(virtio_block_device, notify_config->cap.bar);
	uint16_t *idx = (uint16_t *)(hhdm_request.response->offset + addr.address +
	                             notify_config->cap.offset +
	                             virt_queue->notify_offset *
	                                 notify_config->notify_off_multiplier);
	*idx = 0;
	__sync_synchronize();
	while (virt_queue->used_ring->idx != 1)
		;

	uint8_t status = *((uint8_t *)data + sizeof(VirtioBlkReq) + 512);
	uint8_t *disk_data = (uint8_t *)data + sizeof(VirtioBlkReq);

	printf("status: 0x%x\n", (uint64_t)status);

	printf("data: \n");
	for (uint64_t i = 0; i < 512; i += 8) {
		printf(" 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", disk_data[i + 0],
		       disk_data[i + 1], disk_data[i + 2], disk_data[i + 3],
		       disk_data[i + 4], disk_data[i + 5], disk_data[i + 6],
		       disk_data[i + 7]);
	}
	printf("\n");

	return 0;
};

PCIDriver virtio_blk_drv = {
	.name = "virtio_blk",
	.id_table = id_table,
	.probe = virtio_blk_probe,
};
