#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <pthread.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

int debug_mode = FALSE;
int time_each_request = TRUE;

typedef struct params {
  char **argv;
  int argc;
} params;

void error(const char *msg) { perror(msg); exit(0); }

void* send_request(void* param){
  pthread_t t_id = pthread_self(); 

  // command line arguments initializations
  char **argv = ((params*)param)->argv;
  int argc = ((params*)param)->argc;

  // Request, socket related initilizations
  int portno = atoi(argv[4])>0?atoi(argv[4]):80;
  char *host = strlen(argv[3])>0?argv[3]:"localhost";
  int num_requests = atoi(argv[2]);

  struct hostent *server;
  struct sockaddr_in serv_addr;
  int sockfd, bytes, sent, received, total_received, total, message_size;
  char *message, *message_no_ws, response[4096], uri[1000];

  // Read from file list
  FILE* fp = NULL;
  if(argc <= 6)
    fp = fopen("filelist.txt", "r");
  else
    fp = fopen(argv[6], "r");
  
  int file_count;
  char** filenames;
  fscanf(fp, "%d", &file_count);
  filenames = (char**)malloc(sizeof(char*) * file_count);
  for(int i=0; i<file_count; i++){
    filenames[i] = (char*)malloc(sizeof(char) * 50);
    fscanf(fp, "%s", filenames[i]);
  }

  // Send requests
  total_received = 0;
  for(int req_cnt=0; req_cnt < num_requests; req_cnt++ ){
      clock_t start; start = clock();
      sprintf(uri, "/txt?%s", filenames[req_cnt%file_count]);
      message_size=0;
      if(!strcmp(argv[5],"GET"))
      {
          message_size+=strlen("%s %s%s%s HTTP/1.0\r\n");        /* method         */
          message_size+=strlen(argv[5]);                         /* path           */
          message_size+=strlen(uri);
          message_size+=strlen("\r\n");                          /* blank line     */
      }

      /* allocate space for the message */
      message=malloc(message_size);
      message_no_ws=malloc(message_size);

      /* fill in the parameters */
      if(!strcmp(argv[5],"GET"))
      {
          sprintf(message,"%s %s HTTP/1.0\r\n", "GET", uri);
          sprintf(message_no_ws,"%s %s HTTP/1.0\r\n", "GET", uri);
          strcat(message,"\r\n");                                /* blank line     */
      }

      /* What are we going to send? */
      printf("Request: %s",message_no_ws);

      /* create the socket */
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) error("ERROR opening socket");

      /* lookup the ip address */
      server = gethostbyname(host);
      if (server == NULL) error("ERROR, no such host");

      /* fill in the structure */
      memset(&serv_addr,0,sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(portno);
      memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

      /* connect the socket */
      if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
          error("ERROR connecting");

      /* send the request */
      total = strlen(message);
      sent = 0;
      do {
          bytes = write(sockfd,message+sent,total-sent);
          if (bytes < 0)
              error("ERROR writing message to socket");
          if (bytes == 0)
              break;
          sent+=bytes;
      } while (sent < total);

      memset(response, 0, sizeof(response));
      total = sizeof(response)-1;
      received = 0;
      do {
          if(debug_mode) printf("%s", response);
          // HANDLE RESPONSE CHUCK HERE BY, FOR EXAMPLE, SAVING TO A FILE.
          memset(response, 0, sizeof(response));
          bytes = recv(sockfd, response, 1024, 0);
          if (bytes < 0)
              printf("ERROR reading response from socket");
          if (bytes == 0)
              break;
          received+=bytes;
      } while (1); 
      if(debug_mode) printf("\n");
      printf("[%ud] Total bytes received: %d\n", t_id, received);
      total_received += received;

      if (received == total)
          error("ERROR storing complete response from socket");

      /* close the socket */
      close(sockfd);
      free(message);

      clock_t end; end = clock();
      if(time_each_request) printf("** One request took: %lfs **\n", ((double) (end - start)) / CLOCKS_PER_SEC);

  } // for req_cnt
  return (void*)total_received;
}

int main(int argc,char *argv[])
{
    int num_threads = atoi(argv[1]);
    if (argc < 6) { puts("Parameters: <num_threads> <num_reqs> <host> <port> <method> <optional:filelist>"); exit(0); }

    pthread_t threads[num_threads];
    params *param = (params*)malloc(sizeof(param));
    param->argc = argc;
    param->argv = argv;

    clock_t start; start = clock();
    // spawn threads
    for (int i=0; i<num_threads; i++){
      int tid = pthread_create(&threads[i], NULL, send_request, (void*)param);
    }

    // join threads
    int status, total_received = 0;
    for (int i=0; i<num_threads; i++){
      pthread_join(threads[i], (void**)&status);
      total_received += (int)status;
    }
    clock_t end; end = clock();
    printf("** Total bytes received: %d **\n", total_received);
    printf("** Time taken: %lfs **\n", ((double) (end - start)) / CLOCKS_PER_SEC);



    //send_request((void*)param);

    
    return 0;
}