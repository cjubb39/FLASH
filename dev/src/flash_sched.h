#ifndef _FLASH_SCHED_H_
#define _FLASH_SCHED_H_
/* copied directly from sched.h */

/*
 * Task state bitmask. NOTE! These bits are also
 * encoded in fs/proc/array.c: get_task_state().
 *
 * We have two separate sets of flags: task->state
 * is about runnability, while task->exit_state are
 * about the task exiting. Confusing, but this way
 * modifying one set can't modify the other one by
 * mistake.
 */
#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define __TASK_STOPPED          4
#define __TASK_TRACED           8
/* in tsk->exit_state */
#define EXIT_DEAD               16
#define EXIT_ZOMBIE             32
#define EXIT_TRACE              (EXIT_ZOMBIE | EXIT_DEAD)
/* in tsk->state again */
#define TASK_DEAD               64
#define TASK_WAKEKILL           128
#define TASK_WAKING             256
#define TASK_PARKED             512
#define TASK_STATE_MAX          1024

/* end of sched.h */

typedef uint32_t flash_pid_t;
typedef uint8_t  flash_pri_t;
typedef uint16_t flash_state_t;

typedef struct {
	flash_pid_t pid;
	flash_pri_t pri;
	flash_state_t state;
} flash_task_t;

#endif
