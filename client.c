#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define LUNGHEZZA_MAX_MESSAGGIO 256

//Crea una fifo per il client
void crea_pipe_client(const char* percorso_pipe) {
    unlink(percorso_pipe);
    if (mkfifo(percorso_pipe, 0666) == -1) {
        perror("Errore creazione pipe client");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nome_utente> <canale>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* nome_utente = argv[1];
    char* nome_canale = argv[2];
    char percorso_pipe[64];
    sprintf(percorso_pipe, "/tmp/client_%s", nome_utente);

    crea_pipe_client(percorso_pipe);

    //Registrazione al server
    int fd_server_scrittura = open("/tmp/chat_server", O_WRONLY);
    if (fd_server_scrittura == -1) {
        perror("Errore apertura pipe server");
        exit(EXIT_FAILURE);
    }
    char messaggio_join[LUNGHEZZA_MAX_MESSAGGIO];
    sprintf(messaggio_join, "JOIN %s %s", nome_utente, nome_canale);
    write(fd_server_scrittura, messaggio_join, strlen(messaggio_join) + 1);
    close(fd_server_scrittura);

    printf("Connesso al canale %s. Scrivi messaggi:\n", nome_canale);

    //Processo figlio: ricezione messaggi
    if (fork() == 0) {
        while (1) {
            int fd_pipe_letto = open(percorso_pipe, O_RDONLY);
            if (fd_pipe_letto == -1) {
                perror("Errore apertura pipe client");
                exit(EXIT_FAILURE);
            }

            char buffer_ricezione[LUNGHEZZA_MAX_MESSAGGIO];
            ssize_t bytes_letti = read(fd_pipe_letto, buffer_ricezione, sizeof(buffer_ricezione) - 1);
            close(fd_pipe_letto);
            if (bytes_letti <= 0) continue;

            buffer_ricezione[bytes_letti] = '\0';

            //Parsing del messaggio: utente, canale, testo
            char* utente_ricevuto = strtok(buffer_ricezione, " ");
            char* canale_ricevuto = strtok(NULL, " ");
            char* testo_ricevuto = strtok(NULL, "\0");

            printf("\n[%s] %s: %s\n> ", canale_ricevuto, utente_ricevuto, testo_ricevuto);
            fflush(stdout);
        }
    }

    //Processo padre: invio messaggi
    while (1) {
        printf("> ");
        char messaggio_invio[LUNGHEZZA_MAX_MESSAGGIO];
        if (!fgets(messaggio_invio, sizeof(messaggio_invio), stdin)) break;
        messaggio_invio[strcspn(messaggio_invio, "\n")] = '\0';

        char messaggio_completo[LUNGHEZZA_MAX_MESSAGGIO];
        sprintf(messaggio_completo, "%s %s %s", nome_utente, nome_canale, messaggio_invio);

        int fd_server2 = open("/tmp/chat_server", O_WRONLY);
        if (fd_server2 == -1) {
            perror("Errore apertura pipe server");
            exit(EXIT_FAILURE);
        }
        write(fd_server2, messaggio_completo, strlen(messaggio_completo) + 1);
        close(fd_server2);
    }

    return 0;
}