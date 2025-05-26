//Fadi Bassous
//1221005

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int size = 0;
int size2 = 300000;
typedef struct {
    int frequency;
    char word[20];
} array;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void wordCount() {
    FILE *file = fopen("file.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open file\n");
        exit(0);
    }

    char temp[100];
    while (fscanf(file, "%s", temp) != EOF) {
        size++;
    }
    fclose(file);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

array *makeArray(int size) {

    array *arr = (array *) malloc(size * sizeof(array));
    if (arr == NULL) {
        printf("Memory allocation failed\n");
        exit(0);
    }
    return arr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

array *insertArray() {
    array *arr = makeArray(size);
    FILE *file;


    file = fopen("file.txt", "r");
    if (file == NULL) {
        printf("Error: Could not reopen file\n");
        free(arr); // Clean up memory before exiting
        exit(0);
    }


    for (int c = 0; c < size; c++) {
        fscanf(file, "%s", arr[c].word);
        arr[c].frequency = 1; // Initialize frequency to 1
    }
    fclose(file);
    return arr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void deleteArray(array* arr,int index)
{
    for(int i=index;i<size2-1 ;i++)
    {
        arr[i]=arr[i+1];
    }
    size2--;
    arr = (array *)realloc(arr, size2 * sizeof(array));
    if (arr == NULL && size2 > 0) {
        printf("Memory reallocation failed\n");
        exit(1);
    }

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
array *compare(array *arr) {
    array *arr2 = makeArray(size2);

    int newSize = 0;

    for (int i = 0; i < size; i++) {
        int found = 0;

        for (int j = 0; j < newSize; j++) {
            if (strcmp(arr[i].word, arr2[j].word) == 0) {

                arr2[j].frequency++;
                found = 1;
                break;
            }
        }


        if (!found) {
            strcpy(arr2[newSize].word, arr[i].word);
            arr2[newSize].frequency = 1;
            newSize++;
        }
    }

    return arr2;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void top10(array *arr)
{
    int max = arr[0].frequency;
    int d=0;
    for (int i = 1; i < size2; i++){
        if (arr[i].frequency > max){
            max = arr[i].frequency;
            d=i;
        }

    }

    printf("%s %d\n", arr[d].word, max);

    deleteArray(arr,d);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
    clock_t start = clock();
    wordCount();
    array *arr = insertArray();

    array *arr2 = compare(arr);


    for (int i = 0; i <10 ; ++i) {
        top10(arr2);

    }

    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("The total time is: %f minutes\n", cpu_time_used/60);


    free(arr);
    free(arr2);

    return 0;
}
