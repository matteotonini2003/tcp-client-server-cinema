// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ctype.h>

#define DEFAULT_PORT_NUMBER 1234
#define MAX_COMMAND        100
#define MAX_MESSAGE_LEN    1024
#define MAX_PASSWORD_LEN   100
#define MAX_THREAD         5
#define NUM_SALE           5
#define SEATS_PER_SALA     50
#define MAX_NAME_LEN       256
#define MAX_NUM_BOOKS      250

/* ------------------------------- STATO GLOBALE ---------------------------------- */

int active_threads = 0;

int rem_sits[NUM_SALE] = {SEATS_PER_SALA, SEATS_PER_SALA, SEATS_PER_SALA, SEATS_PER_SALA, SEATS_PER_SALA};

int prenotazioni[NUM_SALE] = {1, 1, 1, 1, 1};

struct check {
    int  num_sala;
    char check_name[MAX_NAME_LEN];
    int  tck;
};

struct check books[MAX_NUM_BOOKS];
int books_count = 0;

/* ------------------------------- SINCRONIZZAZIONE ---------------------------------- */

pthread_mutex_t threads_mtx;
pthread_cond_t  threads_cnd;

pthread_mutex_t state_mtx;

pthread_mutex_t file_mtx;

FILE *fp = NULL;

/* ------------------------------- STRUTTURA ARGOMENTI THREAD ---------------------------------- */

struct t_args {
    int sock_desc;
    struct sockaddr_in addr;
};

/* ------------------------------- FUNZIONI DI SUPPORTO ---------------------------------- */

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

/* ... (il resto del file rimane IDENTICO al tuo server) ... */
