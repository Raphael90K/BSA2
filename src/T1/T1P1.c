#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 1000

// Shared Memory für Spinlock und Turn
#define SHM_NAME "/spinlock_shm"

typedef struct {
    atomic_int spinlock;  // Spinlock
    atomic_int turn;      // 0: A, 1: B
} Spinlock;

long thread_a_times[NUM_MEASUREMENTS];
long min_times[NUM_REPEATS];

// Hilfsfunktion für präzise Zeitmessung
long get_time_in_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // Hohe Präzision, monotone Uhr
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Spinlock-Operationen
void acquire_spinlock(Spinlock *spinlock) {
    while (atomic_exchange(&spinlock->spinlock, 1)) {
        // Busy-wait (spin)
    }
}

void release_spinlock(Spinlock *spinlock) {
    atomic_store(&spinlock->spinlock, 0);
}

// Thread A: Führt die Zeitmessung durch
void *thread_a_func(void *arg) {
    Spinlock *spinlock = (Spinlock *)arg;

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        long min_time = LONG_MAX;

        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Warten, bis Thread A an der Reihe ist
            while (atomic_load(&spinlock->turn) != 0);

            // Erwerbe den Lock
            acquire_spinlock(spinlock);
            long start_time = get_time_in_nanoseconds();

            // Gib den Lock frei und übergebe die Kontrolle an Thread B
            release_spinlock(spinlock);
            atomic_store(&spinlock->turn, 1); // Thread B ist an der Reihe

            // Messen, wie lange es dauert, bis Thread A den Lock wiedererhält
            while (atomic_load(&spinlock->turn) != 0); // Warten bis A wieder an der Reihe ist

            acquire_spinlock(spinlock);  // Jetzt wartet Thread A und erwirbt den Lock erneut
            long end_time = get_time_in_nanoseconds();

            // Speichere die gemessene Zeit
            thread_a_times[i] = end_time - start_time;

            // Gib den Lock frei
            release_spinlock(spinlock);
        }

        // Mindestzeit berechnen
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            if (thread_a_times[i] < min_time) {
                min_time = thread_a_times[i];
            }
        }
        min_times[repeat] = min_time; // Mindestzeit der aktuellen Wiederholung speichern
    }
    return NULL;
}

int main() {
    // Shared Memory öffnen
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Fehler beim Öffnen des Shared Memory");
        return 1;
    }

    Spinlock *spinlock = mmap(NULL, sizeof(Spinlock), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (spinlock == MAP_FAILED) {
        perror("Fehler beim Mappen des Shared Memory");
        return 1;
    }

    // Initialisiere Spinlock
    atomic_store(&spinlock->spinlock, 0);
    atomic_store(&spinlock->turn, 0); // Thread A beginnt

    pthread_t thread_a;

    // Starte Thread A
    pthread_create(&thread_a, NULL, thread_a_func, spinlock);

    // Warten auf Thread A
    pthread_join(thread_a, NULL);

    // Ergebnisse in eine CSV-Datei speichern
    FILE *csv_file = fopen("T1_spinlock_sm_min_times.csv", "w");
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

    // Shared Memory freigeben
    munmap(spinlock, sizeof(Spinlock));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    printf("Ergebnisse in 'T1_spinlock_sm_min_times.csv' gespeichert.\n");

    return 0;
}
