#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 1000
#define SEM_A_NAME "/sem_a"
#define SEM_B_NAME "/sem_b"

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main() {
    // Semaphoren öffnen
    sem_t *sem_a = sem_open(SEM_A_NAME, O_CREAT, 0666, 0); // Startwert 0
    sem_t *sem_b = sem_open(SEM_B_NAME, O_CREAT, 0666, 1); // Startwert 1
    if (sem_a == SEM_FAILED || sem_b == SEM_FAILED) {
        perror("Fehler beim Öffnen der Semaphoren");
        return 1;
    }

    long thread_a_times[NUM_MEASUREMENTS];
    long min_times[NUM_REPEATS];

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Warten auf Signal von Prozess B
            sem_wait(sem_a);

            // Zeitmessung starten
            long start_time = get_time_in_nanoseconds();

            // Signal an Prozess B senden
            sem_post(sem_b);

            // Warten auf Signal von Prozess B, um Zeitmessung zu stoppen
            sem_wait(sem_a);

            // Zeitmessung stoppen
            long end_time = get_time_in_nanoseconds();
            thread_a_times[i] = end_time - start_time;

            // Signal an Prozess B senden, um nächsten Zyklus zu starten
            sem_post(sem_b);
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

    // Ergebnisse in eine CSV-Datei schreiben
    FILE *csv_file = fopen("T2_sema_2P_min_times.csv", "w");
    if (!csv_file) {
        perror("Fehler beim Öffnen der Datei");
        return 1;
    }

    fprintf(csv_file, "id,mintime\n");
    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        fprintf(csv_file, "%d,%ld\n", repeat + 1, min_times[repeat]);
    }
    fclose(csv_file);

    // Semaphoren schließen
    sem_close(sem_a);
    sem_close(sem_b);

    printf("Ergebnisse in 'T2_sema_2P_min_times.csv' gespeichert.\n");
    return 0;
}
