#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

#define NUM_MEASUREMENTS 100
#define NUM_REPEATS 1000
#define SEM_A_NAME "/sem_a"
#define SEM_B_NAME "/sem_b"

int main() {
    // Semaphoren öffnen
    sem_t *sem_a = sem_open(SEM_A_NAME, O_CREAT, 0666, 0); // Startwert 0
    sem_t *sem_b = sem_open(SEM_B_NAME, O_CREAT, 0666, 1); // Startwert 1
    if (sem_a == SEM_FAILED || sem_b == SEM_FAILED) {
        perror("Fehler beim Öffnen der Semaphoren");
        return 1;
    }

    for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            // Signal an Prozess A senden
            sem_post(sem_a);

            // Warten auf Signal von Prozess A
            sem_wait(sem_b);

            // Signal an Prozess A senden
            sem_post(sem_a);

            // Warten auf Signal von Prozess A
            sem_wait(sem_b);
        }
    }

    // Semaphoren schließen
    sem_close(sem_a);
    sem_close(sem_b);

    printf("Prozess B beendet.\n");
    return 0;
}
