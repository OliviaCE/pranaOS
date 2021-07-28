/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

// includes
#include <base/ByteReader.h>
#include <base/Optional.h>
#include <base/StringView.h>
#include <kernel/arch/x86/InterruptDisabler.h>
#include <kernel/bus/pci/WindowedMMIOAccess.h>
#include <kernel/Debug.h>
#include <kernel/Sections.h>
#include <kernel/vm/MemoryManager.h>

namespace Kernel {
namespace PCI {

UNMAP_AFTER_INIT DeviceConfigurationSpaceMapping::DeviceConfigurationSpaceMapping(Address device_address, const MMIOAccess::MMIOSegment& mmio_segment)
    : m_device_address(device_address)
    , m_mapped_region(MM.allocate_kernel_region(page_round_up(PCI_MMIO_CONFIG_SPACE_SIZE), "PCI MMIO Device Access", Region::Access::Read | Region::Access::Write).release_nonnull())
{
    PhysicalAddress segment_lower_addr = mmio_segment.get_paddr();
    PhysicalAddress device_physical_mmio_space = segment_lower_addr.offset(
        PCI_MMIO_CONFIG_SPACE_SIZE * m_device_address.function() + (PCI_MMIO_CONFIG_SPACE_SIZE * PCI_MAX_FUNCTIONS_PER_DEVICE) * m_device_address.device() + (PCI_MMIO_CONFIG_SPACE_SIZE * PCI_MAX_FUNCTIONS_PER_DEVICE * PCI_MAX_DEVICES_PER_BUS) * (m_device_address.bus() - mmio_segment.get_start_bus()));
    m_mapped_region->physical_page_slot(0) = PhysicalPage::create(device_physical_mmio_space, MayReturnToFreeList::No);
    m_mapped_region->remap();
}

UNMAP_AFTER_INIT void WindowedMMIOAccess::initialize(PhysicalAddress mcfg)
{
    if (!Access::is_initialized()) {
        new WindowedMMIOAccess(mcfg);
        dbgln_if(PCI_DEBUG, "PCI: MMIO access initialised.");
    }
}

UNMAP_AFTER_INIT WindowedMMIOAccess::WindowedMMIOAccess(PhysicalAddress p_mcfg)
    : MMIOAccess(p_mcfg)
{
    dmesgln("PCI: Using MMIO (mapping per device) for PCI configuration space access");

    InterruptDisabler disabler;

    enumerate_hardware([&](const Address& address, ID) {
        m_mapped_device_regions.append(make<DeviceConfigurationSpaceMapping>(address, m_segments.get(address.seg()).value()));
    });
}

Optional<VirtualAddress> WindowedMMIOAccess::get_device_configuration_space(Address address)
{
    dbgln_if(PCI_DEBUG, "PCI: Getting device configuration space for {}", address);
    for (auto& mapping : m_mapped_device_regions) {
        auto checked_address = mapping.address();
        dbgln_if(PCI_DEBUG, "PCI Device Configuration Space Mapping: Check if {} was requested", checked_address);
        if (address.seg() == checked_address.seg()
            && address.bus() == checked_address.bus()
            && address.device() == checked_address.device()
            && address.function() == checked_address.function()) {
            dbgln_if(PCI_DEBUG, "PCI Device Configuration Space Mapping: Found {}", checked_address);
            return mapping.vaddr();
        }
    }

    dbgln_if(PCI_DEBUG, "PCI: No device configuration space found for {}", address);
    return {};
}

}
}