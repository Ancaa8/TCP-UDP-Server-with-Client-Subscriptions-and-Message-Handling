#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 40
#define BUFFER_SIZE 2000

//structura pentru clientii tcp
struct Client {
    int socket;
    struct sockaddr_in address;
    char id[20];
};

//structura pentru clientii udp
struct Client_udp {
    int socket_udp;
    struct sockaddr_in address_udp;
};

//structura pentru primirea mesajelor de la clientii udp 
struct Udp_packet {
    char topic[50];
    char type;
    void* value;
};
//structura pentru rezolvarea abonarii
struct topic_struct {
    char name[1501];
    struct pollfd subscribers_id[50][50];
};

//functie de transformare din int in string
void int_to_string(int num, char* str) {
    int i = 0;
    int len = 0;
    while (num != 0) {
        str[i] = num % 10 + '0';
        i++;
        num = num / 10;
    }
    len = i - 1;
    for (int j = 0; j < (len + 1) / 2; j++) {
        char temp = str[j];
        str[j] = str[len - j];
        str[len - j] = temp;
    }
    str[i] = '\0';
}

int mypow(int a, int b) {
    int rez = 1;
    for (int i = 0; i < b; i++) {
        rez = rez * a;
    }
    return rez;
}

//functie de transformare din uint16_t in string
void short_real_to_string(uint16_t numar, char* str) {
    float num100 = (float)numar / 100;
    sprintf(str, "%.2f", num100);
}

//functie pentru socket tcp
int start_tcp(int port) {
    int server_socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_tcp == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
    int flag = 1;
    if (setsockopt(server_socket_tcp, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag)) < 0) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    //legare socket
    if (bind(server_socket_tcp, (struct sockaddr*)&server_address,
        sizeof(server_address)) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    //ascultare
    if (listen(server_socket_tcp, MAX_CONNECTIONS) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    return server_socket_tcp;
}

//functie pentru socket tcp
int start_udp(int port) {
    int server_socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_udp == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address_udp;
    server_address_udp.sin_family = AF_INET;
    server_address_udp.sin_addr.s_addr = INADDR_ANY;
    server_address_udp.sin_port = htons(port);

    //legare
    if (bind(server_socket_udp, (struct sockaddr*)&server_address_udp,
        sizeof(server_address_udp)) == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    return server_socket_udp;
}

//functia care care primeste mesaje de la udp, tcp, stdin si trimite unde trebuie
void messages(int server_socket_tcp, int server_socket_udp,
    struct Client clients[MAX_CONNECTIONS], int num_clients) {
    char ids[50][50];
    for (int c = 0; c < 50; c++) {
        strcpy(ids[c], "");
    }
    struct pollfd poll_fds_tcp[MAX_CONNECTIONS];
    struct pollfd poll_fds_udp[MAX_CONNECTIONS];
    struct pollfd poll_fds_stdin[2];
    struct topic_struct topics[50];
    //initializari nule
    for (int i = 0; i < 50; i++) {
        strcpy(topics[i].name, "");
    }
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 50; j++) {
            memset(&topics[i].subscribers_id[j], 0, sizeof(struct pollfd) * 50);
        }
    }
    struct pollfd empty_pollfd;
    memset(&empty_pollfd, 0, sizeof(struct pollfd));

    // int numtopics = 0;
    int num_sockets_tcp = 1;
    int num_sockets_udp = 1;
    int num_sockets_stdin = 1;

    struct chat_packet received_packet;
    struct Udp_packet received_packet_udp;
    struct chat_packet sent_packet;
    sent_packet.len = 0;

    poll_fds_tcp[0].fd = server_socket_tcp;
    poll_fds_tcp[0].events = POLLIN;

    poll_fds_udp[0].fd = server_socket_udp;
    poll_fds_udp[0].events = POLLIN;

    poll_fds_stdin[0].fd = STDIN_FILENO;
    poll_fds_stdin[0].events = POLLIN;

    while (1) {
        //timeout 1
        int rc_tcp = poll(poll_fds_tcp, num_sockets_tcp, 1);
        int rc_udp = poll(poll_fds_udp, num_sockets_udp, 1);
        int rc_stdin = poll(poll_fds_stdin, num_sockets_stdin, 1);

        if (rc_tcp < 0 || rc_udp < 0 || rc_stdin < 0) {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_sockets_tcp; i++) {
            if (poll_fds_tcp[i].revents & POLLIN) {
                //gestionare mesaje tcp
                if (poll_fds_tcp[i].fd == server_socket_tcp) {
                    struct sockaddr_in client_address;
                    unsigned int size_client_address = sizeof(client_address);
                    const int newsockfd = accept(
                        server_socket_tcp, (struct sockaddr*)&client_address, &size_client_address);
                    if (newsockfd < 0) {
                        perror("Error");
                        exit(EXIT_FAILURE);
                    }

                    struct chat_packet id_packet;
                    int id_rc = recv_all(newsockfd, &id_packet, sizeof(id_packet));
                    if (id_rc < 0) {
                        perror("Error");
                        exit(EXIT_FAILURE);
                    }

                    int found_client = 0;
                    for (int b = 0; b < 50; b++) {
                        //compar vectorul de id-uri cu mesajul de la tcp(id)
                        if (strcmp(id_packet.message, ids[b]) == 0) {
                            printf("Client %s already connected.\n", id_packet.message);
                            //gasit
                            found_client = 1;
                            break;
                        }
                        if (strcmp(ids[b], "") == 0) {
                            strcpy(ids[b], id_packet.message);
                            break;
                        }
                    }

                    if (found_client) {
                        //daca e gasit inchid socketul
                        close(newsockfd);
                        break;
                    }
                    poll_fds_tcp[num_sockets_tcp].fd = newsockfd;
                    poll_fds_tcp[num_sockets_tcp].events = POLLIN;
                    num_sockets_tcp++;
                    //adaug clientul
                    strncpy(clients[num_clients].id, id_packet.message,
                        sizeof(clients[num_clients].id));
                    clients[num_clients].socket = newsockfd;
                    memcpy(&clients[num_clients].address, &client_address,
                        sizeof(client_address));
                    (num_clients)++;
                    printf("New client %s connected from %s:%d.\n", id_packet.message,
                        inet_ntoa(client_address.sin_addr),
                        ntohs(client_address.sin_port));
                    break;
                }
                else {
                    int rc;
                    while ((rc = recv_all(poll_fds_tcp[i].fd, &received_packet,
                        sizeof(received_packet))) > 0) {
                        //compar mesajul de la tcp cu exit 
                        //logica exit
                        if (strncmp(received_packet.message, "exit", 4) == 0) {
                            for (int j = 0; j < num_clients; j++) {
                                int deconectat = 0;
                                if (poll_fds_tcp[i].fd == clients[j].socket) {
                                    printf("Client %s disconnected.\n", clients[j].id);
                                    for (int b = 0; b < 50; b++) {
                                        if (strcmp(clients[j].id, ids[b]) == 0) {
                                            strcpy(ids[b], "");
                                            break;
                                        }
                                    }
                                    //inchid socketul clientului care da exit si scad numarul de clienti si de socketuri tcp
                                    close(poll_fds_tcp[i].fd);
                                    num_clients = num_clients - 1;
                                    if (j < num_clients) {
                                        clients[j] = clients[num_clients];
                                    }

                                    for (int k = i; k < num_sockets_tcp - 1; k++) {
                                        poll_fds_tcp[k] = poll_fds_tcp[k + 1];
                                    }

                                    num_sockets_tcp--;
                                    deconectat = 1;
                                    break;
                                }
                                if (deconectat) break;
                            }
                        }
                        //compar mesajul tcp cu subscribe
                        //logica subscribe
                        else if (strncmp(received_packet.message, "subscribe", 9) == 0) {
                            //in vectorul topic preiau ce gasesc dupa comanda subscribe ca text
                            char topic_name[1501];
                            memcpy(&topic_name, received_packet.message + 10, 1501);
                            for (int a = 0; a < 50; a++) {
                                //compar topicul cu ce topicuri exista 
                                if (strcmp(topics[a].name, topic_name) == 0) {
                                    for (int d = 0; d < 50; d++) {
                                        if (memcmp(&topics[a].subscribers_id[d], &empty_pollfd,
                                            sizeof(struct pollfd)) == 0) {
                                            memcpy(&topics[a].subscribers_id[d], &poll_fds_tcp[i],
                                                sizeof(struct pollfd));
                                            break;
                                        }
                                    }
                                    break;
                                }
                                if (strcmp(topics[a].name, "") == 0) {
                                    strcpy(topics[a].name, topic_name);
                                    for (int d = 0; d < 50; d++) {
                                        if (memcmp(&topics[a].subscribers_id[d], &empty_pollfd,
                                            sizeof(struct pollfd)) == 0) {
                                            memcpy(&topics[a].subscribers_id[d], &poll_fds_tcp[i],
                                                sizeof(struct pollfd));
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                            //trimit pachet de Subscribed catre tcp
                            strcpy(sent_packet.message, "Subscribed to topic");
                            send_all(poll_fds_tcp[i].fd, &sent_packet, sizeof(sent_packet));
                            break;
                        }
                        //logica unsubscribe
                        else if (strncmp(received_packet.message, "unsubscribe", 11) ==
                            0) {
                            //iau topicul ca fiind ce se gaseste dupa comanda unsubscribe
                            char topic_name[50];
                            memcpy(&topic_name, received_packet.message + 12, 40);
                            for (int a = 0; a < 50; a++) {
                                if (strcmp(topics[a].name, topic_name) == 0) {
                                    for (int d = 0; d < 50; d++) {
                                        if (memcmp(&topics[a].subscribers_id[d], &poll_fds_tcp[i],
                                            sizeof(struct pollfd)) == 0) {
                                            memset(&topics[a].subscribers_id[d], 0,
                                                sizeof(struct pollfd));
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                            //trimit mesaj de Unssubscribed catre tcp
                            strcpy(sent_packet.message, "Unsubscribed to topic");
                            send_all(poll_fds_tcp[i].fd, &sent_packet, sizeof(sent_packet));
                            break;
                        }
                        else {
                            printf("%s", received_packet.message);
                            break;
                        }
                    }
                }
            }
        }
        //gestionez mesajele de la udp
        for (int i = 0; i < num_sockets_udp; i++) {
            if (poll_fds_udp[i].revents & POLLIN) {
                if (poll_fds_udp[i].fd == server_socket_udp) {
                    struct sockaddr_in client_address;
                    unsigned int size_client_address = sizeof(client_address);
                    int rc1 = recvfrom(server_socket_udp, &received_packet.message,
                        sizeof(received_packet), 0,
                        (struct sockaddr*)&client_address, &size_client_address);
                    if (rc1 > 0) {
                        //iau topicul ca un mesaj normal si incep sa formez mesajul pentru tcp
                        memcpy(received_packet_udp.topic, received_packet.message, 50);
                        received_packet_udp.type = received_packet.message[50];
                        if (received_packet_udp.type == 0) {
                            strcpy(sent_packet.message, inet_ntoa(client_address.sin_addr));
                            strcat(sent_packet.message, ":");
                            char port_str[6];
                            snprintf(port_str, sizeof(port_str), "%d",
                                ntohs(client_address.sin_port));
                            strcat(sent_packet.message, port_str);
                            strcat(sent_packet.message, " - ");
                            strcat(sent_packet.message, received_packet_udp.topic);
                            strcat(sent_packet.message, " - INT - ");
                            char sign = received_packet.message[51];
                            //preiai valoarea cu specificatiile de uint32_t
                            uint32_t payload_value;
                            memcpy(&payload_value, received_packet.message + 52,
                                sizeof(payload_value));
                            payload_value = ntohl(payload_value);
                            if (sign == 1) {
                                strcat(sent_packet.message, "-");
                            }

                            char payload_str[20];
                            //fac ca valoarea sa fie trimisa string
                            int_to_string(payload_value, payload_str);
                            strcat(sent_packet.message, payload_str);

                        }
                        else if (received_packet_udp.type == 1) {
                            strcpy(sent_packet.message, inet_ntoa(client_address.sin_addr));
                            strcat(sent_packet.message, ":");
                            char port_str[6];
                            snprintf(port_str, sizeof(port_str), "%d",
                                ntohs(client_address.sin_port));
                            strcat(sent_packet.message, port_str);
                            strcat(sent_packet.message, " - ");
                            strcat(sent_packet.message, received_packet_udp.topic);
                            strcat(sent_packet.message, " - SHORT_REAL - ");
                            //preiau valoarea cu specificatiile de uint16_t
                            uint16_t payload_value;
                            memcpy(&payload_value, received_packet.message + 51,
                                sizeof(payload_value));
                            payload_value = (ntohs(payload_value));
                            char payload_str[20];
                            short_real_to_string(payload_value, payload_str);
                            strcat(sent_packet.message, payload_str);

                        }
                        else if (received_packet_udp.type == 2) {
                            strcpy(sent_packet.message, inet_ntoa(client_address.sin_addr));
                            strcat(sent_packet.message, ":");
                            char port_str[6];
                            snprintf(port_str, sizeof(port_str), "%d",
                                ntohs(client_address.sin_port));
                            strcat(sent_packet.message, port_str);
                            strcat(sent_packet.message, " - ");
                            strcat(sent_packet.message, received_packet_udp.topic);
                            strcat(sent_packet.message, " - FLOAT - ");
                            char sign = received_packet.message[51];
                            if (sign == 1) strcat(sent_packet.message, "-");
                            //preiau valoarea cu specificatiile de uint32_t
                            uint32_t abs;
                            memcpy(&abs, received_packet.message + 52,
                                sizeof(abs));
                            //formez mesajul in doua parti cat si rest
                            abs = ntohl(abs);
                            uint8_t power_of_10;
                            memcpy(&power_of_10, received_packet.message + 56,
                                sizeof(power_of_10));
                            int catul = abs / mypow(10, power_of_10);
                            int restul = abs % mypow(10, power_of_10);
                            char catul_str[20];
                            char restul_str[20];
                            int_to_string(catul, catul_str);
                            int_to_string(restul, restul_str);
                            //pun 0-urile necesare in plus daca e nevoie
                            if (catul == 0) {
                                strcat(sent_packet.message, "0.");
                            }
                            else {
                                strcat(sent_packet.message, catul_str);
                                if (restul != 0) strcat(sent_packet.message, ".");
                            }
                            while (power_of_10 > 0 && restul < mypow(10, power_of_10 - 1)) {
                                strcat(sent_packet.message, "0");
                                power_of_10--;
                            }
                            if (restul != 0) strcat(sent_packet.message, restul_str);
                        }
                        else if (received_packet_udp.type == 3) {
                            strcpy(sent_packet.message, inet_ntoa(client_address.sin_addr));
                            strcat(sent_packet.message, ":");
                            char port_str[6];
                            snprintf(port_str, sizeof(port_str), "%d",
                                ntohs(client_address.sin_port));
                            strcat(sent_packet.message, port_str);
                            strcat(sent_packet.message, " - ");
                            strcat(sent_packet.message, received_packet_udp.topic);
                            strcat(sent_packet.message, " - STRING - ");
                            //preiau valoarea cu specificatiile de la string
                            char string[1501];
                            memcpy(string, received_packet.message + 51, 1501);
                            string[strlen(string)] = '\0';
                            strncat(sent_packet.message, string, strlen(string));
                        }
                        received_packet_udp.topic[strlen(received_packet_udp.topic)] = '\n';
                        //trimit mesajul format catre tcp- urile abonate
                        for (int j = 1; j < num_sockets_tcp; j++) {
                            for (int a = 0; a < 50; a++) {
                                if (strcmp(received_packet_udp.topic, topics[a].name) == 0) {
                                    for (int e = 0; e < 50; e++) {
                                        if (topics[a].subscribers_id[e]->fd == poll_fds_tcp[j].fd)
                                            send_all(poll_fds_tcp[j].fd, &sent_packet,
                                                sizeof(sent_packet));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        //logica exit la server
        for (int i = 0; i < num_sockets_stdin; i++) {
            if (poll_fds_stdin[i].revents & POLLIN) {
                char input[100];
                if (fgets(input, sizeof(input), stdin) == NULL) {
                    break;
                }
                if (strncmp(input, "exit", 4) == 0) {
                    //inchid toti clientii apoi serverul
                    for (int j = 1; j < num_sockets_tcp; j++) {
                        close(poll_fds_tcp[j].fd);
                    }
                    for (int k = 1; k < num_sockets_udp; k++) {
                        close(poll_fds_udp[k].fd);
                    }
                    close(server_socket_udp);
                    close(server_socket_tcp);
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    //verific numarul de argumente dincomanda
    if (argc != 2) {
        perror("Error");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    int server_socket_tcp = start_tcp(port);
    int server_socket_udp = start_udp(port);

    struct Client clients[MAX_CONNECTIONS];
    int num_clients = 0;
    //apelez functia care gestioneaza mesajele
    messages(server_socket_tcp, server_socket_udp, clients,
        num_clients);
    close(server_socket_udp);
    close(server_socket_tcp);
    return 0;
}