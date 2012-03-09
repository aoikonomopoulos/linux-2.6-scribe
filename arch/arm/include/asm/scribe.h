#ifndef _ASM_ARM_SCRIBE_H_
#define _ASM_ARM_SCRIBE_H_

struct scribe_ps_arch {
};

struct scribe_ps;
static inline int init_scribe_arch(struct scribe_ps *scribe) { return 0; };
static inline void exit_scribe_arch(struct scribe_ps *scribe) { };
static inline void scribe_attach_arch(struct scribe_ps *scribe) { };
static inline void scribe_detach_arch(struct scribe_ps *scribe) { };

#endif /* _ASM_ARM_SCRIBE_H_ */
