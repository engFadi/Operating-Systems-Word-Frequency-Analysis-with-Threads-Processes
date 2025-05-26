//1221005
//Fadi Bassous
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>

#define numOfP 2

typedef struct {
    int frequency;
    char word[20];
} array;

int size = 0;
sem_t *sem; // Semaphore pointer

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void wordCount() {
    FILE *file = fopen("file.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open file\n");
        exit(1);
    }
    char temp[100];
    while (fscanf(file, "%s", temp) != EOF) {
        size++;
    }
    fclose(file);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

array *makeArray(int size) {
    array *arr = (array *)malloc(size * sizeof(array));
    if (arr == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    return arr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

array *insertArray() {
    array *arr = makeArray(size);
    FILE *file = fopen("file.txt", "r");
    if (file == NULL) {
        printf("Error: Could not reopen file\n");
        free(arr);
        exit(1);
    }
    for (int c = 0; c < size; c++) {
        fscanf(file, "%s", arr[c].word);
        arr[c].frequency = 1; // Initialize frequency to 1
    }
    fclose(file);
    return arr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void childJob(int start, int end, array *inputArray, array *sharedResult, int sharedIndex) {
    array *localResult = makeArray(end - start); // Local frequency table
    int localSize = 0;

    for (int i = start; i < end; i++) {
        int found = 0;
        for (int j = 0; j < localSize; j++) {
            if (strcmp(inputArray[i].word, localResult[j].word) == 0) {
                localResult[j].frequency++;
                found = 1;
                break;
            }
        }
        if (!found) {
            strcpy(localResult[localSize].word, inputArray[i].word);
            localResult[localSize].frequency = 1;
            localSize++;
        }
    }

    sem_wait(sem); // Acquire the semaphore before writing to sharedResult
    for (int i = 0; i < localSize; i++) {
        sharedResult[sharedIndex + i] = localResult[i];
    }
    sem_post(sem); // Release the semaphore after writing

    free(localResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mergeResults(array *sharedResult, int chunkSize, array *finalResult, int *finalSize) {
    for (int i = 0; i < chunkSize * numOfP; i++) {
        if (sharedResult[i].frequency == 0) continue;

        int found = 0;
        for (int j = 0; j < *finalSize; j++) {
            if (strcmp(sharedResult[i].word, finalResult[j].word) == 0) {
                finalResult[j].frequency += sharedResult[i].frequency;
                found = 1;
                break;
            }
        }

        if (!found) {
            strcpy(finalResult[*final   Size].word, sharedResult[i].word);
            finalResult[*finalSize].frequency = sharedResult[i].frequency;
            (*finalSize)++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void top10(array *arr, int size) {
    for (int i = 0; i < 10; i++) {
        int maxIndex = 0;
        for (int j = 1; j < size; j++) {
            if (arr[j].frequency > arr[maxIndex].frequency) {
                maxIndex = j;
            }
        }
        printf("%s %d\n", arr[maxIndex].word, arr[maxIndex].frequency);
        arr[maxIndex].frequency = 0; // Remove this entry from consideration
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 1); // Initialize the semaphore for shared use

    wordCount();
    array *inputArray = insertArray();
    int chunkSize = (size + numOfP - 1) / numOfP; // Divide workload

    array *sharedResult = mmap(NULL, size * sizeof(array), PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int pid[numOfP];
    for (int i = 0; i < numOfP; i++) {
        pid[i] = fork();
        if (pid[i] < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid[i] == 0) {
            int startIdx = i * chunkSize;
            int endIdx = (i == numOfP - 1) ? size : startIdx + chunkSize;
            childJob(startIdx, endIdx, inputArray, sharedResult, i * chunkSize);
            exit(0);
        }
    }

    for (int i = 0; i < numOfP; i++) {
        waitpid(pid[i], NULL, 0);
    }

    array *finalResult = makeArray(size);
    int finalSize = 0;
    mergeResults(sharedResult, chunkSize, finalResult, &finalSize);

    printf("Top 10 words:\n");
    top10(finalResult, finalSize);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("The total time including child processes is: %f minutes\n", elapsed / 60);

    munmap(sharedResult, size * sizeof(array));
    sem_destroy(sem); // Destroy the semaphore
    munmap(sem, sizeof(sem_t));
    free(inputArray);
    free(finalResult);

    return 0;
}
