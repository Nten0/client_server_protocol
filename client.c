#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <errno.h>

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


int main(int argc, char *argv[])
{
  int sockfd, port_num;
  char buffer[1024];
  memset(&buffer,'\0',sizeof(buffer));
  char tmp[1024];
  memset(&tmp,'\0',sizeof(tmp));
  struct sockaddr_in serv_addr;
  socklen_t addrlen;

  struct hostent *serv_name;
  serv_name = NULL;
  serv_name = gethostbyname(argv[1]);
  if (serv_name == NULL)
  {
  		printf("ERROR: no such host\n");
    	return -1;
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (sockfd < 0) 
  {
      perror("ERROR: client cannot open a socket\n");
      return -1;
  }

  serv_addr.sin_family = AF_INET;
  port_num = atoi(argv[2]);
  serv_addr.sin_port = htons(port_num);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
  {
      perror("ERROR: client cannot initiate connection with the server\n");
      return -1;
  }

  memset(&buffer,'\0',strlen(buffer));
  for (int i = 3; i <= argc - 1; i++)
  {
      int length = 0;
      if (strcmp(argv[i], "put") == 0)
      {
          if( argv[i+1] == NULL || argv[i+2] == NULL)
          {
            close (sockfd);
            return 0;
          }

          buffer[length] ='p';
          length++;
          for (int j = 0 ; j< strlen(argv[i+1]); j++)
          {
            buffer[length] = argv[i+1][j];
            length++;
          }
          buffer[length] = '\0';
          
          length++;
          for (int j = 0 ; j< strlen( argv[i+2]); j++)
          {
            buffer[length] = argv[i+2][j];
            length++;
          }
          buffer[length] = '\0';
          
          if(writen(sockfd,buffer,length+1) < 0)
            perror("ERROR: client cannot write to the socket\n");

          //usleep (50000);

          i = i+2;
      }
      else if (strcmp(argv[i], "get") == 0)
      {
          if( argv[i+1] == NULL)
          {
            close (sockfd);
            return 0;
          }

          buffer[length] ='g';
          length++;
          for (int j = 0; j < strlen(argv[i+1]); j++)
          {
            buffer[length] = argv[i+1][j];
            length++;
          }
          buffer[length] ='\0';

          if(writen(sockfd,buffer,strlen(buffer)+1) < 0)
            perror("ERROR: client cannot write to the socket\n");
          
          //usleep (1000);

          char tmp2[1024];
          memset(&tmp2,'\0',sizeof(tmp2));
          int len;
          int count = 0;
          do
          {
              len = read(sockfd,tmp2,1024);
              memcpy(tmp+count,tmp2,len); 
              if(len == 1 && tmp[len-1] == 'n')
                  break;
              count += len;
              memset(&tmp2,'\0',strlen(tmp2));
          }while(tmp[count-1] != '\0');

          if(len < 0)
              perror("ERROR: client cannot read from the socket\n");
          else
          {
              if(tmp[0] == 'n')
                  printf("\n");
              else if(tmp[0] == 'f')
              {
                 for(int j = 1; j < strlen(tmp); j++)
                    printf("%c",tmp[j]);
                  printf("\n");
              }
              memset(&tmp,'\0',sizeof(tmp));  
          }         
          i++;      
      }
      else
      {
          buffer[0] = '\0';
          writen(sockfd,buffer,1);
          i = argc + 1;
      }
      memset(&buffer,'\0',strlen(buffer));
  }
  close (sockfd);
  return 0;

}
