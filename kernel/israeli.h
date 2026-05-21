// Israeli lock — FIFO with group-based favoritism.
// At most NISRAELI locks system-wide; each lock supports
// at most ISRAELI_MAX_WAITERS processes in its queue.

#define NISRAELI            16
#define ISRAELI_MAX_WAITERS 16

struct israeli_lock {
  struct spinlock lk;                            // protects everything below
  int active;                                    // 1 if created, 0 if free slot
  int favoritism;                                // 0..100, set at create
  struct proc *owner;                            // current holder, 0 if free
  struct proc *queue[ISRAELI_MAX_WAITERS];       // FIFO of waiting procs
  int qlen;                                      // number of waiters
};
