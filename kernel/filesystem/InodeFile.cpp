/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

// includes
#include <base/StringView.h>
#include <kernel/filesystem/FileDescription.h>
#include <kernel/filesystem/Inode.h>
#include <kernel/filesystem/InodeFile.h>
#include <kernel/filesystem/VirtualFileSystem.h>
#include <kernel/Process.h>
#include <kernel/vm/PrivateInodeVMObject.h>
#include <kernel/vm/SharedInodeVMObject.h>
#include <libc/errno_numbers.h>
#include <libc/sys/ioctl_numbers.h>

namespace Kernel {

InodeFile::InodeFile(NonnullRefPtr<Inode>&& inode)
    : m_inode(move(inode))
{
}

InodeFile::~InodeFile()
{
}

KResultOr<size_t> InodeFile::read(FileDescription& description, u64 offset, UserOrKernelBuffer& buffer, size_t count)
{
    if (Checked<off_t>::addition_would_overflow(offset, count))
        return EOVERFLOW;

    auto result = m_inode->read_bytes(offset, count, buffer, &description);
    if (result.is_error())
        return result.error();
    auto nread = result.value();
    if (nread > 0) {
        Thread::current()->did_file_read(nread);
        evaluate_block_conditions();
    }
    return nread;
}

KResultOr<size_t> InodeFile::write(FileDescription& description, u64 offset, const UserOrKernelBuffer& data, size_t count)
{
    if (Checked<off_t>::addition_would_overflow(offset, count))
        return EOVERFLOW;

    auto result = m_inode->write_bytes(offset, count, data, &description);
    if (result.is_error())
        return result.error();

    auto nwritten = result.value();
    if (nwritten > 0) {
        auto mtime_result = m_inode->set_mtime(kgettimeofday().to_truncated_seconds());
        Thread::current()->did_file_write(nwritten);
        evaluate_block_conditions();
        if (mtime_result.is_error())
            return mtime_result;
    }
    return nwritten;
}

KResult InodeFile::ioctl(FileDescription& description, unsigned request, Userspace<void*> arg)
{
    (void)description;

    switch (request) {
    case FIBMAP: {
        if (!Process::current()->is_superuser())
            return EPERM;

        auto user_block_number = static_ptr_cast<int*>(arg);
        int block_number = 0;
        if (!copy_from_user(&block_number, user_block_number))
            return EFAULT;

        if (block_number < 0)
            return EINVAL;

        auto block_address = inode().get_block_address(block_number);
        if (block_address.is_error())
            return block_address.error();

        if (!copy_to_user(user_block_number, &block_address.value()))
            return EFAULT;

        return KSuccess;
    }
    default:
        return EINVAL;
    }
}

KResultOr<Region*> InodeFile::mmap(Process& process, FileDescription& description, const Range& range, u64 offset, int prot, bool shared)
{
    RefPtr<InodeVMObject> vmobject;
    if (shared)
        vmobject = SharedInodeVMObject::try_create_with_inode(inode());
    else
        vmobject = PrivateInodeVMObject::try_create_with_inode(inode());
    if (!vmobject)
        return ENOMEM;
    return process.space().allocate_region_with_vmobject(range, vmobject.release_nonnull(), offset, description.absolute_path(), prot, shared);
}

String InodeFile::absolute_path(const FileDescription& description) const
{
    VERIFY_NOT_REACHED();
    VERIFY(description.custody());
    return description.absolute_path();
}

KResult InodeFile::truncate(u64 size)
{
    if (auto result = m_inode->truncate(size); result.is_error())
        return result;
    if (auto result = m_inode->set_mtime(kgettimeofday().to_truncated_seconds()); result.is_error())
        return result;
    return KSuccess;
}

KResult InodeFile::chown(FileDescription& description, uid_t uid, gid_t gid)
{
    VERIFY(description.inode() == m_inode);
    VERIFY(description.custody());
    return VirtualFileSystem::the().chown(*description.custody(), uid, gid);
}

KResult InodeFile::chmod(FileDescription& description, mode_t mode)
{
    VERIFY(description.inode() == m_inode);
    VERIFY(description.custody());
    return VirtualFileSystem::the().chmod(*description.custody(), mode);
}

}