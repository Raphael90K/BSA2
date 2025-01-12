#include <stdio.h>
#include <zmq.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 1000
long min_times[NUM_REPEATS]; // Mindestzeiten für jede Wiederholung

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // Hohe Präzision, monotone Uhr
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main() {
    void *context = zmq_ctx_new();
    void *socket_send = zmq_socket(context, ZMQ_PUSH);
    zmq_bind(socket_send, "ipc:///tmp/zmq_ipc");

    void *socket_recv = zmq_socket(context, ZMQ_PULL);
    zmq_connect(socket_recv, "ipc:///tmp/zmq_ipc");

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        long min_time = LONG_MAX;

        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            char message[16] = "ready";

            // Zeit messen: Nachricht senden und empfangen
            long start_time = get_time_in_nanoseconds();
            zmq_send(socket_send, message, sizeof(message), 0);
            zmq_recv(socket_recv, message, sizeof(message), 0);
            long end_time = get_time_in_nanoseconds();

            // Berechne und aktualisiere Mindestzeit
            long duration = end_time - start_time;
            if (duration < min_time) {
                min_time = duration;
            }
        }

        min_times[repeat] = min_time;
    }

    zmq_close(socket_send);
    zmq_close(socket_recv);
    zmq_ctx_destroy(context);

    // Ergebnisse in eine CSV-Datei speichern
    FILE *csv_file = fopen("T3_zmq_ipc_min_times.csv", "w");
    if (!csv_file) {
        perror("Fehler beim Öffnen der Datei");
        return 1;
    }

    fprintf(csv_file, "id,mintime\n");
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        fprintf(csv_file, "%d,%ld\n", repeat + 1, min_times[repeat]);
    }
    fclose(csv_file);

    printf("Ergebnisse in 'T3_zmq_ipc_min_times.csv' gespeichert.\n");
    return 0;
}
