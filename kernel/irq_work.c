/*
 * Copyright (C) 2010 Red Hat, Inc., Peter Zijlstra <pzijlstr@redhat.com>
 *
 * Provides a framework for enqueueing and running callbacks from hardirq
 * context. The enqueueing is NMI-safe.
 */

#include <linux/bug.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/irq_work.h>
#include <linux/percpu.h>
#include <linux/hardirq.h>
#include <linux/irqflags.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <asm/processor.h>


static DEFINE_PER_CPU(struct llist_head, raised_list);
static DEFINE_PER_CPU(struct llist_head, lazy_list);

/*
 * Claim the entry so that no one else will poke at it.
 */
static bool irq_work_claim(struct irq_work *work)
{
	unsigned long flags, oflags, nflags;

	/*
	 * Start with our best wish as a premise but only trust any
	 * flag value after cmpxchg() result.
	 */
	flags = work->flags & ~IRQ_WORK_PENDING;
	for (;;) {
		nflags = flags | IRQ_WORK_FLAGS;
		oflags = cmpxchg(&work->flags, flags, nflags);
		if (oflags == flags)
			break;
		if (oflags & IRQ_WORK_PENDING)
			return false;
		flags = oflags;
		cpu_relax();
	}

	return true;
}

void __weak arch_irq_work_raise(void)
{
	/*
	 * Lame architectures will get the timer tick callback
	 */
}

/*
 * Enqueue the irq_work @entry unless it's already pending
 * somewhere.
 *
 * Can be re-enqueued while the callback is still in progress.
 */
void irq_work_queue(struct irq_work *work)
{
	bool lazy_work, realtime = IS_ENABLED(CONFIG_PREEMPT_RT_FULL);
	struct llist_head *list;

	/* Only queue if not already pending */
	if (!irq_work_claim(work))
		return;

	/* Queue the entry and raise the IPI if needed. */
	preempt_disable();

	/* If the work is "lazy", handle it from next tick if any */
	lazy_work = work->flags & IRQ_WORK_LAZY;

	if (lazy_work || (realtime && !(work->flags & IRQ_WORK_HARD_IRQ)))
		list = this_cpu_ptr(&lazy_list);
	else
		list = this_cpu_ptr(&raised_list);

	if (llist_add(&work->llnode, list)) {
		if (!lazy_work || tick_nohz_tick_stopped())
			arch_irq_work_raise();
	}

	preempt_enable();
}
EXPORT_SYMBOL_GPL(irq_work_queue);

bool irq_work_needs_cpu(void)
{
	struct llist_head *raised, *lazy;

	raised = &__get_cpu_var(raised_list);
	lazy = &__get_cpu_var(lazy_list);
	if (llist_empty(raised) && llist_empty(lazy))
		return false;

	/* All work should have been flushed before going offline */
	WARN_ON_ONCE(cpu_is_offline(smp_processor_id()));

	return true;
}

static void irq_work_run_list(struct llist_head *list)
{
	unsigned long flags;
	struct irq_work *work;
	struct llist_node *llnode;

	BUG_ON_NONRT(!irqs_disabled());

	if (llist_empty(list))
		return;

	llnode = llist_del_all(list);
	while (llnode != NULL) {
		work = llist_entry(llnode, struct irq_work, llnode);

		llnode = llist_next(llnode);

		/*
		 * Clear the PENDING bit, after this point the @work
		 * can be re-used.
		 * Make it immediately visible so that other CPUs trying
		 * to claim that work don't rely on us to handle their data
		 * while we are in the middle of the func.
		 */
		flags = work->flags & ~IRQ_WORK_PENDING;
		xchg(&work->flags, flags);

		work->func(work);
		/*
		 * Clear the BUSY bit and return to the free state if
		 * no-one else claimed it meanwhile.
		 */
		(void)cmpxchg(&work->flags, flags, flags & ~IRQ_WORK_BUSY);
	}
}

/*
 * Run the irq_work entries on this cpu. Requires to be ran from hardirq
 * context with local IRQs disabled.
 */
void irq_work_run(void)
{
	irq_work_run_list(this_cpu_ptr(&raised_list));
	if (IS_ENABLED(CONFIG_PREEMPT_RT_FULL)) {
		if (!llist_empty(this_cpu_ptr(&lazy_list)))
			raise_softirq(TIMER_SOFTIRQ);
	} else
		irq_work_run_list(this_cpu_ptr(&lazy_list));
}
EXPORT_SYMBOL_GPL(irq_work_run);

void irq_work_tick(void)
{
	struct llist_head *raised = this_cpu_ptr(&raised_list);

	if (!llist_empty(raised) && !arch_irq_work_has_interrupt())
		irq_work_run_list(raised);

	if (!IS_ENABLED(CONFIG_PREEMPT_RT_FULL))
		irq_work_run_list(this_cpu_ptr(&lazy_list));
}

#if defined(CONFIG_IRQ_WORK) && defined(CONFIG_PREEMPT_RT_FULL)
void irq_work_tick_soft(void)
{
	irq_work_run_list(this_cpu_ptr(&lazy_list));
}
#endif

/*
 * Synchronize against the irq_work @entry, ensures the entry is not
 * currently in use.
 */
void irq_work_sync(struct irq_work *work)
{
	WARN_ON_ONCE(irqs_disabled());

	while (work->flags & IRQ_WORK_BUSY)
		cpu_relax();
}
EXPORT_SYMBOL_GPL(irq_work_sync);

#ifdef CONFIG_HOTPLUG_CPU
static int irq_work_cpu_notify(struct notifier_block *self,
			       unsigned long action, void *hcpu)
{
	long cpu = (long)hcpu;

	switch (action) {
	case CPU_DYING:
		/* Called from stop_machine */
		if (WARN_ON_ONCE(cpu != smp_processor_id()))
			break;
		irq_work_tick();
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block cpu_notify;

static __init int irq_work_init_cpu_notifier(void)
{
	cpu_notify.notifier_call = irq_work_cpu_notify;
	cpu_notify.priority = 0;
	register_cpu_notifier(&cpu_notify);
	return 0;
}
device_initcall(irq_work_init_cpu_notifier);

#endif /* CONFIG_HOTPLUG_CPU */
