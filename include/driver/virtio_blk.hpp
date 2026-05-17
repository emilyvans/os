#ifndef INCLUDE_DRIVER_VIRTIO_BLK_HPP_
#define INCLUDE_DRIVER_VIRTIO_BLK_HPP_
#include "driver/pci.hpp"
#include <stdint.h>

extern PCIDriver virtio_blk_drv;

typedef struct VirtioPciCapability_s {
	pci_capability capability;
	uint8_t length;
	uint8_t config_type;
	uint8_t bar;
	uint8_t id;
	uint8_t padding[2];
	uint32_t offset;
	uint32_t struct_length;
} __attribute__((packed)) VirtioPciCapability;

#define VIRTIO_PCI_CAP_COMMON_CFG 1
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
#define VIRTIO_PCI_CAP_ISR_CFG 3
#define VIRTIO_PCI_CAP_DEVICE_CFG 4

typedef struct VirtioBlkConfig_s {
	uint64_t capacity;
	uint32_t size_max;
	uint32_t seg_max;
	struct virtio_blk_geometry {
		uint16_t cylinders;
		uint8_t heads;
		uint8_t sectors;
	} geometry;
	uint32_t blk_size;
	struct virtio_blk_topology {
		// # of logical blocks per physical block (log2)
		uint8_t physical_block_exp;
		// offset of first aligned logical block
		uint8_t alignment_offset;
		// suggested minimum I/O size in blocks
		uint16_t min_io_size;
		// optimal (suggested maximum) I/O size in blocks
		uint32_t opt_io_size;
	} topology;
	uint8_t writeback;
	uint8_t unused0;
	uint16_t num_queues;
	uint32_t max_discard_sectors;
	uint32_t max_discard_seg;
	uint32_t discard_sector_alignment;
	uint32_t max_write_zeroes_sectors;
	uint32_t max_write_zeroes_seg;
	uint8_t write_zeroes_may_unmap;
	uint8_t unused1[3];
	uint32_t max_secure_erase_sectors;
	uint32_t max_secure_erase_seg;
	uint32_t secure_erase_sector_alignment;
	struct virtio_blk_zoned_characteristics {
		uint32_t zone_sectors;
		uint32_t max_open_zones;
		uint32_t max_active_zones;
		uint32_t max_append_sectors;
		uint32_t write_granularity;
		uint8_t model;
		uint8_t unused2[3];
	} zoned;
} __attribute__((packed)) VirtioBlkConfig;

typedef struct VirtioPciCommonCfg_s {
	uint32_t device_feature_select;
	uint32_t device_feature;
	uint32_t driver_feature_select;
	uint32_t driver_feature;
	uint16_t config_msix_vector;
	uint16_t num_queues;
	uint8_t device_status;
	uint8_t config_generation;
	uint16_t queue_select;
	uint16_t queue_size;
	uint16_t queue_msix_vector;
	uint16_t queue_enable;
	uint16_t queue_notify_off;
	uint64_t queue_desc;
	uint64_t queue_driver;
	uint64_t queue_device;
	uint16_t queue_notify_data;
	uint16_t queue_reset;
} __attribute__((packed)) VirtioPciCommonCfg;

typedef struct VirtioPciNotifyCfg_s {
	VirtioPciCapability cap;
	uint32_t notify_off_multiplier;
} VirtioPciNotifyCfg;

#endif // INCLUDE_DRIVER_VIRTIO_BLK_HPP_
