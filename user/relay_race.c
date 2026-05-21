#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NTEAMS  3
#define RUNNERS 5
#define TARGET  30

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

int
main(int argc, char *argv[])
{
  int favoritism = 50;
  if(argc >= 2)
    favoritism = atoi(argv[1]);

  int lock_id = israeli_create(favoritism);
  if(lock_id < 0){
    printf("israeli_create failed\n");
    exit(1);
  }
  score_reset();

  printf("Relay race starting: %d teams x %d runners, target %d, favoritism %d\n",
         NTEAMS, RUNNERS, TARGET, favoritism);

  for(int team = 0; team < NTEAMS; team++){
    for(int r = 0; r < RUNNERS; r++){
      int pid = fork();
      if(pid < 0){
        printf("fork failed\n");
        exit(1);
      }
      if(pid == 0){
        setgid(team);
        run_relay(lock_id, team);
        // not reached
      }
    }
  }

  for(int i = 0; i < NTEAMS * RUNNERS; i++)
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
