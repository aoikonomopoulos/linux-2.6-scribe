/*
 * syscall.h - Linux syscall interfaces for ARM
 *
 * Copyright (c) 2010 Christoffer Dall
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#ifndef _ASM_ARM_SYSCALLS_H
#define _ASM_ARM_SYSCALLS_H

#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/memory.h>
#include <asm/unistd.h>

int syscall_get_nr(struct task_struct *task, struct pt_regs *regs);

static inline long syscall_get_return_value(struct task_struct *task,
					    struct pt_regs *regs)
{
	return regs->ARM_r0;
}

static inline long syscall_get_error(struct task_struct *task,
				     struct pt_regs *regs)
{
	return regs->ARM_r0;
}

#endif /* _ASM_ARM_SYSCALLS_H */
