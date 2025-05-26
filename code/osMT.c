//Fadi Bassous
//1221005
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define numOfT 2

typedef struct {
    int frequency;
    char word[20];
} array;

typedef struct {
    int start;
    int end;
    array *inputArray;
    array *sharedResult;
    int *sharedIndex;
    pthread_mutex_t *mutex;
} thread_data;

int size = 0;

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

void *childJob(void *arg) {
    thread_data *data = (thread_data *)arg;
    array *localResult = makeArray(data->end - data->start); // Local frequency table
    int localSize = 0;

    // Count frequencies in the local range
    for (int i = data->start; i < data->end; i++) {
        int found = 0;
        for (int j = 0; j < localSize; j++) {
            if (strcmp(data->inputArray[i].word, localResult[j].word) == 0) {
                localResult[j].frequency++;
                found = 1;
                break;
            }
        }
        if (!found) {
            strcpy(localResult[localSize].word, data->inputArray[i].word);
            localResult[localSize].frequency = 1;
            localSize++;
        }
    }

    // Critical section to write to the shared result
    pthread_mutex_lock(data->mutex);
    for (int i = 0; i < localSize; i++) {
        data->sharedResult[*data->sharedIndex] = localResult[i];
        (*data->sharedIndex)++;
    }
    pthread_mutex_unlock(data->mutex);

    free(localResult);
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mergeResults(array *sharedResult, int chunkSize, array *finalResult, int *finalSize) {
    for (int i = 0; i < chunkSize * numOfT; i++) {
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
            strcpy(finalResult[*finalSize].word, sharedResult[i].word);
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

    wordCount();
    array *inputArray = insertArray();
    int chunkSize = (size + numOfT - 1) / numOfT; // Divide workload

    array *sharedResult = makeArray(size); // Shared memory for results
    int sharedIndex = 0; // Index for shared result
    pthread_mutex_t mutex; // Mutex for synchronization
    pthread_mutex_init(&mutex, NULL);

    pthread_t threads[numOfT];
    thread_data threadArgs[numOfT];

    for (int i = 0; i < numOfT; i++) {
        threadArgs[i].start = i * chunkSize;
        threadArgs[i].end = (i == numOfT - 1) ? size : threadArgs[i].start + chunkSize;
        threadArgs[i].inputArray = inputArray;
        threadArgs[i].sharedResult = sharedResult;
        threadArgs[i].sharedIndex = &sharedIndex;
        threadArgs[i].mutex = &mutex;
        pthread_create(&threads[i], NULL, childJob, &threadArgs[i]);
    }

    for (int i = 0; i < numOfT; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    array *finalResult = makeArray(size);
    int finalSize = 0;
    mergeResults(sharedResult, chunkSize, finalResult, &finalSize);

    printf("Top 10 words:\n");
    top10(finalResult, finalSize);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("The total time including threads is: %f minutes\n", elapsed / 60);

    free(sharedResult);
    free(inputArray);
    free(finalResult);

    return 0;
}
