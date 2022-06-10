#include <dev/pci.h>

/* Taken from http://pciids.sourceforge.net/v2.2/pci.ids */

static const pci_device_id pci_vendor_1013[] = {{0x00b8, "GD 5446"}, {0, 0}};

static const pci_device_id pci_vendor_1022[] = {
  {0x2000, "79c970 [PCnet32 LANCE]"}, {0, 0}};

static const pci_device_id pci_vendor_11ab[] = {
  {0x4620, "GT-64120/64120A/64121A System Controller"}, {0, 0}};

static const pci_device_id pci_vendor_10ec[] = {
  {0x8139, "RTL-8100/8101L/8139 PCI Fast Ethernet Adapter"}, {0, 0}};

static const pci_device_id pci_vendor_8086[] = {
  {0x7110, "82371AB/EB/MB PIIX4 ISA"},
  {0x7111, "82371AB/EB/MB PIIX4 IDE"},
  {0x7112, "82371AB/EB/MB PIIX4 USB"},
  {0x7113, "82371AB/EB/MB PIIX4 ACPI"},
  {0, 0}};

const pci_vendor_id pci_vendor_list[] = {
  {0x1013, "Cirrus Logic", pci_vendor_1013},
  {0x1022, "Advanced Micro Devices, Inc.", pci_vendor_1022},
  {0x10ec, "Realtek Semiconductor Co., Ltd.", pci_vendor_10ec},
  {0x11ab, "Marvell Technology Group Ltd.", pci_vendor_11ab},
  {0x8086, "Intel Corporation", pci_vendor_8086},
  {0, 0, 0}};

const char *pci_class_code[] = {
  "",
  "Mass Storage Controller",
  "Network Controller",
  "Display Controller",
  "Multimedia Controller",
  "Memory Controller",
  "Bridge Device",
  "Simple Communication Controllers",
  "Base System Peripherals",
  "Input Devices",
  "Docking Stations",
  "Processors",
  "Serial Bus Controllers",
  "Wireless Controllers",
  "Intelligent I/O Controllers",
  "Satellite Communication Controllers",
  "Encryption/Decryption Controllers",
  "Data Acquisition and Signal Processing Controllers"};
