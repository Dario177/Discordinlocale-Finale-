#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define NUMERO_CLIENT_MAX 3
#define LUNGHEZZA_MAX_MESSAGGIO 256

//Struttura per mantenere le informazioni di ciascun client
typedef struct {
    char nome_utente[20];
    char nome_canale[20];
    char percorso_pipe[50];
} Cliente;

//Array dei client registrati
Cliente lista_clienti[NUMERO_CLIENT_MAX];
int contatore_clienti = 0;

//Crea la fifo principale del server
void crea_pipe_server() {
    unlink("/tmp/chat_server");
    if (mkfifo("/tmp/chat_server", 0666) == -1) {
        perror("Errore creazione pipe server");
        exit(EXIT_FAILURE);
    }
}

//Inoltra il messaggio a tutti i client iscritti al canale specificato
void inoltra_messaggio_a_canale(const char* messaggio_originale, const char* canale_destinazione) {
    for (int i = 0; i < contatore_clienti; i++) {
        if (strcmp(lista_clienti[i].nome_canale, canale_destinazione) == 0) {
            int fd_client = open(lista_clienti[i].percorso_pipe, O_WRONLY);
            if (fd_client == -1) {
                //Se non c'è nessun lettore attivo, salta
                continue;
            }
            write(fd_client, messaggio_originale, strlen(messaggio_originale) + 1);
            close(fd_client);
        }
    }
}

int main() {
    crea_pipe_server();
    printf("Server in ascolto su /tmp/chat_server...\n");

    while (1) {
        //Apertura della pipe server per lettura
        int fd_server_lettura = open("/tmp/chat_server", O_RDONLY);
        if (fd_server_lettura == -1) {
            perror("Errore apertura pipe server");
            exit(EXIT_FAILURE);
        }

        char buffer_messaggio[LUNGHEZZA_MAX_MESSAGGIO];
        ssize_t byte_letti = read(fd_server_lettura, buffer_messaggio, sizeof(buffer_messaggio) - 1);
        close(fd_server_lettura);
        if (byte_letti <= 0) {
            continue;
        }
        buffer_messaggio[byte_letti] = '\0';

        //Copia per inoltro
        char copia_messaggio[LUNGHEZZA_MAX_MESSAGGIO];
        strcpy(copia_messaggio, buffer_messaggio);

        //Divisione del comando o messaggio
        char* token_comando = strtok(buffer_messaggio, " ");
        char* token_arg1 = strtok(NULL, " ");
        char* token_arg2 = strtok(NULL, "\0");

        if (strcmp(token_comando, "JOIN") == 0) {
            //Registrazione nuovo client
            strcpy(lista_clienti[contatore_clienti].nome_utente, token_arg1);
            strcpy(lista_clienti[contatore_clienti].nome_canale, token_arg2);
            sprintf(lista_clienti[contatore_clienti].percorso_pipe, "/tmp/client_%s", token_arg1);
            contatore_clienti++;
            printf("Nuovo client registrato: %s su canale %s\n", token_arg1, token_arg2);
        } else {
            //Messaggio broadcast sul canale
            //token_comando = username mittente, token_arg1 = canale, token_arg2 = testo
            printf("Inoltro: [%s] %s: %s\n", token_arg1, token_comando, token_arg2);
            inoltra_messaggio_a_canale(copia_messaggio, token_arg1);
        }
    }

    return 0;
}
