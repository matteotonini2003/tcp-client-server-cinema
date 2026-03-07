//--------------------------------------------------CLIENT CORRETTO--------------------------------------------------
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
#define MAX_MESSAGE_LEN     1024
#define MAX_PASSWORD_LEN    100
#define MAX_NAME_LEN        256

// per gestione CTRL+C
volatile sig_atomic_t ctrlc_flag = 0;

void sigint_handler(int signo) {
    (void)signo;
    ctrlc_flag = 1;
}

// send_all / recv_all come nel server (versione client)
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

int main(int argc, char *argv[]) {
    // registra handler SIGINT
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        fprintf(stderr, "Impossibile registrare l'handler del segnale: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // parsing argomenti riga di comando
    char *addr_str = (char *)DEFAULT_ADDR;
    int num_port = DEFAULT_PORT_NUMBER;

    if (argc >= 2) {
        addr_str = argv[1];
    }
    if (argc >= 3) {
        num_port = atoi(argv[2]);
        if (num_port <= 0 || num_port > 65535) {
            fprintf(stderr, "Errore: numero di porta non valido (%s).\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    }

    // creazione socket
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        fprintf(stderr, "Impossibile creare il socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    in_addr_t address;
    if (inet_pton(AF_INET, addr_str, &address) <= 0) {
        fprintf(stderr, "Impossibile convertire l'indirizzo '%s': %s\n", addr_str, strerror(errno));
        close(sd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(num_port);
    sa.sin_addr.s_addr = address;

    if (connect(sd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        fprintf(stderr, "Impossibile connettersi al server: %s\n", strerror(errno));
        close(sd);
        exit(EXIT_FAILURE);
    }

    printf("Connesso al server [indirizzo %s ; porta %d]\n", addr_str, num_port);
    printf("Benvenuto al multisala!\n");

    char command[MAX_COMMAND];
    char input[MAX_COMMAND];
    char name[MAX_NAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char new_pass[MAX_PASSWORD_LEN];
    char message[MAX_MESSAGE_LEN];
    char pass_req[MAX_MESSAGE_LEN];

    int nsala, nposti;

    while (1) {
        nsala = 0;
        nposti = 0;
        memset(command, 0, sizeof(command));
        memset(input, 0, sizeof(input));
        memset(name, 0, sizeof(name));
        memset(password, 0, sizeof(password));
        memset(new_pass, 0, sizeof(new_pass));
        memset(message, 0, sizeof(message));
        memset(pass_req, 0, sizeof(pass_req));

        // lettura comando
        int comando_valido = 0;
        while (!comando_valido) {
            printf("bys> ");
            fflush(stdout);

            if (fgets(input, sizeof(input), stdin) == NULL) {
                // EOF o errore
                strcpy(command, "quit");
                comando_valido = 1;
                break;
            }
            input[strcspn(input, "\n")] = '\0';

            if (ctrlc_flag) {
                // se è arrivato CTRL+C, simulo quit
                strcpy(command, "quit");
                comando_valido = 1;
                break;
            }

            int count = sscanf(input, "%s %d %d", command, &nsala, &nposti);
            if (count >= 1 && strlen(command) > 0) {
                comando_valido = 1;
            }
        }

        // invio nsala
        uint32_t nsala_net = htonl((uint32_t)nsala);
        if (send_all(sd, &nsala_net, sizeof(nsala_net)) < 0) {
            fprintf(stderr, "Impossibile inviare il numero di sala al server: %s\n", strerror(errno));
            break;
        }

        // invio nposti
        uint32_t nposti_net = htonl((uint32_t)nposti);
        if (send_all(sd, &nposti_net, sizeof(nposti_net)) < 0) {
            fprintf(stderr, "Impossibile inviare il numero di posti al server: %s\n", strerror(errno));
            break;
        }

        // invio lunghezza comando
        uint32_t len_net = htonl((uint32_t)strlen(command));
        if (send_all(sd, &len_net, sizeof(len_net)) < 0) {
            fprintf(stderr, "Impossibile inviare la lunghezza del comando al server: %s\n", strerror(errno));
            break;
        }

        // invio comando
        if (send_all(sd, command, strlen(command)) < 0) {
            fprintf(stderr, "Impossibile inviare il comando al server: %s\n", strerror(errno));
            break;
        }

        // --- gestione input extra in base al comando ---

        // book / cancel → nominativo
        if (strncmp(command, "book", 4) == 0 ||
            strncmp(command, "cancel", 6) == 0) {

            printf("> Inserisci nome e cognome:\n");
            if (fgets(name, sizeof(name), stdin) == NULL) {
                strcpy(name, "quit");
            }
            name[strcspn(name, "\n")] = '\0';

            if (ctrlc_flag) {
                strcpy(name, "quit");
            }

            uint32_t name_len_net = htonl((uint32_t)strlen(name));
            if (send_all(sd, &name_len_net, sizeof(name_len_net)) < 0) {
                fprintf(stderr, "Impossibile inviare la lunghezza del nome al server: %s\n", strerror(errno));
                break;
            }
            if (send_all(sd, name, strlen(name)) < 0) {
                fprintf(stderr, "Impossibile inviare il nome al server: %s\n", strerror(errno));
                break;
            }
        }

        // open / close → password admin
        else if (strncmp(command, "open", 4) == 0 ||
                 strncmp(command, "close", 5) == 0) {

            printf("> Inserisci la password di amministratore:\n");
            if (fgets(password, sizeof(password), stdin) == NULL) {
                strcpy(password, "quit");
            }
            password[strcspn(password, "\n")] = '\0';

            if (ctrlc_flag) {
                strcpy(password, "quit");
            }

            uint32_t pass_len_net = htonl((uint32_t)strlen(password));
            if (send_all(sd, &pass_len_net, sizeof(pass_len_net)) < 0) {
                fprintf(stderr, "Impossibile inviare la lunghezza della password al server: %s\n", strerror(errno));
                break;
            }
            if (send_all(sd, password, strlen(password)) < 0) {
                fprintf(stderr, "Impossibile inviare la password al server: %s\n", strerror(errno));
                break;
            }
        }

        // changepwd → vecchia password + nuova password
        else if (strncmp(command, "changepwd", 9) == 0) {
            printf("> Inserisci la vecchia password di amministratore:\n");
            if (fgets(password, sizeof(password), stdin) == NULL) {
                strcpy(password, "quit");
            }
            password[strcspn(password, "\n")] = '\0';

            if (ctrlc_flag) {
                strcpy(password, "quit");
            }

            uint32_t pass_len_net = htonl((uint32_t)strlen(password));
            if (send_all(sd, &pass_len_net, sizeof(pass_len_net)) < 0) {
                fprintf(stderr, "Impossibile inviare la lunghezza della password al server: %s\n", strerror(errno));
                break;
            }
            if (send_all(sd, password, strlen(password)) < 0) {
                fprintf(stderr, "Impossibile inviare la password al server: %s\n", strerror(errno));
                break;
            }

            // attendo eventuale richiesta "Inserisci la nuova password..."
            uint32_t req_len_net;
            ssize_t r = recv_all(sd, &req_len_net, sizeof(req_len_net));
            if (r <= 0) {
                fprintf(stderr, "Connessione chiusa o errore durante la ricezione della richiesta nuova password.\n");
                break;
            }
            int req_len = (int)ntohl(req_len_net);
            if (req_len <= 0 || req_len >= MAX_MESSAGE_LEN) {
                fprintf(stderr, "Messaggio di richiesta nuova password non valido.\n");
                break;
            }
            r = recv_all(sd, pass_req, (size_t)req_len);
            if (r <= 0) {
                fprintf(stderr, "Errore durante la ricezione della richiesta nuova password.\n");
                break;
            }
            pass_req[req_len] = '\0';

            if (strncmp(pass_req, "> Inserisci la nuova password di amministratore:\n", 49) == 0) {
                printf("%s", pass_req);

                if (fgets(new_pass, sizeof(new_pass), stdin) == NULL) {
                    strcpy(new_pass, "quit");
                }
                new_pass[strcspn(new_pass, "\n")] = '\0';

                uint32_t new_pass_len_net = htonl((uint32_t)strlen(new_pass));
                if (send_all(sd, &new_pass_len_net, sizeof(new_pass_len_net)) < 0) {
                    fprintf(stderr, "Impossibile inviare la lunghezza della nuova password al server: %s\n", strerror(errno));
                    break;
                }
                if (send_all(sd, new_pass, strlen(new_pass)) < 0) {
                    fprintf(stderr, "Impossibile inviare la nuova password al server: %s\n", strerror(errno));
                    break;
                }
            } else {
                // il server ha risposto subito con un errore (es. password vecchia errata)
                // in questo caso "pass_req" contiene già il messaggio da mostrare
                printf("%s\n", pass_req);
                // e saltiamo la normale ricezione del messaggio generico sotto,
                // che comunque arriverà (il server segue il suo flusso).
            }
        }

        // ricezione risposta server (tutti i comandi)
        uint32_t msg_len_net;
        ssize_t r = recv_all(sd, &msg_len_net, sizeof(msg_len_net));
        if (r <= 0) {
            fprintf(stderr, "Connessione chiusa dal server o errore nella ricezione.\n");
            break;
        }
        int msg_len = (int)ntohl(msg_len_net);
        if (msg_len <= 0 || msg_len >= MAX_MESSAGE_LEN) {
            fprintf(stderr, "Messaggio di risposta non valido (lunghezza errata).\n");
            break;
        }
        r = recv_all(sd, message, (size_t)msg_len);
        if (r <= 0) {
            fprintf(stderr, "Errore nella ricezione del messaggio dal server.\n");
            break;
        }
        message[msg_len] = '\0';

        printf("%s\n", message);

        if (strncmp(command, "quit", 4) == 0) {
            break;
        }
    }

    close(sd);
    return 0;
}
