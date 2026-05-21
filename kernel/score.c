// Kernel-resident team-score table for Task 2 (relay race).
// Shared across all processes; protected by an internal spinlock so a
// non-Israeli-lock caller can't corrupt it.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MAX_TEAMS 16

static struct {
  struct spinlock lk;
  int scores[MAX_TEAMS];
} score_table;

void
score_init(void)
{
  initlock(&score_table.lk, "scores");
  for(int i = 0; i < MAX_TEAMS; i++)
    score_table.scores[i] = 0;
}

int
score_reset(void)
{
  acquire(&score_table.lk);
  for(int i = 0; i < MAX_TEAMS; i++)
    score_table.scores[i] = 0;
  release(&score_table.lk);
  return 0;
}

int
score_inc(int team)
{
  if(team < 0 || team >= MAX_TEAMS)
    return -1;
  acquire(&score_table.lk);
  score_table.scores[team]++;
  int v = score_table.scores[team];
  release(&score_table.lk);
  return v;
}

int
score_get(int team)
{
  if(team < 0 || team >= MAX_TEAMS)
    return -1;
  acquire(&score_table.lk);
  int v = score_table.scores[team];
  release(&score_table.lk);
  return v;
}
