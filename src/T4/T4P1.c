#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

#define SERVER_IP "172.18.0.2"    // Hostname des Servers im Docker-Netzwerk
#define SERVER_PORT 5555      // Port des Servers
#define NUM_MEASUREMENTS 100  // Anzahl der Messungen pro Wiederholung
#define NUM_REPEATS 100       // Anzahl der Wiederholungen

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Funktion für die Zeitmessungen
void perform_measurements() {
    long min_times[NUM_REPEATS];
    struct sockaddr_in server_address;

    // Socket erstellen
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Fehler beim Erstellen des Sockets");
        exit(EXIT_FAILURE);
    }

    // Serveradresse konfigurieren
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Ungültige Adresse oder Adresse nicht unterstützt");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Verbindung zum Server herstellen
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Verbindung zum Server fehlgeschlagen");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        long min_time = LONG_MAX;

        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            char message[16] = "ready";
            char response[16];
            long start_time = get_time_in_nanoseconds();

            // Nachricht senden
            if (send(client_socket, message, strlen(message), 0) < 0) {
                perror("Fehler beim Senden der Nachricht");
                continue;
            }

            // Antwort empfangen
            if (recv(client_socket, response, sizeof(response), 0) < 0) {
                perror("Fehler beim Empfang der Antwort");
                continue;
            }

            long end_time = get_time_in_nanoseconds();
            long duration = end_time - start_time;
            if (duration < min_time) {
                min_time = duration;
            }
        }

        min_times[repeat] = min_time;
    }

    // Ergebnisse in CSV-Datei speichern
    FILE *csv_file = fopen("tcp_min_times.csv", "w");
    if (!csv_file) {
        perror("Fehler beim Öffnen der CSV-Datei");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    fprintf(csv_file, "id,mintime\n");
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        fprintf(csv_file, "%d,%ld\n", repeat + 1, min_times[repeat]);
    }
    fclose(csv_file);

    printf("Ergebnisse in 'tcp_min_times.csv' gespeichert.\n");
    close(client_socket);
}

int main() {
    perform_measurements();
    return 0;
}
