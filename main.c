#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cross-Platform Socket Boilerplate
#if defined(_WIN32)
#include <winsock2.h>
    #include <ws2tcpip.h>
#if defined(_MSC_VER)
    #pragma comment(lib, "ws2_32.lib")
#endif
    #define CLOSESOCKET(s) closesocket(s)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define INVALID_SOCKET -1
#endif

#include "dns_wire.h"

int main(void) {
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        printf("Failed to initialize Windows Sockets.\n");
        return 1;
    }
#endif

    const char *target_domain = "example.com";

    // 1. Build the query
    size_t query_len = 0;
    char *query_buf = build_dns_query(target_domain, &query_len);
    if (!query_buf) {
        printf("Failed to build DNS query\n");
        return 1;
    }

    // 2. Open Socket
    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        free(query_buf);
        return 1;
    }
    printf("Success! Socket File Descriptor ID: %d\n", (int)sockfd);

    // 3. Setup Destination (8.8.8.8:53)
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);

    // 4. Send Packet
    int bytes_sent = sendto(sockfd, query_buf, query_len, 0,
                            (struct sockaddr *)&dest, sizeof(dest));
    if (bytes_sent < 0) {
        printf("sendto failed\n");
        free(query_buf);
        CLOSESOCKET(sockfd);
        return 1;
    }
    printf("Sent %d bytes to 8.8.8.8\n", bytes_sent);

    // 5. Receive Response
    char reply_buf[1024];
    memset(reply_buf, 0, sizeof(reply_buf));
    struct sockaddr_in src_addr;
    socklen_t src_len = sizeof(src_addr);

    int reply_len = recvfrom(sockfd, reply_buf, sizeof(reply_buf), 0,
                             (struct sockaddr *)&src_addr, &src_len);

    if (reply_len < 0) {
        printf("recvfrom failed\n");
        free(query_buf);
        CLOSESOCKET(sockfd);
        return 1;
    }
    printf("Received %d bytes from DNS server\n", reply_len);

    // 6. Print Hex
    printf("\nDNS Response Hex:\n");
    for (int i = 0; i < reply_len; i++) {
        printf("%02X ", (unsigned char)reply_buf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n\n");

    // 7. Parse the IP using our wire module
    char resolved_ip[16];
    if (parse_dns_reply(reply_buf, reply_len, resolved_ip, sizeof(resolved_ip)) == 0) {
        printf(">>> Resolved IP: %s <<<\n", resolved_ip);
    } else {
        printf("Failed to parse A record from response.\n");
    }

    // 8. Cleanup
    free(query_buf);
    CLOSESOCKET(sockfd);
#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}
