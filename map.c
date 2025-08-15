#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct pairStruct {
    char key[11];
    void* value;
    int size;
    int capacity;
};

struct mapStruct {
    struct pairStruct* data;
    int size;
    int capacity;
};

struct mapStruct* map_init(void) {
    struct mapStruct *map = malloc(sizeof(struct mapStruct));
    if (!map) {
        fprintf(stderr, "Error with malloc\n");
        exit(1);
    }
    map->size = 0;
    map->capacity = 3;
    map->data = malloc(map->capacity * sizeof(struct pairStruct));
    if (!map->data) {
        fprintf(stderr, "Error with malloc\n");
        exit(1);
    }
    return map;
}

struct pairStruct* map_get(struct mapStruct *map, char key[10]) {
    for (int i = 0; i < map->size; i++) {
        if (strcmp(map->data[i].key, key) == 0)
            return &map->data[i];
    }
    return NULL;
}

int is_subscribed(struct pairStruct* current_pair, char topic[51]) {
    char **array = (char **)current_pair->value;
    for (int j = 0; j < current_pair->size; j++) {
        // printf("%s %ld, %s %ld\n", array[j], strlen(array[j]),  topic, strlen(topic));
        if (strcmp(array[j], topic) == 0) {
            return 1;
        }
    }
    return 0;
}

int map_add_int(struct mapStruct *map, char key[10], int value) {
    if (map_get(map, key)) return 1;
    if (map->size == map->capacity) {
        map->capacity *= 2;
        map->data = realloc(map->data, map->capacity * sizeof(struct pairStruct));
        if (!map->data) exit(1);
    }
    strcpy(map->data[map->size].key, key);
    int *ptr = malloc(sizeof(int));
    *ptr = value;
    map->data[map->size].value = ptr;
    map->data[map->size].size = 1;
    map->data[map->size].capacity = 1;
    map->size++;
    return 0;
}

int map_add_string_to_array(struct mapStruct *map, char key[10], char* str) {
    struct pairStruct* pair = map_get(map, key);
    if (pair) {
        char **array = (char **)pair->value;
        if (pair->size == pair->capacity) {
            pair->capacity *= 2;
            array = realloc(array, pair->capacity * sizeof(char*));
            if (!array) {
                fprintf(stderr, "Error with realloc\n");
                exit(1);
            }
            pair->value = array;
        }
        array[pair->size] = malloc((50 + 1) * sizeof(char));
        strcpy(array[pair->size], str);
        pair->size++;
        return 0;
    }

    // Create new pair
    if (map->size == map->capacity) {
        map->capacity *= 2;
        map->data = realloc(map->data, map->capacity * sizeof(struct pairStruct));
        if (!map->data) exit(1);
    }
    strcpy(map->data[map->size].key, key);
    char **array = malloc(8 * sizeof(char*));
    array[0] = malloc((50 + 1) * sizeof(char));
    strcpy(array[0], str);

    map->data[map->size].value = array;
    map->data[map->size].size = 1;
    map->data[map->size].capacity = 8;
    map->size++;
    return 0;
}

void remove_topic(struct mapStruct *map, char key[10], char* topic) {
    struct pairStruct* pair = map_get(map, key);
    if (!pair) return;

    char **values_array = (char **)pair->value;

    for (int i = 0; i < pair->size; i++) {
        if (strcmp(values_array[i], topic) == 0) {
            free(values_array[i]);  // free the removed string

            // shift the rest left
            for (int j = i; j < pair->size - 1; j++) {
                values_array[j] = values_array[j + 1];
            }

            pair->size--;
            return;
        }
    }
}

void map_remove(struct mapStruct *map, char key[10]) {
    for (int i = 0; i < map->size; i++) {
        if (strcmp(map->data[i].key, key) == 0) {
            for (int j = i; j < map->size - 1; j++) {
                map->data[j] = map->data[j + 1];
            }
            map->size--;
            return;
        }
    }
}

void map_free(struct mapStruct *map) {
    for (int i = 0; i < map->size; i++) {
        struct pairStruct* pair = &map->data[i];
        if (pair->capacity == 1 && pair->size == 1) {
            // int pointer
            free(pair->value);
        } else {
            // string array
            char **array = (char **)pair->value;
            for (int j = 0; j < pair->size; j++) {
                free(array[j]);
            }
            free(array);
        }
    }
    free(map->data);
    free(map);
}