// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define DEFAULT_ADDR        "127.0.0.1"
#define DEFAULT_PORT_NUMBER 1234
#define MAX_COMMAND         100
#define MAX_MESSAGE_LEN    1024
#define MAX_PASSWORD_LEN   100
#define MAX_NAME_LEN       256

volatile sig_atomic_t ctrlc_flag = 0;

void sigint_handler(int signo) {
    (void)signo;
    ctrlc_flag = 1;
}

ssize_t recv_all(int sd, void *buf, size_t len) {
    size_t received = 0;
    char *p = buf;
    while (received < len) {
        ssize_t r = recv(sd, p + received, len - received, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;
        received += (size_t)r;
    }
    return (ssize_t)received;
}

ssize_t send_all(int sd, const void *buf, size_t len) {
    size_t sent = 0;
    const char *p = buf;
    while (sent < len) {
        ssize_t s = send(sd, p + sent, len - sent, 0);
        if (s < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)s;
    }
    return (ssize_t)sent;
}

/* ... (anche qui il resto è IDENTICO al tuo client) ... */
