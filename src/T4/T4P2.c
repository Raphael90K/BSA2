#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 5555
#define BACKLOG 1 // Maximale Anzahl gleichzeitiger Verbindungen

void run_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Server-Socket erstellen
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fehler beim Erstellen des Sockets");
        exit(EXIT_FAILURE);
    }

    // Serveradresse konfigurieren
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    // Socket binden
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Fehler beim Binden des Sockets");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Lauschen auf Verbindungen
    if (listen(server_socket, BACKLOG) < 0) {
        perror("Fehler beim Lauschen auf Verbindungen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Server läuft und wartet auf Verbindungen...\n");

    // Verbindung akzeptieren
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (client_socket < 0) {
        perror("Fehler beim Akzeptieren der Verbindung");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Client verbunden.\n");

    char buffer[16];
    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Nachricht empfangen
        if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
            printf("Verbindung geschlossen oder Fehler beim Empfang.\n");
            break;
        }

        // Antwort senden
        if (send(client_socket, buffer, sizeof(buffer), 0) < 0) {
            perror("Fehler beim Senden der Antwort");
            break;
        }
    }

    close(client_socket);
    close(server_socket);
}

int main() {
    run_server();
    return 0;
}
