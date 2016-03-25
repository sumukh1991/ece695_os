#include <linux/latencytop.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/mempolicy.h>
#include <linux/migrate.h>
#include <linux/task_work.h>

#include <trace/events/sched.h>
#include <linux/sysfs.h>
#include <linux/vmalloc.h>

#include "sched.h"


static struct mycfsnode* mycfs_next(struct mycfsnode *node) {
  return node->next;
}

static void mycfs_erase(struct mycfsnode *node) {
	//return node->next;
}

/*
 * Targeted preemption latency for CPU-bound tasks:
 * (default: 6ms * (1 + ilog(ncpus)), units: nanoseconds)
 *
 * NOTE: this latency value is not the same as the concept of
 * 'timeslice length' - timeslices in CFS are of variable length
 * and have no persistent notion like in traditional, time-slice
 * based scheduling concepts.
 *
 * (to see the precise effective timeslice length of your workload,
 *  run vmstat and monitor the context-switches (cs) field)
 */
//CHECK THESE unsigned int sysctl_sched_latency = 6000000ULL;
//CHECK THESE unsigned int normalized_sysctl_sched_latency = 6000000ULL;
//CHECK THESE  
//CHECK THESE /*
//CHECK THESE  * The initial- and re-scaling of tunables is configurable
//CHECK THESE  * (default SCHED_TUNABLESCALING_LOG = *(1+ilog(ncpus))
//CHECK THESE  *
//CHECK THESE  * Options are:
//CHECK THESE  * SCHED_TUNABLESCALING_NONE - unscaled, always *1
//CHECK THESE  * SCHED_TUNABLESCALING_LOG - scaled logarithmical, *1+ilog(ncpus)
//CHECK THESE  * SCHED_TUNABLESCALING_LINEAR - scaled linear, *ncpus
//CHECK THESE  */
//CHECK THESE enum sched_tunable_scaling sysctl_sched_tunable_scaling
//CHECK THESE         = SCHED_TUNABLESCALING_LOG;
//CHECK THESE  
//CHECK THESE /*
//CHECK THESE  * Minimal preemption granularity for CPU-bound tasks:
//CHECK THESE  * (default: 0.75 msec * (1 + ilog(ncpus)), units: nanoseconds)
//CHECK THESE  */
//CHECK THESE unsigned int sysctl_sched_min_granularity = 750000ULL;
//CHECK THESE unsigned int normalized_sysctl_sched_min_granularity = 750000ULL;
//CHECK THESE  
//CHECK THESE /*
//CHECK THESE  * is kept at sysctl_sched_latency / sysctl_sched_min_granularity
//CHECK THESE  */
//CHECK THESE static unsigned int sched_nr_latency = 8;
//CHECK THESE  
//CHECK THESE /*
//CHECK THESE  * After fork, child runs first. If set to 0 (default) then
//CHECK THESE  * parent will (try to) run first.
//CHECK THESE  */
//CHECK THESE unsigned int sysctl_sched_child_runs_first __read_mostly;
//CHECK THESE  
//CHECK THESE /*
//CHECK THESE  * SCHED_OTHER wake-up granularity.
//CHECK THESE  * (default: 1 msec * (1 + ilog(ncpus)), units: nanoseconds)
//CHECK THESE  *
//CHECK THESE  * This option delays the preemption effects of decoupled workloads
//CHECK THESE  * and reduces their over-scheduling. Synchronous workloads will still
//CHECK THESE  * have immediate wakeup/sleep latencies.
//CHECK THESE  */
//CHECK THESE unsigned int sysctl_sched_wakeup_granularity = 1000000UL;
//CHECK THESE unsigned int normalized_sysctl_sched_wakeup_granularity = 1000000UL;sched_mycfs_entity {
//CHECK THESE  
//CHECK THESE const_debug unsigned int sysctl_sched_migration_cost = 500000UL;
//CHECK THESE  
//CHECK THESE /*
//CHECK THESE  * The exponential sliding  window over which load is averaged for shares
//CHECK THESE  * distribution.
//CHECK THESE  * (default: 10msec)
//CHECK THESE  */
//CHECK THESE unsigned int __read_mostly sysctl_sched_shares_window = 10000000UL;



//CHECK THESE  #if BITS_PER_LONG == 32
//CHECK THESE  # define WMULT_CONST	(~0UL)
//CHECK THESE  #else
//CHECK THESE  # define WMULT_CONST	(1UL << 32)
//CHECK THESE  #endif
//CHECK THESE   
//CHECK THESE  #define WMULT_SHIFT	32
//CHECK THESE   
//CHECK THESE  /*
//CHECK THESE   * Shift right and round:
//CHECK THESE   */
//CHECK THESE  #define SRR(x, y) (((x) + (1UL << ((y) - 1))) >> (y))

/*
 * delta *= weight / lw
 */
static unsigned long
calc_delta_mycfs(unsigned long delta_exec, unsigned long weight,
		struct load_weight *lw)
{
	u64 tmp;

//        /*
//         * weight can be less than 2^SCHED_LOAD_RESOLUTION for task group sched
//         * entities since MIN_SHARES = 2. Treat weight as 1 if less than
//         * 2^SCHED_LOAD_RESOLUTION.
//         */
//        if (likely(weight > (1UL << SCHED_LOAD_RESOLUTION)))
//        	tmp = (u64)delta_exec * scale_load_down(weight);
//        else
//        	tmp = (u64)delta_exec;
// 
//        if (!lw->inv_weight) {
//        	unsigned long w = scale_load_down(lw->weight);
// 
//        	if (BITS_PER_LONG > 32 && unlikely(w >= WMULT_CONST))
//        		lw->inv_weight = 1;
//        	else if (unlikely(!w))
//        		lw->inv_weight = WMULT_CONST;
//        	else
//        		lw->inv_weight = WMULT_CONST / w;
//        }
// 
//        /*
//         * Check whether we'd overflow the 64-bit multiplication:
//         */
//        if (unlikely(tmp > WMULT_CONST))
//        	tmp = SRR(SRR(tmp, WMULT_SHIFT/2) * lw->inv_weight,
//        		WMULT_SHIFT/2);
//        else
//        	tmp = SRR(tmp * lw->inv_weight, WMULT_SHIFT);

	return (unsigned long)min(tmp, (u64)(unsigned long)LONG_MAX);
}

const struct sched_class mycfs_sched_class;

static inline struct task_struct *mycfs_task_of(struct sched_mycfs_entity *se)
{
  return container_of(se, struct task_struct, se);
}

static inline struct rq *rq_of(struct mycfs_rq *cfs_rq)
{
  return container_of(cfs_rq, struct rq, cfs);
}

#define entity_is_task(se)	1

#define for_each_sched_entity(se) \
		for (; se; se = NULL)

static inline struct mycfs_rq *mycfs_task_cfs_rq(struct task_struct *p)
{
  return &task_rq(p)->mycfs;
}

static inline struct mycfs_rq *mycfs_rq_of(struct sched_mycfs_entity *se)
{
//  return se->mycfs_rq;
  struct task_struct *p = mycfs_task_of(se);
  struct rq *rq = task_rq(p);
  
  return &rq->mycfs;
}
static inline u64 max_vruntime(u64 max_vruntime, u64 vruntime)
{
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
}

static inline int entity_before(struct sched_mycfs_entity *a,
				struct sched_mycfs_entity *b)
{
}

static void update_min_vruntime(struct mycfs_rq *cfs_rq)
{
}

/*
 * Enqueue an entity into the rb-tree:
 */
static void __mycfs_enqueue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
	struct mycfsnode *link = &cfs_rq->leftmost;
	struct mycfsnode *parent = NULL;
	struct sched_mycfs_entity *entry;
	int leftmost = 1;

        printk(KERN_ALERT "DEBUG Inside enqueue\n");

	/*
	 * Find the right place in the rbtree:
	 */
	while (link) {
//        	parent = *link;
//        	entry = rb_entry(parent, struct sched_mycfs_entity, run_node);
//        	/*
//        	 * We dont care about collisions. Nodes with
//        	 * the same key stay together.
//        	 */
//        	if (entity_before(se, entry)) {
//        		link = &parent->rb_left;
//        	} else {
//        		link = &parent->rb_right;
//        		leftmost = 0;
//        	}

          link = link->next;
          //          if(link)
          leftmost = 0;
	}

	/*
	 * Maintain a cache of leftmost tree entries (it is frequently
	 * used):
	 */
	if (leftmost)
		cfs_rq->leftmost = &se->run_node;

//        rb_link_node(&se->run_node, parent, link);
//        rb_insert_color(&se->run_node, &cfs_rq->tasks_timeline);
        link = &se->run_node;
}

static void __mycfs_dequeue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
	if (cfs_rq->leftmost == &se->run_node) {
		struct mycfsnode *next_node;

		next_node = mycfs_next(&se->run_node);
		cfs_rq->leftmost = next_node;
	}
        printk(KERN_ALERT "Done moving to front of queue\n");
	mycfs_erase(&se->run_node);//, &cfs_rq->tasks_timeline);
}


/*
 * delta /= w
 */
static inline unsigned long
calc_delta_fair(unsigned long delta, struct sched_mycfs_entity *se)
{
//        if (unlikely(se->load.weight != NICE_0_LOAD))
//        	delta = calc_delta_mycfs(delta, NICE_0_LOAD, &se->load);

	return delta;
}

/*
 * We calculate the wall-time slice from the period by taking a part
 * proportional to the weight.
 *
 * s = p*P[w/rw]
 */
static u64 sched_slice(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
  u64 slice;
  return slice;
}

/*
 * We calculate the vruntime slice of a to-be-inserted task.
 *
 * vs = s/w
 */
static u64 sched_vslice(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
	return calc_delta_fair(sched_slice(cfs_rq, se), se);
}

/*
 * Update the current task's runtime statistics. Skip current tasks that
 * are not in our scheduling class.
 */
static inline void
__update_curr(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *curr,
	      unsigned long delta_exec)
{
//        unsigned long delta_exec_weighted;
// 
//        schedstat_set(curr->statistics.exec_max,
//        	      max((u64)delta_exec, curr->statistics.exec_max));
// 
//        curr->sum_exec_runtime += delta_exec;
//        schedstat_add(cfs_rq, exec_clock, delta_exec);
//        delta_exec_weighted = calc_delta_fair(delta_exec, curr);
// 
//        curr->vruntime += delta_exec_weighted;
//        update_min_vruntime(cfs_rq);
}

static void update_curr(struct mycfs_rq *cfs_rq)
{
//        struct sched_mycfs_entity *curr = cfs_rq->curr;
//        u64 now = rq_of(cfs_rq)->clock_task;
//        unsigned long delta_exec;
// 
//        if (unlikely(!curr))
//        	return;
// 
//        /*
//         * Get the amount of time the current task was running
//         * since the last time we changed load (this cannot
//         * overflow on 32 bits):
//         */
//        delta_exec = (unsigned long)(now - curr->exec_start);
//        if (!delta_exec)
//        	return;
// 
//        __update_curr(cfs_rq, curr, delta_exec);
//        curr->exec_start = now;
// 
//        if (entity_is_task(curr)) {
//        	struct task_struct *curtask = task_of(curr);
// 
//        	trace_sched_stat_runtime(curtask, delta_exec, curr->vruntime);
//        	cpuacct_charge(curtask, delta_exec);
//        	account_group_exec_runtime(curtask, delta_exec);
//        }
// 
//        account_cfs_rq_runtime(cfs_rq, delta_exec);
}





static void
place_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se, int initial)
{
//        u64 vruntime = cfs_rq->min_vruntime;
// 
//        /*
//         * The 'current' period is already promised to the current tasks,
//         * however the extra weight of the new task will slow them down a
//         * little, place the new task so that it fits in the slot that
//         * stays open at the end.
//         */
//        if (initial && sched_feat(START_DEBIT))
//        	vruntime += sched_vslice(cfs_rq, se);
// 
//        /* sleeps up to a single latency don't count. */
//        if (!initial) {
//        	unsigned long thresh = sysctl_sched_latency;
// 
//        	/*
//        	 * Halve their sleep time's effect, to allow
//        	 * for a gentler effect of sleepers:
//        	 */
//        	if (sched_feat(GENTLE_FAIR_SLEEPERS))
//        		thresh >>= 1;
// 
//        	vruntime -= thresh;
//        }
// 
//        /* ensure we never gain time by being placed backwards. */
//        se->vruntime = max_vruntime(se->vruntime, vruntime);
}

static void
dequeue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se, int flags)
{
//        /*
//         * Update run-time statistics of the 'current'.
//         */
//        update_curr(cfs_rq);
//        dequeue_entity_load_avg(cfs_rq, se, flags & DEQUEUE_SLEEP);
// 
//        update_stats_dequeue(cfs_rq, se);
//        if (flags & DEQUEUE_SLEEP) {
//#ifdef CONFIG_SCHEDSTATS
//        	if (entity_is_task(se)) {
//        		struct task_struct *tsk = task_of(se);
// 
//        		if (tsk->state & TASK_INTERRUPTIBLE)
//        			se->statistics.sleep_start = rq_of(cfs_rq)->clock;
//        		if (tsk->state & TASK_UNINTERRUPTIBLE)
//        			se->statistics.block_start = rq_of(cfs_rq)->clock;
//        	}
//#endif
//        }
// 
//        clear_buddies(cfs_rq, se);
// 
	if (se != cfs_rq->curr)
		__mycfs_dequeue_entity(cfs_rq, se);
        se->on_rq = 0;
//        account_entity_dequeue(cfs_rq, se);
// 
//        /*
//         * Normalize the entity after updating the min_vruntime because the
//         * update can refer to the ->curr item and we need to reflect this
//         * movement in our normalized position.
//         */
//        if (!(flags & DEQUEUE_SLEEP))
//        	se->vruntime -= cfs_rq->min_vruntime;
// 
//        /* return excess runtime on last dequeue */
//        return_cfs_rq_runtime(cfs_rq);
// 
//        update_min_vruntime(cfs_rq);
//        update_cfs_shares(cfs_rq);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void
check_preempt_tick(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *curr)
{
//        unsigned long ideal_runtime, delta_exec;
//        struct sched_mycfs_entity *se;
//        s64 delta;
// 
//        ideal_runtime = sched_slice(cfs_rq, curr);
//        delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
//        if (delta_exec > ideal_runtime) {
//        	resched_task(rq_of(cfs_rq)->curr);
//        	/*
//        	 * The current task ran long enough, ensure it doesn't get
//        	 * re-elected due to buddy favours.
//        	 */
//        	clear_buddies(cfs_rq, curr);
//        	return;
//        }
// 
//        /*
//         * Ensure that a task that missed wakeup preemption by a
//         * narrow margin doesn't have to wait for a full slice.
//         * This also mitigates buddy induced latencies under load.
//         */
//        if (delta_exec < sysctl_sched_min_granularity)
//        	return;
// 
//        se = __pick_first_entity(cfs_rq);
//        delta = curr->vruntime - se->vruntime;
// 
//        if (delta < 0)
//        	return;
// 
//        if (delta > ideal_runtime)
//        	resched_task(rq_of(cfs_rq)->curr);
}

static void
set_next_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
	/* 'current' is not kept within the tree. */
	if (se->on_rq) {
		/*
		 * Any task has to be enqueued before it get to execute on
		 * a CPU. So account for the time it spent waiting on the
		 * runqueue.
		 */
          //		update_stats_wait_end(cfs_rq, se);
		__mycfs_dequeue_entity(cfs_rq, se);
                //		update_entity_load_avg(se, 1);
	}
// 
//        update_stats_curr_start(cfs_rq, se);
        cfs_rq->curr = se;
//#ifdef CONFIG_SCHEDSTATS
//        /*
//         * Track our maximum slice length, if the CPU's load is at
//         * least twice that of our own weight (i.e. dont track it
//         * when there are only lesser-weight tasks around):
//         */
//        if (rq_of(cfs_rq)->load.weight >= 2*se->load.weight) {
//        	se->statistics.slice_max = max(se->statistics.slice_max,
//        		se->sum_exec_runtime - se->prev_sum_exec_runtime);
//        }
//#endif
//        se->prev_sum_exec_runtime = se->sum_exec_runtime;
}



static void put_prev_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *prev)
{
	/*
	 * If still on the runqueue then deactivate_task()
	 * was not called and update_curr() has to be done:
	 */
	if (prev->on_rq)
          update_curr(cfs_rq);
        
//        /* throttle cfs_rqs exceeding runtime */
//        check_cfs_rq_runtime(cfs_rq);
 
//	check_spread(cfs_rq, prev);
	if (prev->on_rq) {
          //		update_stats_wait_start(cfs_rq, prev);
		/* Put 'current' back into the tree. */
		__mycfs_enqueue_entity(cfs_rq, prev);
		/* in !on_rq case, update occurred at dequeue */
                //		update_entity_load_avg(prev, 1);
	}
	cfs_rq->curr = NULL;
}

static void
entity_tick(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *curr, int queued)
{
//        /*
//         * Update run-time statistics of the 'current'.
//         */
//        update_curr(cfs_rq);
// 
//        /*
//         * Ensure that runnable average is periodically updated.
//         */
//        update_entity_load_avg(curr, 1);
//        update_cfs_rq_blocked_load(cfs_rq, 1);
//        update_cfs_shares(cfs_rq);
// 
//        if (cfs_rq->nr_running > 1)
//        	check_preempt_tick(cfs_rq, curr);
}


static unsigned long
wakeup_gran_mycfs(struct sched_mycfs_entity *curr, struct sched_mycfs_entity *se)
{
	unsigned long gran = sysctl_sched_wakeup_granularity;

	/*
	 * Since its curr running now, convert the gran from real-time
	 * to virtual-time in his units.
	 *
	 * By using 'se' instead of 'curr' we penalize light tasks, so
	 * they get preempted easier. That is, if 'se' < 'curr' then
	 * the resulting gran will be larger, therefore penalizing the
	 * lighter, if otoh 'se' > 'curr' then the resulting gran will
	 * be smaller, again penalizing the lighter task.
	 *
	 * This is especially important for buddies when the leftmost
	 * task is higher priority than the buddy.
	 */
	return calc_delta_fair(gran, se);
}

/*
 * Should 'se' preempt 'curr'.
 *
 *             |s1
 *        |s2
 *   |s3
 *         g
 *      |<--->|c
 *
 *  w(c, s1) = -1
 *  w(c, s2) =  0
 *  w(c, s3) =  1
 *
 */
static int
wakeup_preempt_entity_mycfs(struct sched_mycfs_entity *curr, struct sched_mycfs_entity *se)
{
	s64 gran, vdiff = curr->vruntime - se->vruntime;

	if (vdiff <= 0)
		return -1;

//        gran = wakeup_gran_mycfs(curr, se);
//        if (vdiff > gran)
//        	return 1;

	return 0;
}

__init void init_sched_mycfs_class(void)
{

}

void init_mycfs_rq(struct mycfs_rq *cfs_rq)
{
	struct mycfsnode a;
	cfs_rq->leftmost = &a; 
	printk(KERN_ALERT "Debug: inside init_mycfs_rq\n");
	//	cfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

static void
mycfs_enqueue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se, int flags)
{
	/*
	 * Update the normalized vruntime before updating min_vruntime
	 * through callig update_curr().
	 */
//        if (!(flags & ENQUEUE_WAKEUP) || (flags & ENQUEUE_WAKING))
//        	se->vruntime += cfs_rq->min_vruntime;
// 
//        if (flags & ENQUEUE_WAKEUP) {
//        	place_entity(cfs_rq, se, 0);
//        	enqueue_sleeper(cfs_rq, se);
//        }

	if (se != cfs_rq->curr)
		__mycfs_enqueue_entity(cfs_rq, se);
	se->on_rq = 1;

//        if (cfs_rq->nr_running == 1) {
//          //		list_add_leaf_cfs_rq(cfs_rq);
//          //		check_enqueue_throttle(cfs_rq);
//        }
}


/*
 * The enqueue_task method is called before nr_running is
 * increased. Here we update the fair scheduling stats and
 * then put the task into the rbtree:
 */
static void
enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &p->mycfs_se;
	printk(KERN_ALERT "Inside enqueue_task_mycfs function\n"); 
	for_each_sched_entity(se) {
		printk(KERN_ALERT "Inside...\n"); 
		if (se->on_rq)
			break;
		cfs_rq = mycfs_rq_of(se);
		mycfs_enqueue_entity(cfs_rq, se, flags);
 
		/*
		 * end evaluation on encountering a throttled cfs_rq
		 *
		 * note: in the case of encountering a throttled cfs_rq we will
		 * post the final h_nr_running increment below.
		*/
//        	if (cfs_rq_throttled(cfs_rq))
//        		break;
//        	cfs_rq->h_nr_running++;
// 
//        	flags = ENQUEUE_WAKEUP;
	}
// 
//        for_each_sched_entity(se) {
//        	cfs_rq = cfs_rq_of(se);
//        	cfs_rq->h_nr_running++;
// 
//        	if (cfs_rq_throttled(cfs_rq))
//        		break;
// 
//        	update_cfs_shares(cfs_rq);
//        	update_entity_load_avg(se, 1);
//        }
// 
        if (!se) {
//        	update_rq_runnable_avg(rq, rq->nr_running);
        	inc_nr_running(rq);
        }
//        hrtick_update(rq);
}

/*
 * The dequeue_task method is called before nr_running is
 * decreased. We remove the task from the rbtree and
 * update the fair scheduling stats:
 */
static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &p->se;
	int task_sleep = flags & DEQUEUE_SLEEP;
 
	for_each_sched_entity(se) {
		cfs_rq = mycfs_rq_of(se);
		dequeue_entity(cfs_rq, se, flags);
// 
//        	/*
//        	 * end evaluation on encountering a throttled cfs_rq
//        	 *
//        	 * note: in the case of encountering a throttled cfs_rq we will
//        	 * post the final h_nr_running decrement below.
//        	*/
//        	if (cfs_rq_throttled(cfs_rq))
//        		break;
//        	cfs_rq->h_nr_running--;
// 
//        	/* Don't dequeue parent if it has other entities besides us */
//        	if (cfs_rq->load.weight) {
//        		/*
//        		 * Bias pick_next to pick a task from this cfs_rq, as
//        		 * p is sleeping when it is within its sched_slice.
//        		 */
//        		if (task_sleep && parent_entity(se))
//        			set_next_buddy(parent_entity(se));
// 
//        		/* avoid re-evaluating load for this entity */
//        		se = parent_entity(se);
//        		break;
//        	}
//        	flags |= DEQUEUE_SLEEP;
        }
// 
//        for_each_sched_entity(se) {
//        	cfs_rq = cfs_rq_of(se);
//        	cfs_rq->h_nr_running--;
// 
//        	if (cfs_rq_throttled(cfs_rq))
//        		break;
// 
//        	update_cfs_shares(cfs_rq);
//        	update_entity_load_avg(se, 1);
//        }
// 
	if (!se) {
		dec_nr_running(rq);
//        	update_rq_runnable_avg(rq, 1);
        }
//        hrtick_update(rq);
}

//NEEDS TO BE FIXED
/*
 * sched_yield() is very simple
 *
 * The magic of dealing with the ->skip buddy is in pick_next_entity_mycfs.
 */
static void yield_task_mycfs(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct mycfs_rq *cfs_rq = mycfs_task_cfs_rq(curr);
	struct sched_mycfs_entity *se = &curr->se;
 
	/*
	 * Are we the only task in the tree?
	 */
	if (unlikely(rq->nr_running == 1))
		return;
// 
//        clear_buddies(cfs_rq, se);
// 
//        if (curr->policy != SCHED_BATCH) {
//        	update_rq_clock(rq);
//        	/*
//        	 * Update run-time statistics of the 'current'.
//        	 */
//        	update_curr(cfs_rq);
//        	/*
//        	 * Tell update_rq_clock() that we've just updated,
//        	 * so we don't do microscopic update in schedule()
//        	 * and double the fastpath cost.
//        	 */
//        	 rq->skip_clock_update = 1;
//        }
// 
//        set_skip_buddy(se);
}

static bool yield_to_task_mycfs(struct rq *rq, struct task_struct *p, bool preempt)
{
        struct sched_mycfs_entity *se = &p->se;
// 
//        /* throttled hierarchies are not runnable */
	if (!se->on_rq)// || throttled_hierarchy(cfs_rq_of(se)))
		return false;
// 
//        /* Tell the scheduler that we'd really like pse to run next. */
//        set_next_buddy(se);
// 
        yield_task_mycfs(rq);
// 
        return true;
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_wakeup_mycfs(struct rq *rq, struct task_struct *p, int wake_flags)
{
	struct task_struct *curr = rq->curr;
	struct sched_mycfs_entity *se = &curr->se, *pse = &p->se;
	struct mycfs_rq *cfs_rq = mycfs_task_cfs_rq(curr);
        //FIX	int scale = cfs_rq->nr_running >= sched_nr_latency;
	int next_buddy_marked = 0;
 
	if (unlikely(se == pse))
		return;
// 
//        /*
//         * This is possible from callers such as move_task(), in which we
//         * unconditionally check_prempt_curr() after an enqueue (which may have
//         * lead to a throttle).  This both saves work and prevents false
//         * next-buddy nomination below.
//         */
//        if (unlikely(throttled_hierarchy(cfs_rq_of(pse))))
//        	return;
// 
//        if (sched_feat(NEXT_BUDDY) && scale && !(wake_flags & WF_FORK)) {
//        	set_next_buddy(pse);
//        	next_buddy_marked = 1;
//        }
// 
//        /*
//         * We can come here with TIF_NEED_RESCHED already set from new task
//         * wake up path.
//         *
//         * Note: this also catches the edge-case of curr being in a throttled
//         * group (e.g. via set_curr_task), since update_curr() (in the
//         * enqueue of curr) will have resulted in resched being set.  This
//         * prevents us from potentially nominating it as a false LAST_BUDDY
//         * below.
//         */
	if (test_tsk_need_resched(curr))
		return;
 
	/* Idle tasks are by definition preempted by non-idle tasks. */
	if (unlikely(curr->policy == SCHED_IDLE) &&
	    likely(p->policy != SCHED_IDLE))
		goto preempt;
 
	/*
	 * Batch and idle tasks do not preempt non-idle tasks (their preemption
	 * is driven by the tick):
	 */
	if (unlikely(p->policy != SCHED_NORMAL) || !sched_feat(WAKEUP_PREEMPTION))
		return;
 
        //FIX	find_matching_se(&se, &pse);
	update_curr(mycfs_rq_of(se));
	BUG_ON(!pse);
//        if (wakeup_preempt_entity_mycfs(se, pse) == 1) {
//        	/*
//        	 * Bias pick_next to pick the sched entity that is
//        	 * triggering this preemption.
//        	 */
//        	if (!next_buddy_marked)
//        		set_next_buddy(pse);
//        	goto preempt;
//        }
 
	return;
 
preempt:
	resched_task(curr);
	/*
	 * Only set the backward buddy when the current task is still
	 * on the rq. This can happen when a wakeup gets interleaved
	 * with schedule on the ->pre_schedule() or idle_balance()
	 * point, either of which can * drop the rq lock.
	 *
	 * Also, during early boot the idle thread is in the fair class,
	 * for obvious reasons its a bad idea to schedule back to it.
	 */
	if (unlikely(!se->on_rq || curr == rq->idle))
		return;
 
//FIX         if (sched_feat(LAST_BUDDY) && scale && entity_is_task(se))
//FIX         	set_last_buddy(se);
}


/*
 * Account for a descheduled task:
 */
static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{
	struct sched_mycfs_entity *se = &prev->se;
	struct mycfs_rq *cfs_rq;
 
	for_each_sched_entity(se) {
		cfs_rq = mycfs_rq_of(se);
		put_prev_entity(cfs_rq, se);
	}
}



/* Account for a task changing its policy or group.
 *
 * This routine is mostly called to set cfs_rq->curr field when a task
 * migrates between groups/classes.
 */
static void set_curr_task_mycfs(struct rq *rq)
{
  //  printk(KERN_ALERT "DEBUG In set_curr_task_mycfs\n");
	struct sched_mycfs_entity *se = &rq->curr->se;
 
	for_each_sched_entity(se) {
          printk(KERN_ALERT "DEBUG before in  set_curr_task_mycfs\n");
          struct mycfs_rq *cfs_rq = mycfs_rq_of(se);
          printk(KERN_ALERT "DEBUG  after in set_curr_task_mycfs\n");
 
          set_next_entity(cfs_rq, se);
		/* ensure bandwidth has been allocated on our new cfs_rq */
                //account_cfs_rq_runtime(cfs_rq, 0);
	}
}


/*
 * scheduler tick hitting a task of our scheduling class:
 */
static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &curr->se;
 
	for_each_sched_entity(se) {
		cfs_rq = mycfs_rq_of(se);
		entity_tick(cfs_rq, se, queued);
	}
 
//        if (sched_feat_numa(NUMA))
//        	task_tick_numa(rq, curr);
 
//	update_rq_runnable_avg(rq, 1);
}


/*
 * called on fork with the child task as argument from the parent's context
 *  - child not yet on the tasklist
 *  - preemption disabled
 */
static void task_fork_mycfs(struct task_struct *p)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &p->se, *curr;
	int this_cpu = smp_processor_id();
	struct rq *rq = this_rq();
	unsigned long flags;
 
	raw_spin_lock_irqsave(&rq->lock, flags);
 
	update_rq_clock(rq);
 
	cfs_rq = mycfs_task_cfs_rq(current);
	curr = cfs_rq->curr;
 
	/*
	 * Not only the cpu but also the task_group of the parent might have
	 * been changed after parent->se.parent,cfs_rq were copied to
	 * child->se.parent,cfs_rq. So call __set_task_cpu() to make those
	 * of child point to valid ones.
	 */
	rcu_read_lock();
	__set_task_cpu(p, this_cpu);
	rcu_read_unlock();
 
	update_curr(cfs_rq);
 
	if (curr)
          se->vruntime = curr->vruntime;
	place_entity(cfs_rq, se, 1);
 
	if (sysctl_sched_child_runs_first && curr && entity_before(curr, se)) {
		/*
		 * Upon rescheduling, sched_class::put_prev_task() will place
		 * 'current' within the tree based on its new key value.
		 */
		swap(curr->vruntime, se->vruntime);
		resched_task(rq->curr);
	}
 
        //FIX	se->vruntime -= cfs_rq->min_vruntime;
 
	raw_spin_unlock_irqrestore(&rq->lock, flags);
}


/*
 * Priority of the task has changed. Check to see if we preempt
 * the current task.
 */
static void
prio_changed_mycfs(struct rq *rq, struct task_struct *p, int oldprio)
{
//        if (!p->se.on_rq)
//        	return;
// 
//        /*
//         * Reschedule if we are currently running on this runqueue and
//         * our priority decreased, or if we are not currently running on
//         * this runqueue and our priority is higher than the current's
//         */
//        if (rq->curr == p) {
//        	if (p->prio > oldprio)
//        		resched_task(rq->curr);
//        } else
//        	check_preempt_curr(rq, p, 0);
}

static void switched_from_mycfs(struct rq *rq, struct task_struct *p)
{
//        struct sched_mycfs_entity *se = &p->se;
//        struct mycfs_rq *cfs_rq = cfs_rq_of(se);
// 
//        /*
//         * Ensure the task's vruntime is normalized, so that when it's
//         * switched back to the fair class the enqueue_entity(.flags=0) will
//         * do the right thing.
//         *
//         * If it's on_rq, then the dequeue_entity(.flags=0) will already
//         * have normalized the vruntime, if it's !on_rq, then only when
//         * the task is sleeping will it still have non-normalized vruntime.
//         */
//        if (!p->on_rq && p->state != TASK_RUNNING) {
//        	/*
//        	 * Fix up our vruntime so that the current sleep doesn't
//        	 * cause 'unlimited' sleep bonus.
//        	 */
//        	place_entity(cfs_rq, se, 0);
//        	se->vruntime -= cfs_rq->min_vruntime;
//        }

}


/*
 * We switched to the sched_mycfs class.
 */
static void switched_to_mycfs(struct rq *rq, struct task_struct *p)
{
	if (!p->se.on_rq)
		return;
 
	/*
	 * We were most likely switched from sched_rt, so
	 * kick off the schedule if running, otherwise just see
	 * if we can still preempt the current task.
	 */
	if (rq->curr == p)
		resched_task(rq->curr);
	else
		check_preempt_curr(rq, p, 0);
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
        struct sched_mycfs_entity *se = &task->se;
        unsigned int rr_interval = 0;
// 
//        /*
//         * Time slice is 0 for SCHED_OTHER tasks that are on an otherwise
//         * idle runqueue:
//         */
//        if (rq->cfs.load.weight)
//        	rr_interval = NS_TO_JIFFIES(sched_slice(mycfs_rq_of(se), se));
// 
        return rr_interval;
}

struct sched_mycfs_entity *__pick_first_entity_mycfs(struct mycfs_rq *cfs_rq)
{
  struct mycfsnode *left = cfs_rq->leftmost;

	if (!left)
		return NULL;

	return container_of(left, struct sched_mycfs_entity, run_node);
}

static struct sched_mycfs_entity *__pick_next_entity_mycfs(struct sched_mycfs_entity *se)
{
	struct mycfsnode *next = mycfs_next(&se->run_node);
 
	  if (!next)
		return NULL;
 
	return container_of(next, struct sched_mycfs_entity, run_node);
}
/*
 * Pick the next process, keeping these things in mind, in this order:
 * 1) keep things fair between processes/task groups
 * 2) pick the "next" process, since someone really wants that to run
 * 3) pick the "last" process, for cache locality
 * 4) do not run the "skip" process, if something else is available
 */
static struct sched_mycfs_entity *pick_next_entity_mycfs(struct mycfs_rq *cfs_rq)
{
        struct sched_mycfs_entity *se = __pick_first_entity_mycfs(cfs_rq);
        struct sched_mycfs_entity *left = se;


	printk(KERN_ALERT "Debug: Inside pick_next_entity_mycfs\n");
// 
//        /*
//         * Avoid running the skip buddy, if running something else can
//         * be done without getting too unfair.
//         */
//        if (cfs_rq->skip == se) {
//        	struct sched_mycfs_entity *second = __pick_next_entity_mycfs(se);
//        	if (second && wakeup_preempt_entity_mycfs(second, left) < 1)
//        		se = second;
//        }
// 
//        /*
//         * Prefer last buddy, try to return the CPU to a preempted task.
//         */
//        if (cfs_rq->last && wakeup_preempt_entity_mycfs(cfs_rq->last, left) < 1)
//        	se = cfs_rq->last;
// 
//        /*
//         * Someone really wants this to run. If it's not unfair, run it.
//         */
//        if (cfs_rq->next && wakeup_preempt_entity_mycfs(cfs_rq->next, left) < 1)
//        	se = cfs_rq->next;
// 
//        clear_buddies(cfs_rq, se);
	if (cfs_rq->next)// && wakeup_preempt_entity_mycfs(cfs_rq->next, left) < 1)
          se = cfs_rq->next;
	
	printk(KERN_ALERT "Debug: Returned from pick_next_entity_mycfs\n");

	return se;
}


static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	struct task_struct *p;
        struct mycfs_rq *cfs_rq = &rq->mycfs;
        struct sched_mycfs_entity *se;
 

	if (!cfs_rq->leftmost)
		return NULL;
 
        do {
        	se = pick_next_entity_mycfs(cfs_rq);
        	set_next_entity(cfs_rq, se);
                //        	cfs_rq = group_cfs_rq(se);
        } while (cfs_rq);
// 
	p = mycfs_task_of(se);
//        if (hrtick_enabled(rq))
//        	hrtick_start_fair(rq, p);

        printk(KERN_ALERT "DEBUG In pick next task, returning non null\n");

	return p;
}



/*
 * All the scheduling class methods:
 */
const struct sched_class mycfs_sched_class = {
	.next			= &idle_sched_class,
	.enqueue_task		= enqueue_task_mycfs,
	.dequeue_task		= dequeue_task_mycfs,
	.yield_task		= yield_task_mycfs,
	.yield_to_task		= yield_to_task_mycfs,

	.check_preempt_curr	= check_preempt_wakeup_mycfs,

	.pick_next_task		= pick_next_task_mycfs,
	.put_prev_task		= put_prev_task_mycfs,

	.set_curr_task          = set_curr_task_mycfs,
	.task_tick		= task_tick_mycfs,
	.task_fork		= task_fork_mycfs,

	.prio_changed		= prio_changed_mycfs,
	.switched_from		= switched_from_mycfs,
	.switched_to		= switched_to_mycfs,

	.get_rr_interval	= get_rr_interval_mycfs,
};

