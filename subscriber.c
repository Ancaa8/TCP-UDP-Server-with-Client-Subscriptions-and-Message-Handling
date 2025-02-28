#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "common.h"
#include "helpers.h"

//pregatire socket pentru conectarea la server
int start_client(const char *server_ip, const char *server_port) {
    uint16_t port;
    int sockfd, rc;
    struct sockaddr_in serv_addr;

    rc = sscanf(server_port, "%hu", &port);
    if (rc != 1) {
    perror("Error");
    exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    perror("Error");
    exit(EXIT_FAILURE);
    }

    int flag = 1;
    rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    if (rc < 0) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);
    if (rc <= 0) {
    perror("Error");
    exit(EXIT_FAILURE);
    }

    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc < 0) {
    perror("Error");
    exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Funcția pentru rulare tcp
void run_client(int sockfd, const char *client_id) {
    char buf[MSG_MAXSIZE + 1];
    memset(buf, 0, MSG_MAXSIZE + 1);

    struct chat_packet sent_packet;
    struct chat_packet recv_packet;

    sent_packet.len = strlen(client_id) + 1;
    strcpy(sent_packet.message, client_id);
    send_all(sockfd, &sent_packet, sizeof(sent_packet));

    struct pollfd poll_fds[2];
    poll_fds[0].fd = sockfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    while (1) {
        poll(poll_fds, 2, 1);

        if (poll_fds[0].revents & POLLIN) {
            int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
            if (rc <= 0) {
                break;
            }
            printf("%s\n", recv_packet.message);
        }

        if (poll_fds[1].revents & POLLIN) {
            if (fgets(buf, sizeof(buf), stdin) == NULL) {
                break;
            }
            sent_packet.len = strlen(buf) + 1;
            strcpy(sent_packet.message, buf);
            send_all(sockfd, &sent_packet, sizeof(sent_packet));
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc != 4) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    const char *client_id = argv[1];
    const char *server_ip = argv[2];
    const char *server_port = argv[3];

    int sockfd = start_client(server_ip, server_port);

    run_client(sockfd, client_id);
    close(sockfd);

    return 0;
}
