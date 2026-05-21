#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "israeli.h"

struct israeli_lock israeli_locks[NISRAELI];

void
israeli_init(void)
{
  for(int i = 0; i < NISRAELI; i++){
    initlock(&israeli_locks[i].lk, "israeli");
    israeli_locks[i].active = 0;
    israeli_locks[i].favoritism = 0;
    israeli_locks[i].owner = 0;
    israeli_locks[i].qlen = 0;
  }
}

int
israeli_create(int favoritism)
{
  if(favoritism < 0 || favoritism > 100)
    return -1;

  for(int i = 0; i < NISRAELI; i++){
    struct israeli_lock *L = &israeli_locks[i];
    acquire(&L->lk);
    if(!L->active){
      L->active = 1;
      L->favoritism = favoritism;
      L->owner = 0;
      L->qlen = 0;
      release(&L->lk);
      return i;
    }
    release(&L->lk);
  }
  return -1;
}

int
israeli_acquire(int id)
{
  if(id < 0 || id >= NISRAELI)
    return -1;

  struct israeli_lock *L = &israeli_locks[id];
  struct proc *p = myproc();

  acquire(&L->lk);
  if(!L->active){
    release(&L->lk);
    return -1;
  }
  // Reject re-entrant acquire.
  if(L->owner == p){
    release(&L->lk);
    return -1;
  }

  // Fast path: nobody holds it and nobody is waiting.
  if(L->owner == 0 && L->qlen == 0){
    L->owner = p;
    release(&L->lk);
    return 0;
  }

  // Slow path: enqueue and sleep until the releaser hands ownership over.
  if(L->qlen >= ISRAELI_MAX_WAITERS){
    release(&L->lk);
    return -1;
  }
  L->queue[L->qlen++] = p;

  // sleep() atomically releases L->lk and parks us. We use the proc pointer
  // itself as the channel so the releaser can wake exactly this waiter via
  // wakeup(winner). Loop guards against spurious wakeups (e.g. from wait()
  // in a parent process that shares this proc pointer as a channel).
  while(L->owner != p){
    sleep(p, &L->lk);
    if(!L->active){            // lock destroyed under us — bail
      release(&L->lk);
      return -1;
    }
  }
  // The releaser already dequeued us and set L->owner = p.
  release(&L->lk);
  return 0;
}

int
israeli_release(int id)
{
  if(id < 0 || id >= NISRAELI)
    return -1;

  struct israeli_lock *L = &israeli_locks[id];
  struct proc *p = myproc();

  acquire(&L->lk);
  if(!L->active || L->owner != p){
    release(&L->lk);
    return -1;
  }

  if(L->qlen == 0){
    L->owner = 0;
    release(&L->lk);
    return 0;
  }

  // Pick the next owner.
  int G = p->gid;
  int same_group_idx = -1;
  for(int i = 0; i < L->qlen; i++){
    if(L->queue[i]->gid == G){
      same_group_idx = i;
      break;
    }
  }

  int chosen;
  if(same_group_idx >= 0 && (lcg_rand() % 100) < (uint)L->favoritism){
    chosen = same_group_idx;   // favoritism path
  } else {
    chosen = 0;                // FIFO path
  }

  struct proc *winner = L->queue[chosen];

  // Remove winner from the queue (shift the tail left).
  for(int i = chosen; i < L->qlen - 1; i++){
    L->queue[i] = L->queue[i+1];
  }
  L->qlen--;

  // Hand off ownership directly — lock never visibly becomes free.
  L->owner = winner;
  wakeup(winner);
  release(&L->lk);
  return 0;
}

int
israeli_destroy(int id)
{
  if(id < 0 || id >= NISRAELI)
    return -1;

  struct israeli_lock *L = &israeli_locks[id];

  acquire(&L->lk);
  if(!L->active || L->owner != 0 || L->qlen != 0){
    release(&L->lk);
    return -1;
  }
  L->active = 0;
  release(&L->lk);
  return 0;
}
