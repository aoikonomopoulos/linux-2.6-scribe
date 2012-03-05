/*
 *  Scribe, the record/replay mechanism
 *
 * Copyright (C) 2010 Oren Laadan <orenl@cs.columbia.edu>
 * Copyright (C) 2010 Nicolas Viennot <nicolas@viennot.biz>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of the Linux
 *  distribution for more details.
 */

#ifndef _ASM_X86_RESOURCE_H_
#define _ASM_X86_RESOURCE_H_

#ifdef CONFIG_SCRIBE

struct scribe_ps_arch {
	int tsc_disabled;
};

struct scribe_ps;
extern int init_scribe_arch(struct scribe_ps *scribe);
extern void exit_scribe_arch(struct scribe_ps *scribe);
extern void scribe_attach_arch(struct scribe_ps *scribe);
extern void scribe_detach_arch(struct scribe_ps *scribe);

extern int scribe_handle_rdtsc(struct scribe_ps *scribe, struct pt_regs *regs);

#ifdef CONFIG_X86_32
/*
 * Syscalls offsets for multiplexed calls
 */
#define SCRIBE_SOCKETCALL_BASE		0xf000
#define SCRIBE_FUTEX_BASE		0xf100

#define __NR_socket		(SCRIBE_SOCKETCALL_BASE + SYS_SOCKET)
#define __NR_bind		(SCRIBE_SOCKETCALL_BASE + SYS_BIND)
#define __NR_connect		(SCRIBE_SOCKETCALL_BASE + SYS_CONNECT)
#define __NR_listen		(SCRIBE_SOCKETCALL_BASE + SYS_LISTEN)
#define __NR_accept		(SCRIBE_SOCKETCALL_BASE + SYS_ACCEPT)
#define __NR_getsockname	(SCRIBE_SOCKETCALL_BASE + SYS_GETSOCKNAME)
#define __NR_getpeername	(SCRIBE_SOCKETCALL_BASE + SYS_GETPEERNAME)
#define __NR_socketpair		(SCRIBE_SOCKETCALL_BASE + SYS_SOCKETPAIR)
#define __NR_send		(SCRIBE_SOCKETCALL_BASE + SYS_SEND)
#define __NR_recv		(SCRIBE_SOCKETCALL_BASE + SYS_RECV)
#define __NR_sendto		(SCRIBE_SOCKETCALL_BASE + SYS_SENDTO)
#define __NR_recvfrom		(SCRIBE_SOCKETCALL_BASE + SYS_RECVFROM)
#define __NR_shutdown		(SCRIBE_SOCKETCALL_BASE + SYS_SHUTDOWN)
#define __NR_setsockopt		(SCRIBE_SOCKETCALL_BASE + SYS_SETSOCKOPT)
#define __NR_getsockopt		(SCRIBE_SOCKETCALL_BASE + SYS_GETSOCKOPT)
#define __NR_sendmsg		(SCRIBE_SOCKETCALL_BASE + SYS_SENDMSG)
#define __NR_recvmsg		(SCRIBE_SOCKETCALL_BASE + SYS_RECVMSG)
#define __NR_accept4		(SCRIBE_SOCKETCALL_BASE + SYS_ACCEPT4)
#define __NR_recvmmsg2		(SCRIBE_SOCKETCALL_BASE + SYS_RECVMMSG)

#define __NR_futex_wait		(SCRIBE_FUTEX_BASE + FUTEX_WAIT)
#define __NR_futex_wake		(SCRIBE_FUTEX_BASE + FUTEX_WAKE)
#define __NR_futex_fd		(SCRIBE_FUTEX_BASE + FUTEX_FD)
#define __NR_futex_requeue	(SCRIBE_FUTEX_BASE + FUTEX_REQUEUE)
#define __NR_futex_cmp_requeue	(SCRIBE_FUTEX_BASE + FUTEX_CMP_REQUEUE)
#define __NR_futex_wake_op	(SCRIBE_FUTEX_BASE + FUTEX_WAKE_OP)
#define __NR_futex_lock_pi	(SCRIBE_FUTEX_BASE + FUTEX_LOCK_PI)
#define __NR_futex_unlock_pi	(SCRIBE_FUTEX_BASE + FUTEX_UNLOCK_PI)
#define __NR_futex_trylock_pi	(SCRIBE_FUTEX_BASE + FUTEX_TRYLOCK_PI)
#define __NR_futex_wait_bitset	(SCRIBE_FUTEX_BASE + FUTEX_WAIT_BITSET)
#define __NR_futex_wake_bitset	(SCRIBE_FUTEX_BASE + FUTEX_WAKE_BITSET)
#define __NR_futex_wait_requeue_pi (SCRIBE_FUTEX_BASE + FUTEX_WAIT_REQUEUE_PI)
#define __NR_futex_cmp_requeue_pi (SCRIBE_FUTEX_BASE + FUTEX_CMP_REQUEUE_PI)
#endif	/* CONFIG_X86_32 */

#endif /* CONFIG_SCRIBE */

#endif /* _ASM_X86_RESOURCE_H_ */
