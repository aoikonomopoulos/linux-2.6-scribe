/*
 * Copyright (C) 2010 Oren Laadan <orenl@cs.columbia.edu>
 * Copyright (C) 2010 Nicolas Viennot <nicolas@viennot.biz>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of the Linux
 *  distribution for more details.
 */

#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/scribe.h>
#include <linux/syscalls.h>
#include <linux/net.h>
#include <linux/futex.h>
#include <asm/syscall.h>

union scribe_syscall_event_union {
	struct scribe_event *generic;
	struct scribe_event_syscall *regular;
	struct scribe_event_syscall_extra *extra;
};

static int scribe_regs(struct scribe_ps *scribe, struct pt_regs *regs)
{
	struct scribe_event_regs *event_regs;
	struct pt_regs regs_tmp;
	int ret;

	/* We don't want to touch the given registers */
	regs_tmp = *regs;
	regs = &regs_tmp;

	/*
	 * Somehow the high bits are non zero in some cases, don't really know
	 * why.
	 */
	regs->gs &= 0xFFFF;
	regs->fs &= 0xFFFF;
	regs->es &= 0xFFFF;
	regs->ds &= 0xFFFF;
	regs->flags &= 0xFFFF;
	regs->cs &= 0xFFFF;
	regs->ss &= 0xFFFF;

	if (is_recording(scribe)) {
		if (scribe_queue_new_event(scribe->queue, SCRIBE_EVENT_REGS,
					   .regs = *regs)) {
			scribe_kill(scribe->ctx, -ENOMEM);
			return -ENOMEM;
		}
	} else {
		event_regs = scribe_dequeue_event_specific(scribe,
						SCRIBE_EVENT_REGS);
		if (IS_ERR(event_regs))
			return PTR_ERR(event_regs);

		ret = memcmp(regs, &event_regs->regs, sizeof(*regs));
		scribe_free_event(event_regs);

		if (ret) {
			scribe_diverge(scribe, SCRIBE_EVENT_DIVERGE_REGS,
				       .regs = *regs);
			return -EDIVERGE;
		}
	}

	return 0;
}

static inline int is_scribe_syscall(int nr)
{
	return nr == __NR_get_scribe_flags ||
	       nr == __NR_set_scribe_flags;
}

static int scribe_need_syscall_ret_record(struct scribe_ps *scribe)
{
	/*
	 * We'll postpone the insertion of the syscall event for the
	 * return value.
	 *
	 * XXX This is potentially dangerous in the sense that the
	 * userspace can make the kernel allocate many events during
	 * the syscall, which won't get flushed to the logfile until
	 * the syscall returns.
	 */
	scribe_create_insert_point(&scribe->syscall_ip, &scribe->queue->stream);
	return 0;
}

static int scribe_need_syscall_ret_replay(struct scribe_ps *scribe)
{
	union scribe_syscall_event_union event;
	int syscall_extra = should_scribe_syscall_extra(scribe);

	if (syscall_extra)
		event.extra = scribe_dequeue_event_specific(scribe,
				      SCRIBE_EVENT_SYSCALL_EXTRA);
	else
		event.regular = scribe_dequeue_event_specific(scribe,
				      SCRIBE_EVENT_SYSCALL);

	if (IS_ERR(event.generic))
		return PTR_ERR(event.generic);

	if (syscall_extra) {
		if (event.extra->nr != scribe->nr_syscall) {
			scribe_diverge(scribe, SCRIBE_EVENT_DIVERGE_SYSCALL,
				       .nr = scribe->nr_syscall);
		}
		scribe->orig_ret = event.extra->ret;
	} else
		scribe->orig_ret = event.regular->ret;

	scribe_free_event(event.generic);

	/*
	 * FIXME Do something about non deterministic errors such as
	 * -ENOMEM.
	 */
	return 0;
}

static int __scribe_need_syscall_ret(struct scribe_ps *scribe)
{
	scribe->need_syscall_ret = true;

	if (is_recording(scribe))
		return scribe_need_syscall_ret_record(scribe);
	else
		return scribe_need_syscall_ret_replay(scribe);
}

int scribe_need_syscall_ret(struct scribe_ps *scribe)
{
	if (!is_scribed(scribe))
		return 0;

	if (!should_scribe_syscalls(scribe))
		return 0;

	if (scribe->need_syscall_ret)
		return 0;

	return __scribe_need_syscall_ret(scribe);
}

static bool is_interruptible_syscall(int nr_syscall)
{
	/*
	 * FIXME Only do that for interruptible system calls (with
	 * nr_syscall).
	 */
	return true;
}

static int get_nr_syscall(struct pt_regs *regs)
{
	unsigned long call;
	int nr;

	nr = syscall_get_nr(current, regs);
	if (nr == __NR_socketcall) {
		syscall_get_arguments(current, regs, 0, 1, &call);
		if (call < 1 || call > SYS_RECVMMSG)
			return nr;

		return SCRIBE_SOCKETCALL_BASE + call;
	}
	if (nr == __NR_futex) {
		syscall_get_arguments(current, regs, 1, 1, &call);
		call &= FUTEX_CMD_MASK;
		if (call > FUTEX_CMP_REQUEUE_PI)
			return nr;

		return SCRIBE_FUTEX_BASE + call;
	}

	return nr;
}

void scribe_enter_syscall(struct pt_regs *regs)
{
	struct scribe_ps *scribe = current->scribe;
	int num_sig_deferred;

	if (!is_scribed(scribe))
		return;

	scribe->nr_syscall = get_nr_syscall(regs);
	if (is_scribe_syscall(scribe->nr_syscall))
		return;

	/* It should already be set to false, but let's be sure */
	scribe->need_syscall_ret = false;

	scribe_data_det();

	scribe_signal_enter_sync_point(&num_sig_deferred);
	if (num_sig_deferred > 0) {
		/* TODO We could go back to userspace to reduce latency */
	}

	__scribe_forbid_uaccess(scribe);

	scribe_bookmark_point(SCRIBE_BOOKMARK_PRE_SYSCALL);

	if (scribe_maybe_detach(scribe))
		return;

	/* FIXME signals needs the return value */
	if (!should_scribe_syscalls(scribe))
		return;

	if (should_scribe_syscall_ret(scribe) ||
	    is_interruptible_syscall(scribe->nr_syscall))
		__scribe_need_syscall_ret(scribe);

	if (should_scribe_syscalls(scribe) &&
	    should_scribe_regs(scribe) &&
	    scribe_regs(scribe, regs))
		return;

	recalc_sigpending();
}

static void scribe_commit_syscall_record(struct scribe_ps *scribe,
					 struct pt_regs *regs, long ret_value)
{
	union scribe_syscall_event_union event;
	int syscall_extra = should_scribe_syscall_extra(scribe);

	if (syscall_extra)
		event.extra = scribe_alloc_event(SCRIBE_EVENT_SYSCALL_EXTRA);
	else
		event.regular = scribe_alloc_event(SCRIBE_EVENT_SYSCALL);

	if (!event.generic)
		goto err;

	if (syscall_extra) {
		event.extra->nr = scribe->nr_syscall;
		event.extra->ret = ret_value;
	} else
		event.regular->ret = ret_value;

	scribe_queue_event_at(&scribe->syscall_ip, event.generic);
	scribe_commit_insert_point(&scribe->syscall_ip);

	if (syscall_extra) {
		if (scribe_queue_new_event(scribe->queue,
				   SCRIBE_EVENT_SYSCALL_END))
			goto err;
	}

	return;
err:
	scribe_kill(scribe->ctx, -ENOMEM);
}

static void scribe_commit_syscall_replay(struct scribe_ps *scribe,
					 struct pt_regs *regs, long ret_value)
{
	struct scribe_event_syscall_end *event_end;
	int syscall_extra = should_scribe_syscall_extra(scribe);

	if (syscall_extra) {
		event_end = scribe_dequeue_event_specific(scribe,
						SCRIBE_EVENT_SYSCALL_END);
		if (!IS_ERR(event_end))
			scribe_free_event(event_end);
	}

	if (should_ret_check(scribe)) {
		if (scribe->orig_ret != ret_value) {
			scribe_diverge(scribe, SCRIBE_EVENT_DIVERGE_SYSCALL_RET,
				       .ret = ret_value);
		}
	}
}

void scribe_commit_syscall(struct scribe_ps *scribe, struct pt_regs *regs,
			   long ret_value)
{
	if (!scribe->need_syscall_ret)
		return;

	scribe->need_syscall_ret = false;

	if (is_recording(scribe))
		scribe_commit_syscall_record(scribe, regs, ret_value);
	else
		scribe_commit_syscall_replay(scribe, regs, ret_value);
}

void scribe_exit_syscall(struct pt_regs *regs)
{
	struct scribe_ps *scribe = current->scribe;

	if (!is_scribed(scribe))
		return;

	if (is_scribe_syscall(scribe->nr_syscall))
		return;

	scribe_commit_syscall(scribe, regs,
			      syscall_get_return_value(current, regs));

	scribe_bookmark_point(SCRIBE_BOOKMARK_POST_SYSCALL);

	if (scribe_maybe_detach(scribe))
		return;

	__scribe_allow_uaccess(scribe);
	scribe_signal_leave_sync_point();

	/*
	 * During the replay, the sigpending flag was cleared to not disturb
	 * the syscall. Now we want do_signal() to be called if needed.
	 * Note: If the syscall was interrupted with a fake signal,
	 * we are not clearing the sigpending flag either.
	 */
	recalc_sigpending_and_wake(current);

	if (unlikely(!scribe->can_uaccess))
		scribe_kill(scribe->ctx, -EINVAL);
}

void scribe_ret_from_fork(struct pt_regs *regs)
{
	struct scribe_ps *scribe = current->scribe;

	if (!is_scribed(scribe))
		return;

	scribe_bookmark_point(SCRIBE_BOOKMARK_POST_SYSCALL);

	if (scribe_maybe_detach(scribe))
		return;

	__scribe_allow_uaccess(scribe);
}

static inline struct task_struct *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_vpid(pid) : current;
}

static int do_scribe_flags(pid_t pid, unsigned long *in_flags,
			   unsigned long __user *out_flags)
{
	struct task_struct *t;
	struct scribe_ps *scribe;
	unsigned long old_flags = 0;
	int err;

	rcu_read_lock();
	err = -ESRCH;
	t = find_process_by_pid(pid);
	if (!t)
		goto out;
	scribe = t->scribe;

	err = -EINVAL;
	if (!is_scribed(scribe))
		goto out;

	old_flags = scribe->flags;
	if (in_flags) {
		/* We allow only enable flags to be set */
		scribe->flags &= ~SCRIBE_PS_ENABLE_ALL;
		scribe->flags |= *in_flags & SCRIBE_PS_ENABLE_ALL;
		scribe_mem_reload(scribe);
	}

	err = 0;
out:
	rcu_read_unlock();

	if (out_flags && !err && put_user(old_flags, out_flags))
	    err = -EFAULT;
	return err;
}

SYSCALL_DEFINE2(get_scribe_flags, pid_t, pid,
		unsigned long __user *, flags)
{
	return do_scribe_flags(pid, NULL, flags);
}

SYSCALL_DEFINE2(set_scribe_flags, pid_t, pid,
		unsigned long, flags)
{
	return do_scribe_flags(pid, &flags, NULL);
}
