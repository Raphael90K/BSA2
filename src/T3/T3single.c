#include <stdio.h>
#include <pthread.h>
#include <zmq.h>
#include <time.h>
#include <limits.h>
#include <string.h>

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // Hohe Präzision, monotone Uhr
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Globale Variablen
#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 100
long min_times[NUM_REPEATS]; // Mindestzeiten für jede Wiederholung

// ZeroMQ-Kontext (wird von beiden Threads geteilt)
void *context;

// Thread A: Zeitmessung durchführen
void *thread_a_func(void *arg) {
    void *socket_send = zmq_socket(context, ZMQ_PUSH); // Sender-Socket
    zmq_connect(socket_send, "inproc://to_b");

    void *socket_recv = zmq_socket(context, ZMQ_PULL); // Empfänger-Socket
    zmq_bind(socket_recv, "inproc://to_a");

    // Timeout für empfangen einstellen
    int timeout = 1000; // 1000 ms
    zmq_setsockopt(socket_recv, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        long min_time = LONG_MAX;

        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            char message[16] = "ready";

            // Zeit messen: Nachricht senden und empfangen
            long start_time = get_time_in_nanoseconds();

            if (zmq_send(socket_send, message, strlen(message), 0) == -1) {
                printf("Thread A: Fehler beim Senden der Nachricht.\n");
                continue;
            }

            if (zmq_recv(socket_recv, message, sizeof(message), 0) == -1) {
                printf("Thread A: Fehler beim Empfangen der Nachricht.\n");
                continue;
            }

            long end_time = get_time_in_nanoseconds();

            // Berechne und aktualisiere Mindestzeit
            long duration = end_time - start_time;
            if (duration < min_time) {
                min_time = duration;
            }
        }

        // Speichere die minimale Zeit der aktuellen Wiederholung
        min_times[repeat] = min_time;
    }

    zmq_close(socket_send);
    zmq_close(socket_recv);

    return NULL;
}

// Thread B: Nachrichten empfangen und zurücksenden
void *thread_b_func(void *arg) {
    void *socket_recv = zmq_socket(context, ZMQ_PULL); // Empfänger-Socket
    zmq_bind(socket_recv, "inproc://to_b");

    void *socket_send = zmq_socket(context, ZMQ_PUSH); // Sender-Socket
    zmq_connect(socket_send, "inproc://to_a");

    // Timeout für empfangen einstellen
    int timeout = 1000; // 1000 ms
    zmq_setsockopt(socket_recv, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    char message[16];
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            if (zmq_recv(socket_recv, message, sizeof(message), 0) == -1) {
                printf("Thread B: Fehler beim Empfangen der Nachricht.\n");
                continue;
            }

            if (zmq_send(socket_send, message, strlen(message), 0) == -1) {
                printf("Thread B: Fehler beim Senden der Nachricht.\n");
                continue;
            }
        }
    }

    zmq_close(socket_send);
    zmq_close(socket_recv);

    return NULL;
}

int main() {
    pthread_t thread_a, thread_b;

    // Erstelle den gemeinsamen ZeroMQ-Kontext
    context = zmq_ctx_new();

    // Starte beide Threads
    pthread_create(&thread_a, NULL, thread_a_func, NULL);
    pthread_create(&thread_b, NULL, thread_b_func, NULL);

    // Warten auf beide Threads
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);

    // Ergebnisse in eine CSV-Datei speichern
    FILE *csv_file = fopen("zmq_inproc_min_times.csv", "w");
    if (!csv_file) {
        perror("Fehler beim Öffnen der Datei");
        return 1;
    }

    // CSV-Header schreiben
    fprintf(csv_file, "id,mintime\n");

    // Schreibe die Mindestzeiten der Wiederholungen
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        fprintf(csv_file, "%d,%ld\n", repeat + 1, min_times[repeat]);
    }

    fclose(csv_file);
    printf("Ergebnisse in 'zmq_inproc_min_times.csv' gespeichert.\n");

    // ZeroMQ-Kontext zerstören
    zmq_ctx_destroy(context);

    return 0;
}
