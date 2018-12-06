#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "lib/mylib.h"

struct Server {
  char ip[255];
  int port;
};



bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  uint64_t answer = 1;
  FILE *fp;
  char servers[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k))
            return 1;
      case 1:
        if(!ConvertStringToUI64(optarg, &mod))
            return 1;
        break;
      case 2:
        memcpy(servers, optarg, strlen(optarg));
        fp = fopen(servers, "r");
        if (fp == NULL)
        {
            printf("File %s not found",servers);
            return 1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k \"num\" --mod \"num\" --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  
  unsigned int servers_num = 0;
  char *addr = NULL;
  char portl[255];
  char *line = NULL; 
  size_t len = 0;
  struct Server *to = malloc( sizeof(struct Server));
  
  while (getline(&line, &len, fp) != -1)
  {
    servers_num +=1;
    to = (struct Server*)realloc(to, sizeof(struct Server) * servers_num);
    int j = 0;
    while(line[j]!=':')
        j++; 
    strncpy(to[servers_num - 1].ip, line, j);
    strncpy(portl, line+j+1, strlen(line) - j);
    printf("IP: %s Port: %s j:%d size: %lu\n", to[servers_num - 1].ip, portl, j, strlen(line));
    //memcpy(to[servers_num - 1].ip, line, sizeof(line)+1);
    
    
    to[servers_num-1].port = atoi(portl);
  }
  int *sck_arr = calloc(servers_num, sizeof(int));
  int used = 0;
  for (int i = 0; i < servers_num; i++) {
    struct hostent *hostname = gethostbyname(to[i].ip);
    
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      exit(1);
    }
    printf("Connect to %s:%d\n",to[i].ip, to[i].port);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }
    sck_arr[i] = sck;
    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }
    
    uint64_t begin = 1 + i * k/servers_num + (i>0);
    uint64_t end = 1 + (i+1) * k/servers_num;
    if (end > k)
        end = k;
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));
    
    if (send(sck, task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }

    used +=1;

    // close(sck);
    if (end == k)
        break;
    
  }
 
  
 
  
  for(int i = 0; i<used;i++)
  {
    char response[sizeof(uint64_t)];
    if (recv(sck_arr[i], response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }
    uint64_t serv_answer = 0;   
    memcpy(&serv_answer, response, sizeof(uint64_t));
    printf("server %s:%d answer is %lu\n", to[i].ip ,to[i].port, serv_answer);
    answer = MultModulo(answer, serv_answer, mod);
    close(sck_arr[i]);
  }
  
  printf("answer: %lu\n", answer);
  free(line);
  free(to);
  fclose(fp);
  return 0;
}