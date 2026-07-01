#include "driver/virtio_blk.hpp"
#include "disk.hpp"
#include "driver/console.hpp"
#include "driver/pci.hpp"
#include "limine/limine_requests.hpp"
#include "list/container_of.hpp"
#include "list/klist.hpp"
#include "memory/virtual_memory.hpp"
#include "panic.hpp"
#include "utils.hpp"
#include <stdint.h>

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

VirtQueue *create_virtqueue(volatile pci_header *pci_device,
                            volatile VirtioPciNotifyCfg *notify_cfg,
                            volatile VirtioPciCommonCfg *cfg, uint32_t index) {
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
	BarAddress addr = get_address_from_bar(pci_device, notify_cfg->cap.bar);
	virt_queue->notify_address =
		(uint64_t)(hhdm_request.response->offset + addr.address +
	               notify_cfg->cap.offset +
	               cfg->queue_notify_off * notify_cfg->notify_off_multiplier);

	cfg->queue_desc =
		(uint64_t)virt_queue->descriptor_table - hhdm_request.response->offset;
	cfg->queue_driver =
		(uint64_t)virt_queue->available_ring - hhdm_request.response->offset;
	cfg->queue_device =
		(uint64_t)virt_queue->used_ring - hhdm_request.response->offset;

	cfg->queue_enable = 1;
	__sync_synchronize();
	return virt_queue;
}

uint16_t virt_queue_get_next_index(VirtQueue *virt_queue) {
	uint16_t index = virt_queue->next_descriptor_index++ % virt_queue->size;
	return index;
}

void virt_queue_notify(VirtQueue virt_queue) {
	__sync_synchronize();
	*((uint16_t *)virt_queue.notify_address) = 0;
	__sync_synchronize();
}

uint16_t virt_queue_send_chain(VirtQueue *virt_queue,
                               VirtQueueDescriptor *descriptors,
                               uint16_t length) {
	uint16_t index = virt_queue_get_next_index(virt_queue);
	for (uint16_t i = 1; i < length; i++) {
		descriptors[i - 1].next = virt_queue_get_next_index(virt_queue);
	}

	for (uint16_t i = 0, current_index = index; i < length; i++) {
		virt_queue->descriptor_table[current_index] = descriptors[i];
		if ((i + 1) < length) {
			current_index = descriptors[i].next;
		}
	}
	virt_queue->available_ring
		->ring[virt_queue->available_ring->idx % virt_queue->size] = index;
	__sync_synchronize();
	virt_queue->available_ring->idx++;
	__sync_synchronize();
	return index;
}

void virtio_blk_read(Disk *disk, uint64_t start_sector, void *buffer,
                     uint64_t sector_count) {
	VirtioBlk *device = (VirtioBlk *)disk->owner;
	if (buffer == nullptr || sector_count == 0 || device == nullptr)
		return;

	VirtioBlkReq *request = (VirtioBlkReq *)kzalloc(sizeof(VirtioBlkReq) + 1);
	uint8_t *status = (uint8_t *)request + sizeof(VirtioBlkReq);

	request->type = 0;
	request->sector = start_sector;

	VirtQueueDescriptor descriptors[3];
	descriptors[0] = {
		.address = ((uint64_t)request) - hhdm_request.response->offset,
		.length = sizeof(VirtioBlkReq),
		.flags = 1,
	};

	descriptors[1] = {
		.address = (uint64_t)buffer - hhdm_request.response->offset,
		.length = 512 * (uint32_t)sector_count,
		.flags = 3,
	};

	descriptors[2] = {
		.address = ((uint64_t)status) - hhdm_request.response->offset,
		.length = 1,
		.flags = 2,
	};

	VirtQueue *virt_queue =
		container_of(device->virt_queues.next, VirtQueue, list);

	uint16_t start = virt_queue_send_chain(virt_queue, descriptors, 3);

	uint16_t next_id = virt_queue->used_ring->idx + 1;
#define ttt 0
#if ttt
	printf("ddd(before): \n");
	for (uint64_t i = 0; i < 512; i += 8) {
		printf(" 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
		       ((uint8_t *)buffer)[i + 0], ((uint8_t *)buffer)[i + 1],
		       ((uint8_t *)buffer)[i + 2], ((uint8_t *)buffer)[i + 3],
		       ((uint8_t *)buffer)[i + 4], ((uint8_t *)buffer)[i + 5],
		       ((uint8_t *)buffer)[i + 6], ((uint8_t *)buffer)[i + 7]);
	}
	printf("\n");
#endif

	virt_queue_notify(*virt_queue);
#if 1
	// while (virt_queue->used_ring->idx != next_id) {
	//	asm("hlt");
	// }

	uint16_t last_seen = virt_queue->used_ring->idx;

	while (1) {
		__sync_synchronize();

		uint16_t current = virt_queue->used_ring->idx;

		while (last_seen != current) {
			VirtQueueUsedElement *elem =
				&virt_queue->used_ring->ring[last_seen % virt_queue->size];

			if (elem->id == start)
				goto done;

			last_seen++;
		}

		asm("hlt");
	}
done:
#if ttt
	printf("ddd(after)(%lu): \n", start_sector);
	for (uint64_t i = 0; i < 512; i += 8) {
		printf(" 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
		       ((uint8_t *)buffer)[i + 0], ((uint8_t *)buffer)[i + 1],
		       ((uint8_t *)buffer)[i + 2], ((uint8_t *)buffer)[i + 3],
		       ((uint8_t *)buffer)[i + 4], ((uint8_t *)buffer)[i + 5],
		       ((uint8_t *)buffer)[i + 6], ((uint8_t *)buffer)[i + 7]);
	}
	printf("\n");
#endif

	// while (1) {
	//	for (int i = 0; i < virt_queue->size; i++) {
	//		if (virt_queue->used_ring->ring[i].id == start) {
	//			goto done;
	//		}
	//	}
	//	asm("hlt");
	// }
#else
	VirtQueueUsedElement elem =
		virt_queue->used_ring
			->ring[(virt_queue->used_ring->idx - 1) % virt_queue->size];

	while (elem.id != start) {
		asm("hlt");
		elem = virt_queue->used_ring
		           ->ring[(virt_queue->used_ring->idx - 1) % virt_queue->size];
	}
#endif

	kfree(request);

	return;
}

void virtio_blk_write(Disk *disk, uint64_t start_sector, void *buffer,
                      uint64_t sector_count) {
	VirtioBlk *device = (VirtioBlk *)disk->owner;
	if (buffer == nullptr && sector_count == 0 && device == nullptr)
		return;
	UNIMPLEMENTED();
}

DiskOperations virtio_blk_dops = {
	.read = &virtio_blk_read,
	.write = &virtio_blk_write,
};

int virtio_blk_probe(PCIDevice *device) {
	pci_header *virtio_block_pci_device = (pci_header *)device->config_address;
	VirtioBlk *virtio_block_device = (VirtioBlk *)kzalloc(sizeof(VirtioBlk));

	device->device.driver_data = (void *)virtio_block_device;
	virtio_block_device->pci_device = device;
	klist_init(&virtio_block_device->virt_queues);

	pci_capability *capability =
		(pci_capability
	         *)((uint64_t)virtio_block_pci_device +
	            (virtio_block_pci_device->type_0.capabilities_pointer & ~0x3));
	// 64-bit bar 4: bar5[63:32] bar4[31:0]
	bool mapped_bar[] = {false, false, false, false, false};
	while (capability != nullptr) {
		if (capability->id == 0x9) {
			VirtioPciCapability *virtio_capability =
				(VirtioPciCapability *)capability;

			BarAddress addr = get_address_from_bar(virtio_block_pci_device,
			                                       virtio_capability->bar);

			if (addr.is_memory_space) {

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
					virtio_block_device->device_config =
						(VirtioBlkConfig *)(addr.address +
					                        hhdm_request.response->offset +
					                        virtio_capability->offset);

				} else if (virtio_capability->config_type ==
				           VIRTIO_PCI_CAP_COMMON_CFG) {
					virtio_block_device->common_config =
						(VirtioPciCommonCfg *)(addr.address +
					                           hhdm_request.response->offset +
					                           virtio_capability->offset);
				} else if (virtio_capability->config_type ==
				           VIRTIO_PCI_CAP_NOTIFY_CFG) {
					virtio_block_device->notify_config =
						(VirtioPciNotifyCfg *)capability;
				} else if (virtio_capability->config_type ==
				           VIRTIO_PCI_CAP_ISR_CFG) {
					virtio_block_device->isr_config =
						(uint8_t *)(addr.address + virtio_capability->offset +
					                hhdm_request.response->offset);
				}
			}
		}
		if (capability->next == 0)
			break;

		capability = (pci_capability *)((uint64_t)virtio_block_pci_device +
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

	virtio_block_device->common_config->device_status = DEVICE_RESET;
	virtio_block_device->common_config->device_status |= DEVICE_ACKNOWLEDGE;
	virtio_block_device->common_config->device_status |= DEVICE_DRIVER;

	virtio_block_device->common_config->driver_feature_select = 0;
	virtio_block_device->common_config->device_feature_select = 0;
	// SEG_MAX(2) GEOMETRY(4) BLK_SIZE(6) EVENT_IDX(29) VERSION_1(32)

	if (is_feature_available(virtio_block_device->common_config, 2)) {
		set_feature(virtio_block_device->common_config, 2);
	}
	if (is_feature_available(virtio_block_device->common_config, 4)) {
		set_feature(virtio_block_device->common_config, 4);
	}
	if (is_feature_available(virtio_block_device->common_config, 6)) {
		set_feature(virtio_block_device->common_config, 6);
	}
	if (is_feature_available(virtio_block_device->common_config, 29)) {
		set_feature(virtio_block_device->common_config, 29);
	}
	if (is_feature_available(virtio_block_device->common_config, 32)) {
		set_feature(virtio_block_device->common_config, 32);
	}
	virtio_block_device->common_config->device_status |= DEVICE_FEATURES_OK;

	virtio_block_device->common_config->driver_feature_select = 0;
	virtio_block_device->common_config->device_feature_select = 0;

	if (virtio_block_device->common_config->device_status & DEVICE_FAILED) {
		printf("failed\n");
		hcf();
	}

	VirtQueue *virt_queue = create_virtqueue(
		virtio_block_pci_device, virtio_block_device->notify_config,
		virtio_block_device->common_config, 0);
	klist_add_tail(&virtio_block_device->virt_queues, &virt_queue->list);
	virtio_block_device->common_config->device_status |= 4;

	Disk *disk = (Disk *)kzalloc(sizeof(Disk));
	disk->owner = virtio_block_device;
	disk->sector_size = 512;
	disk->size_in_sectors = virtio_block_device->device_config->capacity;
	disk->disk_ops = virtio_blk_dops;
	klist_init(&disk->siblings);
	klist_add_tail(&disk_list, &disk->global);

	return 0;
	// FIXME: remove the code for real use

	/*uint32_t request_size = sizeof(VirtioBlkReq);
	VirtioBlkReq *data = (VirtioBlkReq *)kzalloc(request_size + 513);
	data->sector = 0;
	uint64_t base_phys = (uint64_t)data - hhdm_request.response->offset;

	VirtQueueDescriptor descriptors[3];
	descriptors[0] = {
	    .address = base_phys,
	    .length = request_size,
	    .flags = 1,
	};

	descriptors[1] = {
	    .address = base_phys + sizeof(VirtioBlkReq),
	    .length = 512,
	    .flags = 3,
	};

	descriptors[2] = {
	    .address = base_phys + sizeof(VirtioBlkReq) + 512,
	    .length = 1,
	    .flags = 2,
	};

	virt_queue_send_chain(virt_queue, descriptors, 3);
	// write 0 to notify
	BarAddress addr = get_address_from_bar(
	    virtio_block_pci_device, virtio_block_device->notify_config->cap.bar);
	uint16_t *idx =
	    (uint16_t *)(hhdm_request.response->offset + addr.address +
	                 virtio_block_device->notify_config->cap.offset +
	                 virt_queue->notify_offset *
	                     virtio_block_device->notify_config
	                         ->notify_off_multiplier);
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

	data->sector = 2048;
	virt_queue_send_chain(virt_queue, descriptors, 3);
	*idx = 0;
	__sync_synchronize();
	while (virt_queue->used_ring->idx != 2)
	    ;

	status = *((uint8_t *)data + sizeof(VirtioBlkReq) + 512);

	printf("status: 0x%x\n", (uint64_t)status);

	printf("data: \n");
	for (uint64_t i = 0; i < 512; i += 8) {
	    printf(" 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", disk_data[i + 0],
	           disk_data[i + 1], disk_data[i + 2], disk_data[i + 3],
	           disk_data[i + 4], disk_data[i + 5], disk_data[i + 6],
	           disk_data[i + 7]);
	}
	printf("\n"); */

	return 0;
};

PCIDriver virtio_blk_drv = {
	.name = "virtio_blk",
	.id_table = id_table,
	.probe = virtio_blk_probe,
};
