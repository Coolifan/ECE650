#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

char *cmd1[] = {"ls", "-l", 0}; // 'ls -l' command
char *cmd2[] = {"wc", "-l", 0}; // 'wc -l' command

int main(int argc, char *argv[])
{
  int pid;
  int pipefd[2];

  //Create the pipe
  pipe(pipefd);

  //Create a child process
  pid = fork();

  if (pid == 0) {          //child
    dup2(pipefd[0], 0);    //Make stdin equivalent to read end of pipe
    close(pipefd[1]);      //Child doesn't need the write end of the pipe
    execvp(cmd2[0], cmd2); //execute 'wc -l'
  } else {                   //parent
    dup2(pipefd[1], 1);      //Make stdout equivalent to the write end of the pipe
    close(pipefd[0]);        //Parent doesn't need the read end of the pipe
    execvp(cmd1[0], cmd1);   //execute 'ls -l'
  } 

  return 0;
}


