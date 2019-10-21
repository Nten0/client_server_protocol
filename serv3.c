#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include "keyvalue.h"
#include <errno.h>

#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


int shmid;


struct mem
{
    int counter;
    char memory[1000][2][1024];
};


void sig_chld(int sig) 
{
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
    signal(SIGCHLD,sig_chld);
}


ssize_t writen(int fd, const void *vptr, size_t n)
{
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;
  ptr = vptr;
  nleft = n;
  while (nleft > 0)
  {
    if ( (nwritten = write(fd, ptr, nleft)) <= 0 ) 
    {
      if (errno == EINTR)
       nwritten = 0;
      else
        return -1;
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return n;
}


void my_func(int newsock)
{
    char buffer[1024];
    memset(&buffer,'\0',sizeof(buffer));
    char tmp[1024];
    memset(&tmp,'\0',sizeof(tmp));
    char key[50];
    memset(&key,'\0',sizeof(key));
    char value[50];
    memset(&value,'\0',sizeof(value));
    char return_buffer[1024];
    memset(&return_buffer,'\0',sizeof(return_buffer));
    int k = 0;
    int l = 0;     
    int len;
        while((len = read(newsock,buffer,1024))>0)
        {
          k = 0;
          while(k < len)
          {  
                  if(buffer[k] == 'p')
                  {
                    l = 0;
                    k++;
                      do
                      {
                        key[l] = buffer[k];
                        k++;
                        l++;
                      }while (buffer[k]!='\0');
                      key[k]='\0'; 

                      l = 0;
                      do
                      {
                        k++;
                        value[l] = buffer[k];
                        l++;
                      }while (buffer[k]!= '\0');
                      value[l] = '\0';

                      put(key,value);

                      memset(&key,'\0',sizeof(key));
                      memset(&value,'\0',sizeof(value));
                  }
                  else if(buffer[k] == 'g')
                  {
                      l = 0;
                      k++;
                      do
                      {
                        key[l] = buffer[k];
                        k++;
                        l++;
                      }while (buffer[k]!='\0');

                      memset(&tmp,'\0',sizeof(tmp));
                      strcpy(tmp,get(key));

                      if(strcmp(tmp, "NULL") == 0)
                      {
                          return_buffer[0] = 'n';
                          writen(newsock,return_buffer,1);
                      }
                      else 
                      {
                          return_buffer[0] = 'f';
                          int i;
                          for(i = 0; i < strlen(tmp); i++)
                              return_buffer[i+1] = tmp[i];

                          return_buffer[i+1] = '\0';
                          writen(newsock,return_buffer,strlen(return_buffer)+1);
                      }
                      memset(&key,'\0',sizeof(key));
                      memset(&tmp,'\0',sizeof(tmp));
                      memset(&return_buffer,'\0',sizeof(return_buffer));
                  }
                  else 
                  {
                      k = len;
                      close (newsock); 
                  }                
                  k++;
          }
        }   
}



void put(char *key, char *value)
{
  struct mem *my_mem;
  my_mem = (struct mem*)shmat(shmid,0,0);
  int flag = 0;

  for (int i = 0; i < my_mem->counter; i++)
  {
        if(strcmp(my_mem->memory[i][0], key) == 0)
        {
            flag = 1;
            strcpy(my_mem->memory[i][1], value);
        }
  }
  
  if (flag == 0)
  {
      strcpy(my_mem->memory[my_mem->counter][0], key);
      strcpy(my_mem->memory[my_mem->counter][1], value);

      my_mem->counter = my_mem->counter + 1;
  }
}


char *get(char *key)
{
  struct mem *my_mem;
  my_mem = (struct mem*)shmat(shmid,0,0);

    for (int i = 0; i < my_mem->counter; i++)
    {
        if(strcmp(key, my_mem->memory[i][0]) == 0)
        {
            strcpy(key, my_mem->memory[i][1]);
            return key;
        }
    }
    return "NULL";  
}


int main(int argc, char *argv[])
{
    int sockfd, port_num, newsock;
    struct sockaddr_in client_addr;
    socklen_t addrlen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
    if (sockfd < 0) 
    {
        perror("ERROR: server cannot open a socket\n");
        return -1;
    }

    client_addr.sin_family = AF_INET;

    if (argc < 2) 
    {
        //printf("WARNING: no port provided (port set to 1234) \n");
        port_num = 1234;
    } 
    else
        port_num = atoi(argv[1]);

    client_addr.sin_port = htons(port_num);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    if(bind(sockfd, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("ERROR: server cannot specify the address of the socket\n");
        return -1;
    }

    if(listen(sockfd, 5) == 0){}
        //printf("Listening...\n");
    else 
    {
        perror("ERROR: client's socket cannot receive incoming connections\n");
        return -1;
    }

    addrlen = sizeof(struct sockaddr_in);

    struct sembuf up = {0, 1, 0};
    struct sembuf down = {0, -1, 0};

    int len = sizeof(struct mem);
    shmid = shmget(IPC_PRIVATE, len, 0600);
    struct mem *my_mem;
    my_mem = (struct mem*)shmat(shmid,0,0);
    my_mem->counter = 0;

    int num_of_processes = atoi(argv[2]);
    pid_t pid;
    for(int i = 0; i < num_of_processes; i++)
    {
      pid = fork();
      if(pid == 0)
      {
          while(1)
          {
              int my_sem = 1;
              semop(my_sem, &down, 1);
                  newsock = accept(sockfd, (struct sockaddr *) &client_addr, &addrlen);
                  if(newsock < 0)
                  {
                      perror("ERROR (accept): server cannot create a new socket \n");
                      return -1;
                  }
                  my_func(newsock);
                  close(newsock);
              semop(my_sem, &up, 1);
          } 
      }
      else
          close(newsock);
    }
    signal(SIGCHLD,sig_chld);
    while((pid = wait(NULL)) > 0);
}