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

static void mycfs_erase(struct mycfsnode *node,struct mycfs_rq *cfs_rq) {
	struct mycfsnode *link = cfs_rq->leftmost;
	struct mycfsnode *link_prev = cfs_rq->leftmost;

	while (link) {
	  if(link == node)
	    {
              link_prev->next = node->next;
              break;
            }//	      if(link)
          link_prev = link;
          link = link->next;
	}
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

  printk(KERN_ALERT "DEBUG Done entering __pick_first_entity_mycfs \n");
	if (!left)
		return NULL;

	return container_of(left, struct sched_mycfs_entity, run_node);
}

struct sched_mycfs_entity *__pick_next_entity_mycfs(struct sched_mycfs_entity *se)
{
  struct mycfsnode *next = se->run_node.next;
  if (!next)
    return NULL;
  
  return container_of(next, struct sched_mycfs_entity, run_node);
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
  printk(KERN_ALERT "DEBUG Done entering dequeue entity \n");
	if (cfs_rq->leftmost == &se->run_node) {
		struct mycfsnode *next_node;

		next_node = mycfs_next(&se->run_node);
		cfs_rq->leftmost = next_node;
                printk(KERN_ALERT "DEBUG MYCFS Done moving to front of queue\n");
	}
        printk(KERN_ALERT "DEBUG Done moving to front of queue\n");
	mycfs_erase(&se->run_node,cfs_rq);//, &cfs_rq->tasks_timeline);
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

//FIX         if (!delta)
//FIX         	return;
//FIX  
//FIX         //__update_curr(cfs_rq, curr, delta_exec);
//FIX         curr->sum_exec_runtime += delta;
//FIX //        schedstat_add(cfs_rq, exec_clock, delta_exec);
//FIX //        delta_exec_weighted = calc_delta_fair(delta_exec, curr);
//FIX  
//FIX         curr->vruntime += delta;
//FIX         curr->exec_start = now;
 
}


static void
dequeue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se, int flags)
{
  printk(KERN_ALERT "DEBUG Done entering higher dequeue entity \n");
// 
	if (se != cfs_rq->curr)
		__mycfs_dequeue_entity(cfs_rq, se);
        se->on_rq = 0;
}


static void
set_next_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se)
{
  printk(KERN_ALERT "DEBUG Done entering set next entity \n");
	if (se->on_rq) {
          __mycfs_dequeue_entity(cfs_rq, se);
	}

        cfs_rq->curr = se;
        se->prev_gruntime = se->gruntime;
}



static void put_prev_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *prev)
{
  printk(KERN_ALERT "DEBUG Done entering put prev entity \n");

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
  unsigned long delta_exec;
	struct sched_mycfs_entity *se;
	long int delta;
        
	//ideal_runtime = sched_slice(cfs_rq, curr);
	delta_exec = curr->gruntime - curr->prev_gruntime;
        printk(KERN_ALERT "DEBUG LIMIT task's runtime is %ld and delta is %ld, prev is %ld, calc delta is %ld",curr->gruntime,delta_exec,curr->prev_gruntime,(curr->gruntime -curr->prev_gruntime) );
	if ((curr->limit != 0) && (curr->gruntime > curr->limit)) {
          printk(KERN_ALERT "DEBUG PREEMPT preempted task due to limit exceeded\n");
	  resched_task(rq_of(cfs_rq)->curr);
		return;
	}
 
	se = __pick_first_entity_mycfs(cfs_rq);
	delta = curr->gruntime - se->gruntime;
 
	if (delta < 0)
		return;
 
        if (delta > 10) {//FIX
          printk(KERN_ALERT "DEBUG PREEMPT preempted task due to unfair run time\n");
          resched_task(rq_of(cfs_rq)->curr);
        }
}

__init void init_sched_mycfs_class(void)
{

}

void init_mycfs_rq(struct mycfs_rq *cfs_rq)
{
  //	struct mycfsnode a;
	cfs_rq->leftmost = NULL; 
        cfs_rq->nr_running =0;
	printk(KERN_ALERT "DEBUG  inside init_mycfs_rq leftmost = %x\n",(unsigned int) cfs_rq->leftmost);
        //	printk(KERN_ALERT "DEBUG  inside init_mycfs_rq leftmost = %x\n",cfs_rq->leftmost->next);
	//	cfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

static void
mycfs_enqueue_entity(struct mycfs_rq *cfs_rq, struct sched_mycfs_entity *se, int flags)
{
  printk(KERN_ALERT "DEBUG Done entering enqueue entity \n");

	if (se != cfs_rq->curr)
		__mycfs_enqueue_entity(cfs_rq, se);
	se->on_rq = 1;

}


static void
enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *cfs_rq;
	struct sched_mycfs_entity *se = &p->mycfs_se;
//        __attribute__((optimize(0))) volatile struct task_struct *proc;
//        __attribute__((optimize(0))) volatile struct rq *rqq;
        
        printk(KERN_ALERT "DEBUG Done entering enqueue_task_mycfs \n");
        cfs_rq =  &rq->mycfs;
	printk(KERN_ALERT "DEBUG Inside enqueue_task_mycfs function\n"); 
	if(se) {
          printk(KERN_ALERT "DEBUG Inside for each sched, se = %x\n",(unsigned int)se); 
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
 
        printk(KERN_ALERT "DEBUG Done TD entering dequete_task_mycfs, sleep is %d \n",task_sleep);
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
	return;
}


/*
 * Account for a descheduled task:
 */
static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{
	struct sched_mycfs_entity *se = &prev->mycfs_se;
	struct mycfs_rq *cfs_rq;
 
        printk(KERN_ALERT "DEBUG Done entering put_prev_task_mycfs  \n");
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
 
        printk(KERN_ALERT "DEBUG Done entering set_curr_task_mycfs \n");
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
        //	struct sched_mycfs_entity *tse;

        if(se) {
          cfs_rq = mycfs_rq_of(se);
        update_curr(cfs_rq);
        if (cfs_rq->nr_running >= 1)
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
 
        printk(KERN_ALERT "DEBUG Done entering task_fork_mycfs\n");
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
 
	raw_spin_unlock_irqrestore(&rq->lock, flags);
        printk(KERN_ALERT "DEBUG Done exiting task_fork_mycfs\n");
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
  return;
}


/*
 * We switched to the sched_mycfs class.
 */
static void switched_to_mycfs(struct rq *rq, struct task_struct *p)
{
  printk(KERN_ALERT "DEBUG Done entering switched_to_mycfs\n");
	if (!p->mycfs_se.on_rq)
		return;
 
	/*
	 * We were most likely switched from sched_rt, so
	 * kick off the schedule if running, otherwise just see
	 * if we can still preempt the current task.
	 */
	if (rq->curr == p) {
          printk(KERN_ALERT "DEBUG Done CALLING RESCHED entering switched_to_mycfs\n");
          resched_task(rq->curr); 
        }
	else
		check_preempt_curr(rq, p, 0);
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
        return 0;
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

        printk(KERN_ALERT "DEBUG Done TICK entering pick_next_entity_mycfs \n");

        //	printk(KERN_ALERT "DEBUG RT Inside pick_next_entity_mycfs, first is %x\n",(unsigned int)se);
        while(se) {
          //          printk(KERN_ALERT "DEBUG RT Inside, gruntime is %d and limit is %d, \n",se->gruntime,se->limit);
          if((se->limit != 0) && (se->gruntime >= se->limit))
            se = __pick_next_entity_mycfs(se);
          else 
            break;
        }
          
//FIX        if (cfs_rq->next)// && wakeup_preempt_entity_mycfs(cfs_rq->next, left) < 1)
//FIX          se = cfs_rq->next;
	
	printk(KERN_ALERT "DEBUG  Returned from pick_next_entity_mycfs, returning %x\n",(unsigned int) se);

	return se;
}


static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	struct task_struct *p;
        struct mycfs_rq *cfs_rq = &rq->mycfs;
        struct sched_mycfs_entity *se;
 
        //        printk(KERN_ALERT "DEBUG Done entering pick_next_task_mycfs, leftmost = %x\n",(unsigned int) cfs_rq->leftmost);

	if (!cfs_rq->leftmost)
		return NULL;

        printk(KERN_ALERT "DEBUG 2 in pick next task mycs, lefmost = %x\n",(unsigned int)cfs_rq->leftmost);
        
//        if(cfs_rq->curr)
//          if(cfs_rq->curr->gruntime < cfs_rq->curr->limit) 
//            return mycfs_task_of(cfs_rq->curr);
        
        do {
          se = pick_next_entity_mycfs(cfs_rq);
//          if(se->gruntime > se->limit) 
//            return NULL;
          if(!se) {
            printk(KERN_ALERT "DEBUG returning NULL RT\n");
            return NULL;
          }
          printk(KERN_ALERT "DEBUG RT gruntime and limit are %ld and %d, gcount is %d\n",se->gruntime,se->limit,cfs_rq->gcount);
          set_next_entity(cfs_rq, se);
          //        	cfs_rq = group_cfs_rq(se);
        } while (0);//FIXcfs_rq);
        // 
        p = mycfs_task_of(se);

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

