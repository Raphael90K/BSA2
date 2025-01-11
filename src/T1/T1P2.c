#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 100

// Shared Memory für Spinlock und Turn
#define SHM_NAME "/spinlock_shm"

typedef struct {
    atomic_int spinlock;  // Spinlock
    atomic_int turn;      // 0: A, 1: B
} Spinlock;

void acquire_spinlock(Spinlock *spinlock) {
    while (atomic_exchange(&spinlock->spinlock, 1)) {
        // Busy-wait (spin)
    }
}

void release_spinlock(Spinlock *spinlock) {
    atomic_store(&spinlock->spinlock, 0);
}

// Thread B: Unterstützt den Test
void *thread_b_func(void *arg) {
    Spinlock *spinlock = (Spinlock *)arg;

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Warten, bis Thread B an der Reihe ist
            while (atomic_load(&spinlock->turn) != 1);

            // Erwerbe den Lock
            acquire_spinlock(spinlock);

            // Gib den Lock frei und übergebe die Kontrolle an Thread A
            release_spinlock(spinlock);
            atomic_store(&spinlock->turn, 0); // Thread A ist an der Reihe
        }
    }
    return NULL;
}

int main() {
    // Shared Memory initialisieren
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Fehler beim Öffnen des Shared Memory");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(Spinlock)) == -1) {
        perror("Fehler beim Setzen der Shared Memory-Größe");
        return 1;
    }

    Spinlock *spinlock = mmap(NULL, sizeof(Spinlock), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (spinlock == MAP_FAILED) {
        perror("Fehler beim Mappen des Shared Memory");
        return 1;
    }

    pthread_t thread_b;

    // Starte Thread B
    pthread_create(&thread_b, NULL, thread_b_func, spinlock);

    // Warten auf Thread B
    pthread_join(thread_b, NULL);

    // Shared Memory freigeben
    munmap(spinlock, sizeof(Spinlock));
    close(shm_fd);

    return 0;
}
