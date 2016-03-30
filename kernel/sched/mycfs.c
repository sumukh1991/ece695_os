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

const struct sched_class mycfs_sched_class;

static inline struct task_struct *mycfs_task_of(struct sched_mycfs_entity *se)
{
  return container_of(se, struct task_struct, mycfs_se);
}

static inline struct rq *rq_of(struct mycfs_rq *cfs_rq)
{
  return container_of(cfs_rq, struct rq, mycfs);
}


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


struct sched_mycfs_entity *__pick_first_entity_mycfs(struct mycfs_rq *cfs_rq)
{
  struct mycfsnode *left = cfs_rq->leftmost;

  printk(KERN_ALERT "Done entering __pick_first_entity_mycfs \n");
	if (!left)
		return NULL;

	return container_of(left, struct sched_mycfs_entity, run_node);
}

/*
 * Enqueue an entity into the rb-tree:
 */
static void __mycfs_enqueue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
	struct mycfsnode *link = cfs_rq->leftmost;
	struct mycfsnode *link_prev = cfs_rq->leftmost;
        struct sched_mycfs_entity *tse;
        //	struct mycfsnode *parent = NULL;
        //	struct sched_mycfs_entity *entry;
	int leftmost = 1;
        int inserted = 0;

        printk(KERN_ALERT "DEBUG INSERT initial link is %x, se gruntime is %lx, limit = %d, glimit is %d ,\n",(unsigned int)link,(unsigned long)se->gruntime,se->limit,mycfs_task_of(se)->glimit);
        if(link)
          printk(KERN_ALERT "DEBUG INSERT initial link->next is %x\n",(unsigned int)link->next);

	/*
	 * Find the right place in the rbtree:
	 */
	while (link) {
          tse = container_of(link, struct sched_mycfs_entity, run_node);
          printk(KERN_ALERT "DEBUG INSERT initial link is %x, se gruntime is %lx, limit = %d , se = %x\n",(unsigned int)link,(unsigned long)se->gruntime,se->limit, (unsigned int)tse);
          printk(KERN_ALERT "DEBUG INSERT in loop link->next is %x, run time comparision corresponds to %lx > %lx\n",(unsigned int)link->next,(unsigned long)tse->gruntime,(unsigned long)se->gruntime);
	  if(tse->gruntime >= se->gruntime)
	    {
              if(leftmost)
                break;
              link_prev->next = &se->run_node;
              se->run_node.next = link;
              leftmost = 0;
              inserted = 1;
              break;
            }//	      if(link)
          link_prev = link;
          link = link->next;
          leftmost = 0;
	}

//        printk(KERN_ALERT "DEBUG INSERT out of while statement\n");
        printk(KERN_ALERT "DEBUG INSERT out of loop not, inserted = %d, leftmost = %d, prev link is %x\n",inserted,leftmost,(unsigned int)link_prev);

	/*
	 * Maintain a cache of leftmost tree entries (it is frequently
	 * used):
	 */
	if (leftmost){
          //          printk(KERN_ALERT "DEBUG INSERT near end of while statement\n");
          printk(KERN_ALERT "DEBUG INSERT near end prev link is %x\n",(unsigned int)link_prev);
          if(cfs_rq->leftmost)
            se->run_node.next= cfs_rq->leftmost;
          else {
            se->run_node.next= NULL;
          }

          cfs_rq->leftmost = &se->run_node;
          inserted = 1;
          printk(KERN_ALERT "DEBUG INSERT near end leftmost is now %x\n",(unsigned int)cfs_rq->leftmost);
        }

//        else {
//          link_prev->next = &se->run_node;
//          printk(KERN_ALERT "DEBUG MYCFS near end of while in else statement\n");
//          printk(KERN_ALERT "DEBUG MYCFS near end prev in else link is %x\n",(unsigned int)link_prev);
//          se->run_node.next=NULL;
//        }

	if(inserted == 0) {
	  link_prev->next = &se->run_node;
	  printk(KERN_ALERT "DEBUG INSERT near end prev in else link prev is %x and new link added is %x\n",(unsigned int)link_prev, (unsigned int)&se->run_node);
	  se->run_node.next=NULL;
	}

        cfs_rq->nr_running++;
//        rb_link_node(&se->run_node, parent, link);
//        rb_insert_color(&se->run_node, &cfs_rq->tasks_timeline);
}

static void __mycfs_dequeue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
  printk(KERN_ALERT "Done entering dequeue entity \n");
	if (cfs_rq->leftmost == &se->run_node) {
		struct mycfsnode *next_node;

		next_node = mycfs_next(&se->run_node);
		cfs_rq->leftmost = next_node;
                printk(KERN_ALERT "DEBUG MYCFS Done moving to front of queue\n");
	}
        printk(KERN_ALERT "Done moving to front of queue\n");
	mycfs_erase(&se->run_node);//, &cfs_rq->tasks_timeline);
        cfs_rq->nr_running--;
}


static void update_curr(struct mycfs_rq *cfs_rq)
{
	struct sched_mycfs_entity *curr = cfs_rq->curr;
	u64 now = cpu_rq(0)->clock_task;
	unsigned long delta;
 
	if (unlikely(!curr))
		return;
 
	/*
	 * Get the amount of time the current task was running
	 * since the last time we changed load (this cannot
	 * overflow on 32 bits):
	 */
	delta = (unsigned long)(now - curr->exec_start)/100;
        //ECE695
        curr->gruntime++;

	if (!delta)
		return;
 
	//__update_curr(cfs_rq, curr, delta_exec);
	curr->sum_exec_runtime += delta;
//        schedstat_add(cfs_rq, exec_clock, delta_exec);
//        delta_exec_weighted = calc_delta_fair(delta_exec, curr);
 
	curr->vruntime += delta;
	curr->exec_start = now;
 
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
  printk(KERN_ALERT "Done entering higher dequeue entity \n");
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


static void
set_next_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
  printk(KERN_ALERT "Done entering set next entity \n");
	/* 'current' is not kept within the tree. */
	if (se->on_rq) {
          /*
           * Any task has to be enqueued before it get to execute on
           * a CPU. So account for the time it spent waiting on the
           * runqueue.
           */
          __mycfs_dequeue_entity(cfs_rq, se);
	}

        cfs_rq->curr = se;
        se->prev_gruntime = se->gruntime;
}



static void put_prev_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *prev)
{
  printk(KERN_ALERT "Done entering put prev entity \n");

	if (prev->on_rq) {
		/* Put 'current' back into the tree. */
          __mycfs_enqueue_entity(cfs_rq, prev);
//FIFO          prev->run_node.next = cfs_rq->leftmost;
//FIFO          cfs_rq->leftmost = &prev->run_node;
          printk(KERN_ALERT "DEBUG PREV_ENTITY is %x and its vruntime = %lx \n",(unsigned int) prev,(unsigned long)prev->vruntime);
        }
	
        cfs_rq->curr = NULL;
}



/*
 * Preempt the current task with a newly woken task if needed:
 */
static void
check_preempt_tick_mycfs(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *curr)
{
  unsigned long ideal_runtime, delta_exec,grt;
	struct sched_mycfs_entity *se;
	long int delta;
        
	//ideal_runtime = sched_slice(cfs_rq, curr);
	delta_exec = curr->gruntime - curr->prev_gruntime;
        printk(KERN_ALERT "DEBUG LIMIT task's runtime is %ld and delta is %ld, prev is %ld, calc delta is",curr->gruntime,delta_exec,curr->prev_gruntime,(curr->gruntime -curr->prev_gruntime) );
	if (curr->gruntime > curr->limit) {
          printk(KERN_ALERT "DEBUG PREEMPT preempted task due to limit exceeded\n");
	  resched_task(rq_of(cfs_rq)->curr);
		/*
		 * The current task ran long enough, ensure it doesn't get
		 * re-elected due to buddy favours.
		 */
	  //clear_buddies(cfs_rq, curr);
		return;
	}
 
	/*
	 * Ensure that a task that missed wakeup preemption by a
	 * narrow margin doesn't have to wait for a full slice.
	 * This also mitigates buddy induced latencies under load.
	 */
//	  if (delta_exec < )
//		return;
 
	se = __pick_first_entity_mycfs(cfs_rq);
	delta = curr->gruntime - se->gruntime;
 
	if (delta < 0)
		return;
 
        if (delta > 10) {//FIX
          printk(KERN_ALERT "DEBUG PREEMPT preempted task due to unfair run time\n");
          resched_task(rq_of(cfs_rq)->curr);
        }
}


static void
entity_tick(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *curr, int queued)
{
//        update_curr(cfs_rq);
// 
//         * Ensure that runnable average is periodically updated.
//        update_entity_load_avg(curr, 1);
//        update_cfs_rq_blocked_load(cfs_rq, 1);
//        update_cfs_shares(cfs_rq);
// 
//        if (cfs_rq->nr_running > 1)
//        	check_preempt_tick(cfs_rq, curr);
}


__init void init_sched_mycfs_class(void)
{

}

void init_mycfs_rq(struct mycfs_rq *cfs_rq)
{
	struct mycfsnode a;
	cfs_rq->leftmost = NULL; 
        cfs_rq->nr_running =0;
	printk(KERN_ALERT "Debug: inside init_mycfs_rq leftmost = %x\n",(unsigned int) cfs_rq->leftmost);
        //	printk(KERN_ALERT "Debug: inside init_mycfs_rq leftmost = %x\n",cfs_rq->leftmost->next);
	//	cfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

static void
mycfs_enqueue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se, int flags)
{
  printk(KERN_ALERT "Done entering enqueue entity \n");
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


static void
enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &p->mycfs_se;
//        __attribute__((optimize(0))) volatile struct task_struct *proc;
//        __attribute__((optimize(0))) volatile struct rq *rqq;
        
        printk(KERN_ALERT "Done entering enqueue_task_mycfs \n");
        cfs_rq =  &rq->mycfs;
	printk(KERN_ALERT "Inside enqueue_task_mycfs function\n"); 
	if(se) {
          printk(KERN_ALERT "Inside for each sched, se = %x\n",(unsigned int)se); 
          if (se->on_rq)
            goto k_out;//was break;
//	  proc = mycfs_task_of(se);
//	  printk(KERN_ALERT "DEBUG Val of proc is %u\n",proc); 
//	  rqq = cpu_rq(0);//task_rq(proc);
//	  printk(KERN_ALERT "DEBUG Val of rqq is %u\n",rqq); 
          cfs_rq = &(rq->mycfs);
          printk(KERN_ALERT "DEBUG MYCFS task on cfs_rq is %x\n",(unsigned int)cfs_rq->curr); 
          printk(KERN_ALERT "DEBUG MYCFS &se =  %x and se->next =  %x\n",(unsigned int)&se->run_node,(unsigned int)se->run_node.next); 
          mycfs_enqueue_entity(cfs_rq, se, flags);
 
	}
k_out:
        if (!se) {
          inc_nr_running(rq);
        }
}

/*
 * The dequeue_task method is called before nr_running is
 * decreased. We remove the task from the rbtree and
 * update the fair scheduling stats:
 */
static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &p->mycfs_se;
        int task_sleep = flags & DEQUEUE_SLEEP;
 
        printk(KERN_ALERT "Done TD entering dequete_task_mycfs, sleep is %d \n",task_sleep);
	if(se) {
		cfs_rq = mycfs_rq_of(se);
		dequeue_entity(cfs_rq, se, flags);
        }
	if (!se) {
		dec_nr_running(rq);
        }
}

//NEEDS TO BE FIXED
static void yield_task_mycfs(struct rq *rq)
{
}

static bool yield_to_task_mycfs(struct rq *rq, struct task_struct *p, bool preempt)
{
  return true;
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_wakeup_mycfs(struct rq *rq, struct task_struct *p, int wake_flags)
{
  printk("DEBUG Entering Preempt wakeup mycfs\n");
//        struct task_struct *curr = rq->curr;
//        struct sched_mycfs_entity *se = &curr->se, *pse = &p->se;
//        struct mycfs_rq *cfs_rq = mycfs_task_cfs_rq(curr);
//        //FIX	int scale = cfs_rq->nr_running >= sched_nr_latency;
//        int next_buddy_marked = 0;
// 
//        if (unlikely(se == pse))
//        	return;
//// 
////        /*
////         * This is possible from callers such as move_task(), in which we
////         * unconditionally check_prempt_curr() after an enqueue (which may have
////         * lead to a throttle).  This both saves work and prevents false
////         * next-buddy nomination below.
////         */
////        if (unlikely(throttled_hierarchy(cfs_rq_of(pse))))
////        	return;
//// 
////        if (sched_feat(NEXT_BUDDY) && scale && !(wake_flags & WF_FORK)) {
////        	set_next_buddy(pse);
////        	next_buddy_marked = 1;
////        }
//// 
////        /*
////         * We can come here with TIF_NEED_RESCHED already set from new task
////         * wake up path.
////         *
////         * Note: this also catches the edge-case of curr being in a throttled
////         * group (e.g. via set_curr_task), since update_curr() (in the
////         * enqueue of curr) will have resulted in resched being set.  This
////         * prevents us from potentially nominating it as a false LAST_BUDDY
////         * below.
////         */
//        if (test_tsk_need_resched(curr))
//        	return;
// 
//        /* Idle tasks are by definition preempted by non-idle tasks. */
//        if (unlikely(curr->policy == SCHED_IDLE) &&
//            likely(p->policy != SCHED_IDLE))
//        	goto preempt;
// 
//        /*
//         * Batch and idle tasks do not preempt non-idle tasks (their preemption
//         * is driven by the tick):
//         */
//        if (unlikely(p->policy != SCHED_NORMAL) || !sched_feat(WAKEUP_PREEMPTION))
//        	return;
// 
//        //FIX	find_matching_se(&se, &pse);
//        update_curr(mycfs_rq_of(se));
//        BUG_ON(!pse);
////        if (wakeup_preempt_entity_mycfs(se, pse) == 1) {
////        	/*
////        	 * Bias pick_next to pick the sched entity that is
////        	 * triggering this preemption.
////        	 */
////        	if (!next_buddy_marked)
////        		set_next_buddy(pse);
////        	goto preempt;
////        }
// 
	return;
 
//preempt:
//        resched_task(curr);
//        /*
//         * Only set the backward buddy when the current task is still
//         * on the rq. This can happen when a wakeup gets interleaved
//         * with schedule on the ->pre_schedule() or idle_balance()
//         * point, either of which can * drop the rq lock.
//         *
//         * Also, during early boot the idle thread is in the fair class,
//         * for obvious reasons its a bad idea to schedule back to it.
//         */
//        if (unlikely(!se->on_rq || curr == rq->idle))
//        	return;
// 
//FIX         if (sched_feat(LAST_BUDDY) && scale && entity_is_task(se))
//FIX         	set_last_buddy(se);
}


/*
 * Account for a descheduled task:
 */
static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{
	struct sched_mycfs_entity *se = &prev->mycfs_se;
	struct mycfs_rq *cfs_rq;
 
        printk(KERN_ALERT "Done entering put_prev_task_mycfs  \n");
	if(se) {
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
	struct sched_mycfs_entity *se = &rq->curr->mycfs_se;
 
        printk(KERN_ALERT "Done entering set_curr_task_mycfs \n");
	for_each_sched_entity(se) {
          struct mycfs_rq *cfs_rq = mycfs_rq_of(se);
 
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
	struct sched_mycfs_entity *se = &curr->mycfs_se;
 
        printk(KERN_ALERT "Done TICK entering task_tick_mycfs \n");
	for_each_sched_entity(se) {
          cfs_rq = mycfs_rq_of(se);
        update_curr(cfs_rq);
// 
//         * Ensure that runnable average is periodically updated.
//        update_entity_load_avg(curr, 1);
//        update_cfs_rq_blocked_load(cfs_rq, 1);
//        update_cfs_shares(cfs_rq);
// 
        if (cfs_rq->nr_running > 1)
          check_preempt_tick_mycfs(cfs_rq, &curr->mycfs_se);
	}
        
}


/*
 * called on fork with the child task as argument from the parent's context
 *  - child not yet on the tasklist
 *  - preemption disabled
 */
static void task_fork_mycfs(struct task_struct *p)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &p->mycfs_se, *curr;
	int this_cpu = smp_processor_id();
	struct rq *rq = this_rq();
	unsigned long flags;
 
        printk(KERN_ALERT "Done entering task_fork_mycfs\n");
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
          se->vruntime = curr->vruntime;//FIX - probably so that on every fork, the child is not the first one to be kept on scheduled
	place_entity(cfs_rq, se, 1);
 
//FIX        if (sysctl_sched_child_runs_first && curr && entity_before(curr, se)) {
//FIX        	/*
//FIX        	 * Upon rescheduling, sched_class::put_prev_task() will place
//FIX        	 * 'current' within the tree based on its new key value.
//FIX        	 */
//FIX        	swap(curr->vruntime, se->vruntime);
//FIX        	resched_task(rq->curr);
//FIX        }
 
        //FIX	se->vruntime -= cfs_rq->min_vruntime;
 
	raw_spin_unlock_irqrestore(&rq->lock, flags);
        printk(KERN_ALERT "Done exiting task_fork_mycfs\n");
}


/*
 * Priority of the task has changed. Check to see if we preempt
 * the current task.
 */
static void
prio_changed_mycfs(struct rq *rq, struct task_struct *p, int oldprio)
{}

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
  printk(KERN_ALERT "Done entering switched_to_mycfs\n");
	if (!p->mycfs_se.on_rq)
		return;
 
	/*
	 * We were most likely switched from sched_rt, so
	 * kick off the schedule if running, otherwise just see
	 * if we can still preempt the current task.
	 */
	if (rq->curr == p) {
          printk(KERN_ALERT "Done CALLING RESCHED entering switched_to_mycfs\n");
          resched_task(rq->curr); 
        }
	else
		check_preempt_curr(rq, p, 0);
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
//        struct sched_mycfs_entity *se = &task->mycfs_se;
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
//        struct sched_mycfs_entity *left = se;

        printk(KERN_ALERT "Done TICK entering pick_next_entity_mycfs \n");

	printk(KERN_ALERT "Debug: Inside pick_next_entity_mycfs, first is %x\n",(unsigned int)se);
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
	
	printk(KERN_ALERT "Debug: Returned from pick_next_entity_mycfs, returning %x\n",(unsigned int) se);

	return se;
}


static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	struct task_struct *p;
        struct mycfs_rq *cfs_rq = &rq->mycfs;
        struct sched_mycfs_entity *se;
 
        //        printk(KERN_ALERT "Done entering pick_next_task_mycfs, leftmost = %x\n",(unsigned int) cfs_rq->leftmost);

	if (!cfs_rq->leftmost)
		return NULL;

        printk(KERN_ALERT "DEBUG in pick next task mycs, lefmost = %x\n",(unsigned int)cfs_rq->leftmost);

        do {
        	se = pick_next_entity_mycfs(cfs_rq);
        	set_next_entity(cfs_rq, se);
                //        	cfs_rq = group_cfs_rq(se);
        } while (0);//FIXcfs_rq);
// 
	p = mycfs_task_of(se);
//        if (hrtick_enabled(rq))
//        	hrtick_start_fair(rq, p);

        printk(KERN_ALERT "DEBUG TD In pick next task, RETURNING %x\n",(unsigned int) p);

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

