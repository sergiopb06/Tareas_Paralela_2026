//
// Created by Sleyter Angulo on 9/14/26.
//

#include "../includes/net_util.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int nu_listen(unsigned short port, int backlog)
{
                        //socket(int domain, int type, int protocol)
    int file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
                            //AF INET = Internet domain sockets
                            //SOCK_STREAM = Byte-stream socket

    if (file_descriptor < 0)
    {
        perror("socket");
        return -1;
    }

    int yes = 1;
        //setsockpot(int socket, int level, int option_name, const void *option_value, socklen_t option_len)
    if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        //SOL_SOCKET = Options to be accessed at socket level, not protocol level
        //SO_REUSEADDR = Reuse of local addresses is supported. 

    {
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in server_address; //Create struct type sockaddr_in, IPv4 protocol
    memset(&server_address, 0, sizeof(server_address));//Cleans up memory with 0's
    server_address.sin_family = AF_INET; //Tells OS its using IPv4 conection
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);//Tells server to use any available IP. (htonl = host to network long, from little Endian to Big Endian)
    server_address.sin_port = htons(port);//Assigns number of port (htons = host to network short)

        //bind(int socket, const struct sockaddr *address, socklen_t addres_len)
    if (bind(file_descriptor, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind");
        close(file_descriptor);
        return -1;
    }

        //listen(int socket, int backlog)
    if (listen(file_descriptor, backlog) < 0)
    {
        perror("listen");
        close(file_descriptor);
        return -1;
    }

    return file_descriptor;
}

int nu_write_all(int file_descriptor, const void* buffer, size_t size)
{
    const char *p = buffer;
    size_t sent = 0;

    while (sent < size) {
        ssize_t n = write(file_descriptor, p + sent, size - sent);
        if (n < 0) {
            if (errno == EINTR)
                continue;          /* interrupted by a signal: retry */
            perror("write");
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

ssize_t nu_drain_request(int file_descriptor)
{
    char buffer[2048];
    ssize_t n;

    do {
        n = read(file_descriptor, buffer, sizeof buffer);
    } while (n < 0 && errno == EINTR);

    if (n < 0)
        perror("read");

    return n;
}

int nu_send_response(int file_descriptor, unsigned long connection_id)
{
    char body[128]; //Return data to user
    char header[256]; //Connection data

    int body_len = snprintf(body, sizeof body,
                            "connection %lu handled\n", connection_id);
    if (body_len < 0 || (size_t)body_len >= sizeof body) {
        fprintf(stderr, "nu_send_response: body truncated\n");
        return -1;
    }

    int header_len = snprintf(header, sizeof header,
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: %d\r\n"
                              "Connection: close\r\n"
                              "\r\n",
                              body_len);
    if (header_len < 0 || (size_t)header_len >= sizeof header) {
        fprintf(stderr, "nu_send_response: header truncated\n");
        return -1;
    }

    if (nu_write_all(file_descriptor, header, (size_t)header_len) < 0)
        return -1;

    return nu_write_all(file_descriptor, body, (size_t)body_len);
}