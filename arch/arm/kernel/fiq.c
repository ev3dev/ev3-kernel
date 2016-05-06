/*
 *  linux/arch/arm/kernel/fiq.c
 *
 *  Copyright (C) 1998 Russell King
 *  Copyright (C) 1998, 1999 Phil Blundell
 *
 *  FIQ support written by Philip Blundell <philb@gnu.org>, 1998.
 *
 *  FIQ support re-written by Russell King to be more generic
 *
 *  FIQ handler in C support written by Andy Green <andy@openmoko.com>
 *
 * We now properly support a method by which the FIQ handlers can
 * be stacked onto the vector.  We still do not support sharing
 * the FIQ vector itself.
 *
 * Operation is as follows:
 *  1. Owner A claims FIQ:
 *     - default_fiq relinquishes control.
 *  2. Owner A:
 *     - inserts code.
 *     - sets any registers,
 *     - enables FIQ.
 *  3. Owner B claims FIQ:
 *     - if owner A has a relinquish function.
 *       - disable FIQs.
 *       - saves any registers.
 *       - returns zero.
 *  4. Owner B:
 *     - inserts code.
 *     - sets any registers,
 *     - enables FIQ.
 *  5. Owner B releases FIQ:
 *     - Owner A is asked to reacquire FIQ:
 *	 - inserts code.
 *	 - restores saved registers.
 *	 - enables FIQ.
 *  6. Goto 3
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/seq_file.h>

#include <asm/cacheflush.h>
#include <asm/cp15.h>
#include <asm/fiq.h>
#include <asm/irq.h>
#include <asm/traps.h>

#define FIQ_OFFSET ({					\
		extern void *vector_fiq_offset;		\
		(unsigned)&vector_fiq_offset;		\
	})

static unsigned long dfl_fiq_insn;
static struct pt_regs dfl_fiq_regs;

/* Default reacquire function
 * - we always relinquish FIQ control
 * - we always reacquire FIQ control
 */
static int fiq_def_op(void *ref, int relinquish)
{
	if (!relinquish) {
		/* Restore default handler and registers */
		local_fiq_disable();
		set_fiq_regs(&dfl_fiq_regs);
		set_fiq_handler(&dfl_fiq_insn, sizeof(dfl_fiq_insn));
		local_fiq_enable();

		/* FIXME: notify irq controller to standard enable FIQs */
	}

	return 0;
}

static struct fiq_handler default_owner = {
	.name	= "default",
	.fiq_op = fiq_def_op,
};

static struct fiq_handler *current_fiq = &default_owner;

int show_fiq_list(struct seq_file *p, int prec)
{
	if (current_fiq != &default_owner)
		seq_printf(p, "%*s:              %s\n", prec, "FIQ",
			current_fiq->name);

	return 0;
}

void set_fiq_handler(void *start, unsigned int length)
{
	void *base = vectors_page;
	unsigned offset = FIQ_OFFSET;

	memcpy(base + offset, start, length);
	if (!cache_is_vipt_nonaliasing())
		flush_icache_range((unsigned long)base + offset, offset +
				   length);
	flush_icache_range(0xffff0000 + offset, 0xffff0000 + offset + length);
}

/* ---------------------------- FIQ handler in C ---------------------------- */
/*
 * Major Caveats for using this
 * ----------------------------
 *
 * 1) it CANNOT touch any vmalloc()'d memory, only memory
 *    that was kmalloc()'d.  Static allocations in the monolithic kernel
 *    are kmalloc()'d so they are okay.  You can touch memory-mapped IO, but
 *    the pointer for it has to have been stored in kmalloc'd memory.  The
 *    reason for this is simple: every now and then Linux turns off interrupts
 *    and reorders the paging tables.  If a FIQ happens during this time, the
 *    virtual memory space can be partly or entirely disordered or missing.
 *
 * 2) Because vmalloc() is used when a module is inserted, THIS FIQ
 *    ISR HAS TO BE IN THE MONOLITHIC KERNEL, not a module.  But the way
 *    it is set up, you can all to enable and disable it from your module
 *    and intercommunicate with it through struct fiq_ipc
 *    fiq_ipc which you can define in
 *    asm/archfiq_ipc_type.h.  The reason is the same as above, a
 *    FIQ could happen while even the ISR is not present in virtual memory
 *    space due to pagetables being changed at the time.
 *
 * 3) You can't call any Linux API code except simple macros
 *    - understand that FIQ can come in at any time, no matter what
 *      state of undress the kernel may privately be in, thinking it
 *      locked the door by turning off interrupts... FIQ is an
 *      unstoppable monster force (which is its value)
 *    - they are not vmalloc()'d memory safe
 *    - they might do crazy stuff like sleep: FIQ pisses fire and
 *      is not interested in 'sleep' that the weak seem to need
 *    - calling APIs from FIQ can re-enter un-renterable things
 *    - summary: you cannot interoperate with linux APIs directly in the FIQ ISR
 *
 * If you follow these rules, it is fantastic, an extremely powerful, solid,
 * genuine hard realtime feature.
 */

static void (*current_fiq_c_isr)(void);

#define FIQ_C_ISR_STACK_SIZE 	256

static void __attribute__((naked)) __jump_to_isr(void)
{
	asm __volatile__ ("mov pc, r8");
}


static void __attribute__((naked)) __actual_isr(void)
{
	asm __volatile__ (
		"stmdb	sp!, {r0-r12, lr};"
		"mov     fp, sp;"
	);

	current_fiq_c_isr();

	asm __volatile__ (
		"ldmia	sp!, {r0-r12, lr};"
		"subs	pc, lr, #4;"
	);
}

fiq_c_handler_t get_fiq_c_handler(void)
{
	return current_fiq_c_isr;
}

void set_fiq_c_handler(fiq_c_handler_t isr)
{
	struct pt_regs regs;

	memset(&regs, 0, sizeof(regs));
	regs.ARM_r8 = (unsigned long) __actual_isr;
	regs.ARM_sp = 0xffff001c + FIQ_C_ISR_STACK_SIZE;

	set_fiq_handler(__jump_to_isr, 4);

	current_fiq_c_isr = isr;

	set_fiq_regs(&regs);
}
/* ---------------------------- FIQ handler in C ---------------------------- */

int claim_fiq(struct fiq_handler *f)
{
	int ret = 0;

	if (current_fiq) {
		ret = -EBUSY;

		if (current_fiq->fiq_op != NULL)
			ret = current_fiq->fiq_op(current_fiq->dev_id, 1);
	}

	if (!ret) {
		f->next = current_fiq;
		current_fiq = f;
	}

	return ret;
}

void release_fiq(struct fiq_handler *f)
{
	if (current_fiq != f) {
		pr_err("%s FIQ trying to release %s FIQ\n",
		       f->name, current_fiq->name);
		dump_stack();
		return;
	}

	do
		current_fiq = current_fiq->next;
	while (current_fiq->fiq_op(current_fiq->dev_id, 0));
}

static int fiq_start;

void enable_fiq(int fiq)
{
	enable_irq(fiq + fiq_start);
}

void disable_fiq(int fiq)
{
	disable_irq(fiq + fiq_start);
}

EXPORT_SYMBOL(set_fiq_handler);
EXPORT_SYMBOL(get_fiq_c_handler);
EXPORT_SYMBOL(set_fiq_c_handler);
EXPORT_SYMBOL(__set_fiq_regs);	/* defined in fiqasm.S */
EXPORT_SYMBOL(__get_fiq_regs);	/* defined in fiqasm.S */
EXPORT_SYMBOL(claim_fiq);
EXPORT_SYMBOL(release_fiq);
EXPORT_SYMBOL(enable_fiq);
EXPORT_SYMBOL(disable_fiq);

void __init init_FIQ(int start)
{
	unsigned offset = FIQ_OFFSET;
	dfl_fiq_insn = *(unsigned long *)(0xffff0000 + offset);
	get_fiq_regs(&dfl_fiq_regs);
	fiq_start = start;
}
