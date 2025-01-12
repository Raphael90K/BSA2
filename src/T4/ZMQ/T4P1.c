#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define SERVER_ADDRESS "tcp://172.18.0.2:5555" // IP des Servers im Docker-Netzwerk im Container
#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 1000

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

void perform_measurements() {
    long min_times[NUM_REPEATS];

    // ZeroMQ-Context und Socket erstellen
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REQ);
    if (zmq_connect(socket, SERVER_ADDRESS) != 0) {
        perror("Fehler beim Verbinden mit dem Server");
        zmq_close(socket);
        zmq_ctx_term(context);
        exit(EXIT_FAILURE);
    }

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        long min_time = LONG_MAX;

        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            char message[] = "ready";
            char response[16];
            long start_time = get_time_in_nanoseconds();

            // Nachricht senden
            if (zmq_send(socket, message, strlen(message), 0) < 0) {
                perror("Fehler beim Senden der Nachricht");
                continue;
            }

            // Antwort empfangen
            if (zmq_recv(socket, response, sizeof(response), 0) < 0) {
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
    FILE *csv_file = fopen("T4_zmq_min_times.csv", "w");
    if (!csv_file) {
        perror("Fehler beim Öffnen der CSV-Datei");
        zmq_close(socket);
        zmq_ctx_term(context);
        exit(EXIT_FAILURE);
    }

    fprintf(csv_file, "id,mintime\n");
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        fprintf(csv_file, "%d,%ld\n", repeat + 1, min_times[repeat]);
    }
    fclose(csv_file);

    printf("Ergebnisse in 'T4_zmq_min_times.csv' gespeichert.\n");

    zmq_close(socket);
    zmq_ctx_term(context);
}

int main() {
    perform_measurements();
    return 0;
}
