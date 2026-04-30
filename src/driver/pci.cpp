#include "driver/pci.hpp"
#include "driver/console.hpp"
#include "driver/device.hpp"
#include "list/container_of.hpp"

void register_pci_driver(PCIDriver *driver) {
	driver->driver.name = driver->name;
	register_driver(&driver->driver, &pci_bus);
}
void register_pci_device(PCIDevice *device) {
	register_device(&device->device, &pci_bus);
}

int PCI_PROBE(Device *dev) {
	printf("PCI probe\n");
	PCIDevice *pci_dev = container_of(dev, PCIDevice, device);
	PCIDriver *pci_drv =
		container_of(pci_dev->device.active_driver, PCIDriver, driver);
	pci_drv->probe(pci_dev);
	return 1;
}

int is_list_terminator(PCIDeviceID id) {
	return id.vendor_id == 0;
}

int PCI_MATCH(Device *dev, Driver *drv) {
	printf("PCI match\n");
	PCIDevice *pci_dev = container_of(dev, PCIDevice, device);
	PCIDriver *pci_drv = container_of(drv, PCIDriver, driver);
	PCIDeviceID *id_list = pci_drv->id_table;
	if (id_list) {
		while (!is_list_terminator(*id_list)) {
			if (!(pci_dev->vendor_id == id_list->vendor_id ||
			      id_list->vendor_id == PCI_ANY_ID)) {
				continue;
			}
			if (!(pci_dev->device_id == id_list->device_id ||
			      id_list->device_id == PCI_ANY_ID)) {
				continue;
			}
			return 0;
		}
	}
	return 1;
}

BusType pci_bus = {.name = "PCI", .match = &PCI_MATCH, .probe = &PCI_PROBE};
