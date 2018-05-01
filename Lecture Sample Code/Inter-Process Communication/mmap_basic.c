#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SEM_NAME  "my_sem"


int main(int argc, char *argv[])
{
  int fd, i;
  int *ptr;
  int init_value = 0;

  //Initialize the semaphore (which will serve as a mutex)
  sem_t *mutex = sem_open(SEM_NAME, O_CREAT, 0644, 1);

  //Open a file that we will later map
  fd = open("mmap_basic.txt", O_RDWR | O_CREAT);

  //Write an initial value into the start of the file
  write(fd, &init_value, sizeof(int));

  //Map the file into this process
  ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  //Child process(es) inherit memory mapped address space regions
  int id = fork();

  if (id == 0) {
    for (i=0; i < 100; i++) {
      sem_wait(mutex);
      printf("child: %d\n", (*ptr)++);
      sem_post(mutex);
    }
  } else {
    for (i=0; i < 100; i++) {
      sem_wait(mutex);
      printf("parent: %d\n", (*ptr)++);
      sem_post(mutex);
    }

    wait(NULL); //Wait for child process to complete
  }

  return 0;
}
