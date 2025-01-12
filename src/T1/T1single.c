#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

// Spinlock-Definition
atomic_int spinlock = 0;
atomic_int turn = 0; // 0: Thread A, 1: Thread B

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // Hohe Präzision, monotone Uhr
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Spinlock-Operationen
void acquire_spinlock() {
    while (atomic_exchange(&spinlock, 1)) {
        // Busy-wait (spin)
    }
}

void release_spinlock() {
    atomic_store(&spinlock, 0);
}

// Globale Variablen für die Ergebnisse
#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 100

long thread_a_times[NUM_MEASUREMENTS];
long min_times[NUM_REPEATS]; // Mindestzeiten aus jeder Wiederholung

// Thread A: Führt die Zeitmessung durch
void *thread_a_func(void *arg) {
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Warten, bis Thread A an der Reihe ist
            while (atomic_load(&turn) != 0);

            // Erwerbe den Lock
            acquire_spinlock();
            long start_time = get_time_in_nanoseconds();

            // Gib den Lock frei und übergebe die Kontrolle an Thread B
            release_spinlock();
            atomic_store(&turn, 1); // Thread B ist an der Reihe

            // Messen, wie lange es dauert, bis Thread A den Lock wiedererhält
            acquire_spinlock();
            long end_time = get_time_in_nanoseconds();

            // Speichere die gemessene Zeit
            thread_a_times[i] = end_time - start_time;

            // Gib den Lock frei
            release_spinlock();
        }

        // Mindestzeit berechnen
        long min_time = LONG_MAX;
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            if (thread_a_times[i] < min_time) {
                min_time = thread_a_times[i];
            }
        }
        min_times[repeat] = min_time; // Mindestzeit der aktuellen Wiederholung speichern
    }
    return NULL;
}

// Thread B: Unterstützt den Test
void *thread_b_func(void *arg) {
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Warten, bis Thread B an der Reihe ist
            while (atomic_load(&turn) != 1);

            // Erwerbe den Lock
            acquire_spinlock();

            // Gib den Lock frei und übergebe die Kontrolle an Thread A
            release_spinlock();
            atomic_store(&turn, 0); // Thread A ist an der Reihe
        }
    }
    return NULL;
}

int main() {
    pthread_t thread_a, thread_b;

    // Starte beide Threads
    pthread_create(&thread_a, NULL, thread_a_func, NULL);
    pthread_create(&thread_b, NULL, thread_b_func, NULL);

    // Warten auf beide Threads
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);

    // Ergebnisse in eine CSV-Datei speichern
    FILE *csv_file = fopen("T1_spinlock_min_times.csv", "w");
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

    // Datei schließen
    fclose(csv_file);

    printf("Ergebnisse in 'T1_spinlock_min_times.csv' gespeichert.\n");

    return 0;
}
