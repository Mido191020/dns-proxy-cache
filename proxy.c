//
// Created by HUAWEI on 4/25/2026.
//
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <stdio.h>      // For printf(), perror()
#include <stdlib.h>     // For exit()
#include <string.h>     // For memset()
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSESOCKET(s) closesocket(s)
#else
#include <unistd.h>     // For close()
#include <sys/socket.h> // For socket(), bind(), AF_INET, SOCK_DGRAM
#include <netinet/in.h> // For struct sockaddr_in, htons()
#include <arpa/inet.h>  // For inet_pton()
#include <sys/select.h> // For fd_set, select(), FD_ZERO, FD_SET
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define INVALID_SOCKET -1
#endif

void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}
struct sockaddr_in  saved_client_addr;
int main() {
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        return 1;
    }
#endif

    // 1. Fixed 'soket' to 'socket' and 'local_sockt' to 'local_sock'
    SOCKET local_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (local_sock == INVALID_SOCKET) {
        die("failed to create socket");
    }
    printf("Local socket created successfully. FD: %d\n", local_sock);

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr)); // Semicolon was already added by you, great!

    // 2. Fixed 'local_adrr' to 'local_addr'
    local_addr.sin_family = AF_INET;

    // Convert IP string → binary and store directly in sin_addr
    if (inet_pton(AF_INET, "127.0.0.1", &local_addr.sin_addr) <= 0) {
        die("inet_pton failed");
    }

    local_addr.sin_port = htons(5353);

    // 3. Changed '<=0' to '< 0'. bind() returns 0 on success, so we only fail if it is less than 0 (-1).
    if (bind(local_sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        die("failed to bind");
    }

    // 4. Fixed 'SOCK_DEGRAM' to 'SOCK_DGRAM'
    SOCKET upstream_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (upstream_sock == INVALID_SOCKET) {
        die("failed to create socket");
    }
    printf("Upstream socket created successfully. FD: %d\n", upstream_sock);

    // Setup the Radar (fd_set)
    fd_set master_set;
    FD_ZERO(&master_set);

    // 5. Correctly passing local_sock by value, and master_set by reference
    FD_SET(local_sock, &master_set);
    FD_SET(upstream_sock, &master_set);
    int maxfd = MAX(local_sock, upstream_sock);
    printf("Setup complete. Monitoring FDs: %d and %d. MaxFD: %d\n",
           local_sock, upstream_sock, maxfd);

    while(1){
        fd_set working_set = master_set;
        printf("\nWaiting for activity on sockets...\n");
        int activity = select(maxfd + 1, &working_set, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(local_sock, &working_set)) {
            unsigned char buffer[512];
            struct sockaddr_in client_addr;
            socklen_t addr_len=sizeof (client_addr);
            int bytes_read = recvfrom(local_sock,
                                      buffer,
                                      sizeof(buffer),
                                      0,
                                        (struct sockaddr *)&client_addr,
                                     &addr_len
                    );
            if (bytes_read<0){
              perror("recvfrom local_sock failed");
            }
            else{
                printf("Received %d bytes from Client (%s:%d)\n",
                       bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            struct sockaddr_in google_addr;
                memset(&google_addr,0,sizeof(google_addr));
                google_addr.sin_family=AF_INET;
                google_addr.sin_port= htons(53);
                inet_pton(AF_INET,"8.8.8.8",&google_addr.sin_addr);
                saved_client_addr=client_addr;
                int bytes_sent= sendto(upstream_sock,
                                       buffer,
                                       bytes_read,
                                       0,
                                       (struct sockaddr *)&google_addr,
                                               sizeof(google_addr)
                                       );
                if (bytes_sent < 0){
                    perror("sendto google failed");
                }else{
                    printf("Forwarded %d bytes to Google (8.8.8.8:53)\n", bytes_sent);
                }
            }

        }
        // 1. Check if the "Messenger" (upstream_sock) has a reply from Google
        if (FD_ISSET(upstream_sock, &working_set)) {
            unsigned char upstream_buffer[512];
            struct sockaddr_in google_reply_addr;
            socklen_t google_addr_len = sizeof(google_reply_addr);

            // 2. Read the reply from Google
            int upstream_bytes = recvfrom(upstream_sock,
                                          upstream_buffer,
                                          sizeof(upstream_buffer),
                                          0,
                                          (struct sockaddr *)&google_reply_addr,
                                          &google_addr_len);

            if (upstream_bytes < 0) {
                perror("recvfrom upstream_sock failed");
            } else {
                printf("Received %d bytes reply from Google (%s)\n",
                       upstream_bytes, inet_ntoa(google_reply_addr.sin_addr));

                // 3. Send the reply back to the ORIGINAL client
                int final_sent = sendto(local_sock,
                                        upstream_buffer,
                                        upstream_bytes,
                                        0,
                                        (struct sockaddr *)&saved_client_addr,
                                        sizeof(saved_client_addr));

                if (final_sent < 0) {
                    perror("sendto client failed");
                } else {
                    printf("Successfully sent reply back to Client.\n");
                }
            }
        }

    }

    return 0; // Good practice to return 0 at the end of main
}