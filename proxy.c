//
// Created by HUAWEI on 4/25/2026.
//
#include <stdio.h>      // For printf(), perror()
#include <stdlib.h>     // For exit()
#include <string.h>     // For memset()
#include <unistd.h>     // For close()
#include <sys/socket.h> // For socket(), bind(), AF_INET, SOCK_DGRAM
#include <netinet/in.h> // For struct sockaddr_in, htons()
#include <arpa/inet.h>  // For inet_addr()
#include <sys/select.h> // For fd_set, select(), FD_ZERO, FD_SET

int main(){
    int local_sock=soket(AF_INET,SOCK_DGRAM,0);
    if(local_sock<0){
        perror("failed to creat socket\n");
        exit(EXIT_FAILURE);
    }
        printf("Local socket created successfully. FD: %d\n", local_sock);
    struct sockaddr_in local_addr;
    memset(&local_addr,0, sizeof(local_addr))
    local_adrr.sin_family=AF_INET;
    // Convert IP string → binary and store directly in sin_addr

    if(inet_pton(AF_INET,"127.0.0.1",&local_addr.sin_addr)<=0){
        perror("inet_pton failed\n");
        exit(EXIT_FAILURE);
    }
    local_addr.sin_port=htons(5353);
    if(bind(local_sock,(struct sockaddr *)&local_addr, sizeof(local_addr))<=0){
        perror("failed to bind\n");
        exit(EXIT_FAILURE);
    }
    int upstream_sock=socket(AF_INET,SOCK_DEGRAM,0);
    if(upstream_sock<0){
        perror("failed to creat socket\n");
        exit(EXIT_FAILURE);

    }
    printf("Upstream socket created successfully. FD: %d\n", upstream_sock);

    fd_set master_set;
    FD_ZERO(&master_set);
    FD_SET(local_sock,&master_set);
    FD_SET(upstream_sock,&master_set);
    int maxfd;
    if(local_sock>upstream_sock){
        maxfd=local_sock;
    } else{
        maxfd=upstream_sock;
    }
    printf("Setup complete. Monitoring FDs: %d and %d. MaxFD: %d\n",
           local_sock, upstream_sock, maxfd);

}