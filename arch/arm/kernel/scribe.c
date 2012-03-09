#include <asm/proc-fns.h>

void scribe_mm_switch_pgd(struct mm_struct *mm, pgd_t *pgd)
{
	cpu_switch_mm(pgd, mm);
}
