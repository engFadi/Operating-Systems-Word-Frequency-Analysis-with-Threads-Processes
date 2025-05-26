#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <limits.h>

#define MAX_LENGTH 50
#define MAX_WORDS 20000000

int wordCount;
int children;

struct Array {
    char word[MAX_LENGTH];
    int frequency;
};

struct Array* insert;
int sem_id;
int* shared_size;

// Convert string to uppercase
char *strupr(char *str) {
    char *p = str;
    while (*p) {
        *p = toupper((unsigned char)*p);
        p++;
    }
    return str;
}

// Check if the word exists in the shared memory array
int exists(char *word) {
    for (int a = 0; a < *shared_size; a++) {
        if (strcmp(insert[a].word, word) == 0) {
            insert[a].frequency++;
            return 1;
        }
    }
    return 0;
}

// Load words from file within a specified range
void loadText(char *filename, int start, int end) {
    FILE *in = fopen(filename, "r");
    if (in == NULL) {
        printf("Error! Can't open the file\n");
        return;
    }

    char word[MAX_LENGTH];
    int word_count = 0;

    // Skip to the start position
    while (word_count < start && fscanf(in, "%49s", word) == 1) {
        word_count++;
    }

    // Read words until the end position
    while (word_count < end && fscanf(in, "%49s", word) == 1) {
        strupr(word);
        if (!exists(word)) {
            strncpy(insert[*shared_size].word, word, MAX_LENGTH - 1);
            insert[*shared_size].word[MAX_LENGTH - 1] = '\0';
            insert[*shared_size].frequency = 1;
            (*shared_size)++;
        }
        word_count++;
    }

    fclose(in);
}

// Sort and display the top 10 most frequent words
void sortTopTen() {
    struct Array topTenWords[10] = {{0}};

    for (int a = 0; a < *shared_size; a++) {
        for (int m = 0; m < 10; m++) {
            if (topTenWords[m].frequency == 0 || insert[a].frequency > topTenWords[m].frequency) {
                for (int r = 9; r > m; r--) {
                    topTenWords[r] = topTenWords[r - 1];
                }
                topTenWords[m] = insert[a];
                break;
            }
        }
    }

    printf("Top 10 most frequent words:\n");
    for (int a = 0; a < 10; a++) {
        if (topTenWords[a].frequency > 0) {
            printf("%s: %d\n", topTenWords[a].word, topTenWords[a].frequency);
        }
    }
}

// Count the number of words in the file
int countwords(char *filename) {
    FILE *fh = fopen(filename, "r");
    if (fh == NULL) {
        printf("The file does not exist\n");
        return 0;
    } else {
        int count = 0;
        char temp[100];
        while (fscanf(fh, "%s", temp) != EOF) {
            count++;
        }
        fclose(fh);
        return count;
    }
}

// Semaphore wait function
void sem_wait() {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    semop(sem_id, &sop, 1);
}

// Semaphore signal function
void sem_signal() {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    semop(sem_id, &sop, 1);
}

// Calculate the difference between two times
double calculate_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <num_children>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    children = atoi(argv[2]);
    wordCount = countwords(filename);

    if (wordCount == 0) {
        printf("No words in the file\n");
        return 1;
    }

    int shm_id = shmget(IPC_PRIVATE, sizeof(struct Array) * MAX_WORDS, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        return 1;
    }

    insert = (struct Array *) shmat(shm_id, NULL, 0);
    if (insert == (void *) -1) {
        perror("shmat failed");
        return 1;
    }

    int size_shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (size_shm_id < 0) {
        perror("shmget failed for shared size");
        return 1;
    }

    shared_size = (int *) shmat(size_shm_id, NULL, 0);
    if (shared_size == (void *) -1) {
        perror("shmat failed for shared size");
        return 1;
    }

    *shared_size = 0;

    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("semget failed");
        return 1;
    }

    semctl(sem_id, 0, SETVAL, 1);

    int words_per_child = wordCount / children;

    struct timespec main_start, main_end;
    clock_gettime(CLOCK_MONOTONIC, &main_start);

    for (int a = 0; a < children; a++) {
        pid_t pid = fork();
        if (pid == 0) {
            struct timespec child_start, child_end;
            clock_gettime(CLOCK_MONOTONIC, &child_start);

            int start = a * words_per_child;
            int end = (a == children - 1) ? wordCount : (a + 1) * words_per_child;
            loadText(filename, start, end);

            clock_gettime(CLOCK_MONOTONIC, &child_end);
            double time_taken = calculate_time(child_start, child_end);
            printf("Child %d completed in %.2f seconds\n", a + 1, time_taken);
            exit(0);
        }
    }

    for (int a = 0; a < children; a++) {
        wait(NULL);
    }

    sortTopTen();

    clock_gettime(CLOCK_MONOTONIC, &main_end);
    double total_time = calculate_time(main_start, main_end);
    printf("Total time taken: %.2f seconds\n", total_time);

    shmdt(insert);
    shmctl(shm_id, IPC_RMID, NULL);
    shmdt(shared_size);
    shmctl(size_shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}
