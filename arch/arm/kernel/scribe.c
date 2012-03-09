#include <linux/scribe.h>
#include <asm/syscall.h>
#include <asm/proc-fns.h>

int scribe_regs(struct scribe_ps *scribe, struct pt_regs *regs)
{
	struct scribe_event_regs *event_regs;
	int ret;

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
	return syscall_get_nr(current, regs);
}


void scribe_mm_switch_pgd(struct mm_struct *mm, pgd_t *pgd)
{
	cpu_switch_mm(pgd, mm);
}
