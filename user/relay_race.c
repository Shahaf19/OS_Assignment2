// Task 2 — Relay Race Tournament.
// Submitted with favoritism = 0; also tested with c = 50 and c = 100.
// Pass a different value as argv[1] (e.g. `relay_race 100`) to override.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NTEAMS   3
#define RUNNERS  5
#define NRUNNERS (NTEAMS * RUNNERS)
#define TARGET   30

static int
race_over(void)
{
  for(int t = 0; t < NTEAMS; t++){
    if(score_get(t) >= TARGET)
      return 1;
  }
  return 0;
}

static void
run_relay(int lock_id, int team)
{
  while(1){
    israeli_acquire(lock_id);

    if(race_over()){
      israeli_release(lock_id);
      exit(0);
    }

    int s = score_inc(team);
    printf("Runner %d (Team %d) acquired the baton\n", getpid(), team);
    printf("Team %d score = %d\n", team, s);

    israeli_release(lock_id);
    sleep(1);
  }
}

// Build a balanced gid list — each team appears exactly RUNNERS times —
// then Fisher-Yates shuffle it. This keeps team sizes equal while
// randomizing fork order so no team has a built-in head start.
static void
shuffled_gids(int *gids)
{
  for(int i = 0; i < NRUNNERS; i++)
    gids[i] = i / RUNNERS;

  for(int i = NRUNNERS - 1; i > 0; i--){
    int j = lcg_rand() % (i + 1);
    int tmp = gids[i];
    gids[i] = gids[j];
    gids[j] = tmp;
  }
}

int
main(int argc, char *argv[])
{
  int favoritism = 0;
  if(argc >= 2)
    favoritism = atoi(argv[1]);

  int lock_id = israeli_create(favoritism);
  if(lock_id < 0){
    printf("israeli_create failed\n");
    exit(1);
  }
  score_reset();

  // Seed with uptime so different runs produce different shuffles.
  lcg_srand(uptime() ^ getpid());

  int gids[NRUNNERS];
  shuffled_gids(gids);

  printf("Relay race: %d teams x %d runners, target %d, favoritism %d\n",
         NTEAMS, RUNNERS, TARGET, favoritism);

  for(int i = 0; i < NRUNNERS; i++){
    int pid = fork();
    if(pid < 0){
      printf("fork failed\n");
      exit(1);
    }
    if(pid == 0){
      setgid(gids[i]);
      run_relay(lock_id, gids[i]);
      // not reached
    }
  }

  for(int i = 0; i < NRUNNERS; i++)
    wait(0);

  printf("\nFinal scores:\n");
  int winner = -1, best = -1;
  for(int t = 0; t < NTEAMS; t++){
    int s = score_get(t);
    printf("  Team %d: %d\n", t, s);
    if(s > best){ best = s; winner = t; }
  }
  printf("Winner: Team %d (favoritism = %d)\n", winner, favoritism);

  israeli_destroy(lock_id);
  exit(0);
}
