/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * This file defines all mmap-related syscall hooks:
 *	brk
 *	mmap
 *	munmap
 *	msync
 */

#include <lego/syscalls.h>
#include <lego/comp_processor.h>

SYSCALL_DEFINE1(brk, unsigned long, brk)
{
	struct p2m_brk_struct payload;
	unsigned long ret_brk;
	int ret;

	payload.pid = current->pid;
	payload.brk = brk;

	ret = net_send_reply_timeout(DEF_MEM_HOMENODE, P2M_BRK,
			&payload, sizeof(payload), &ret_brk, sizeof(ret_brk),
			false, DEF_NET_TIMEOUT);

	if (likely(ret == sizeof(ret_brk))) {
		if (WARN_ON(ret == RET_ESRCH || ret == RET_EINTR))
			return -EINTR;
		return ret_brk;
	}
	return -EIO;
}

SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags,
		unsigned long, fd, unsigned long, off)
{
	struct p2m_mmap_struct payload;
	struct p2m_mmap_reply_struct reply;
	int ret;

	if (offset_in_page(off))
		return -EINVAL;
	if (!len)
		return -EINVAL;
	len = PAGE_ALIGN(len);
	if (!len)
		return -ENOMEM;
	/* overflowed? */
	if ((off + len) < off)
		return -EOVERFLOW;

	payload.pid = current->pid;
	payload.addr = addr;
	payload.len = len;
	payload.prot = prot;
	payload.flags = flags;
	payload.fd = fd;
	payload.pgoff = off >> PAGE_SHIFT;

	ret = net_send_reply_timeout(DEF_MEM_HOMENODE, P2M_MMAP,
			&payload, sizeof(payload), &reply, sizeof(reply),
			false, DEF_NET_TIMEOUT);

	if (likely(ret == sizeof(reply))) {
		if (likely(reply.ret == RET_OKAY))
			return reply.ret_addr;
		else
			return (s64)reply.ret;
	}
	return -EIO;
}

SYSCALL_DEFINE2(munmap, unsigned long, addr, size_t, len)
{
	struct p2m_munmap_struct payload;
	int ret, retbuf;

	if (offset_in_page(addr) || addr > TASK_SIZE || len > TASK_SIZE - addr)
		return -EINVAL;
	if (!len)
		return -EINVAL;
	len = PAGE_ALIGN(len);
	if (!len)
		return -EINVAL;

	payload.pid = current->pid;
	payload.addr = addr;
	payload.len = len;

	ret = net_send_reply_timeout(DEF_MEM_HOMENODE, P2M_MUNMAP,
			&payload, sizeof(payload), &retbuf, sizeof(retbuf),
			false, DEF_NET_TIMEOUT);

	if (likely(ret == sizeof(retbuf)))
		return retbuf;
	return -EIO;
}

/*
 * MS_SYNC syncs the entire file - including mappings.
 *
 * MS_ASYNC does not start I/O (it used to, up to 2.5.67).
 * Nor does it marks the relevant pages dirty (it used to up to 2.6.17).
 * Now it doesn't do anything, since dirty pages are properly tracked.
 *
 * The application may now run fsync() to
 * write out the dirty pages and wait on the writeout and check the result.
 * Or the application may run fadvise(FADV_DONTNEED) against the fd to start
 * async writeout immediately.
 * So by _not_ starting I/O in MS_ASYNC we provide complete flexibility to
 * applications.
 */
SYSCALL_DEFINE3(msync, unsigned long, start, size_t, len, int, flags)
{
	struct p2m_msync_struct payload;
	int retbuf, ret;
	unsigned long end;

	if (flags & ~(MS_ASYNC | MS_INVALIDATE | MS_SYNC))
		return -EINVAL;
	if (offset_in_page(start))
		return -EINVAL;
	if ((flags & MS_ASYNC) && (flags & MS_SYNC))
		return -EINVAL;
	len = (len + ~PAGE_MASK) & PAGE_MASK;
	end = start + len;
	if (end < start)
		return -ENOMEM;
	if (end == start)
		return 0;

	/* all good, send request */
	payload.pid = current->pid;
	payload.start = start;
	payload.len = len;
	payload.flags = flags;

	ret = net_send_reply_timeout(DEF_MEM_HOMENODE, P2M_MSYNC,
			&payload, sizeof(payload), &retbuf, sizeof(retbuf),
			false, DEF_NET_TIMEOUT);

	if (likely(ret == sizeof(retbuf)))
		return retbuf;
	return -EIO;
}

SYSCALL_DEFINE3(mprotect, unsigned long, start, size_t, len,
		unsigned long, prot)
{
	BUG();
}