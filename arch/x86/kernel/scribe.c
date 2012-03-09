#include <linux/scribe.h>
#include <asm/syscall.h>

int scribe_regs(struct scribe_ps *scribe, struct pt_regs *regs)
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

int scribe_get_nr_syscall(struct pt_regs *regs)
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

void scribe_mm_switch_pgd(struct mm_struct *mm, pgd_t *pgd)
{
	(void)mm;
	load_cr3(pgd);
}
