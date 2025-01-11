#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <limits.h>

// Globale Variablen
#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 100

long thread_a_times[NUM_MEASUREMENTS];
long min_times[NUM_REPEATS];

// Semaphoren für Synchronisation
sem_t sem_a; // Signal für Thread A
sem_t sem_b; // Signal für Thread B

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Thread A: Führt die Zeitmessung durch
void *thread_a_func(void *arg) {
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Warten auf Signal von Thread B
            sem_wait(&sem_a);

            // Zeitmessung starten
            long start_time = get_time_in_nanoseconds();

            // Signal an Thread B senden
            sem_post(&sem_b);

            // Warten auf Signal von Thread B, um Zeitmessung zu stoppen
            sem_wait(&sem_a);

            // Zeitmessung stoppen
            long end_time = get_time_in_nanoseconds();
            thread_a_times[i] = end_time - start_time;

            // Signal an Thread B senden, um nächsten Zyklus zu starten
            sem_post(&sem_b);
        }

        // Mindestzeit berechnen
        long min_time = LONG_MAX;
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            if (thread_a_times[i] < min_time) {
                min_time = thread_a_times[i];
            }
        }
        min_times[repeat] = min_time; // Speichere Mindestzeit der aktuellen Wiederholung
    }
    return NULL;
}

// Thread B: Unterstützt den Test
void *thread_b_func(void *arg) {
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Signal an Thread A senden
            sem_post(&sem_a);

            // Warten auf Signal von Thread A
            sem_wait(&sem_b);

            // Signal an Thread A senden
            sem_post(&sem_a);

            // Warten auf Signal von Thread A
            sem_wait(&sem_b);
        }
    }
    return NULL;
}

int main() {
    pthread_t thread_a, thread_b;

    // Semaphoren initialisieren
    sem_init(&sem_a, 0, 0); // Startwert 0: Thread A wartet initial
    sem_init(&sem_b, 0, 1); // Startwert 1: Thread B beginnt

    // Threads starten
    pthread_create(&thread_a, NULL, thread_a_func, NULL);
    pthread_create(&thread_b, NULL, thread_b_func, NULL);

    // Warten auf Threads
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);

    // Ergebnisse in eine CSV-Datei schreiben
    FILE *csv_file = fopen("T2_semaphore_min_times.csv", "w");
    if (!csv_file) {
        perror("Fehler beim Öffnen der Datei");
        return 1;
    }

    // CSV-Header schreiben
    fprintf(csv_file, "id,mintime\n");

    // Mindestzeiten der Wiederholungen schreiben
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        fprintf(csv_file, "%d,%ld\n", repeat + 1, min_times[repeat]);
    }

    // Datei schließen
    fclose(csv_file);

    // Semaphoren zerstören
    sem_destroy(&sem_a);
    sem_destroy(&sem_b);

    printf("Ergebnisse in 'T2_semaphore_min_times.csv' gespeichert.\n");
    return 0;
}
