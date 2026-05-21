#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NPROC  15
#define GROUPS 4

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

  lcg_srand(getpid());

  for(int i = 0; i < NPROC; i++){
    int pid = fork();
    if(pid == 0){
      int gid = lcg_rand() % GROUPS;
      setgid(gid);

      israeli_acquire(lock_id);
      printf("Process %d (gid=%d) acquired the lock\n",
             getpid(), getgid());
      sleep(10);
      israeli_release(lock_id);
      exit(0);
    }
  }

  for(int i = 0; i < NPROC; i++)
    wait(0);

  israeli_destroy(lock_id);
  exit(0);
}
