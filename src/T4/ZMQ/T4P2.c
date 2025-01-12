#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERVER_ADDRESS "tcp://*:5555"

void run_server() {
    // ZeroMQ-Context und Socket erstellen
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REP);
    if (zmq_bind(socket, SERVER_ADDRESS) != 0) {
        perror("Fehler beim Binden des Sockets");
        zmq_close(socket);
        zmq_ctx_term(context);
        exit(EXIT_FAILURE);
    }

    printf("Server läuft und wartet auf Nachrichten...\n");

    char buffer[16];
    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Nachricht empfangen
        if (zmq_recv(socket, buffer, sizeof(buffer), 0) < 0) {
            perror("Fehler beim Empfang der Nachricht");
            break;
        }

        // Antwort senden
        if (zmq_send(socket, buffer, strlen(buffer), 0) < 0) {
            perror("Fehler beim Senden der Antwort");
            break;
        }
    }

    zmq_close(socket);
    zmq_ctx_term(context);
}

int main() {
    run_server();
    return 0;
}
