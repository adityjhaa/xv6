// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAX_ATTEMPTS 3
#define BUFFER_LEN 32

char *argv[] = { "sh", 0 };

int
login(void)
{
  char username[BUFFER_LEN], password[BUFFER_LEN];
  int attempts = 0;
  while(attempts < MAX_ATTEMPTS){
    printf(1,"Enter username: ");
    gets(username, sizeof(username));
    username[strlen(username) - 1] = '\0';
    if(strcmp(username, USERNAME) != 0){
      attempts++;
      continue;
    }
    printf(1, "Enter password: ");
    gets(password, sizeof(password));
    password[strlen(password) - 1] = '\0';
    if(strcmp(password, PASSWORD) != 0){
      attempts++;
    }else{
      printf(1, "Login successful\n");
      return 1;
    }
  }
  return 0;
}

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf(1, "init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      if(!login()){
        printf(2, "Login Unsuccessful\n");
        while(1) sleep(100);
        exit();
      }
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
