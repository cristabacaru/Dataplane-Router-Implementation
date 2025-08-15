#ifndef MAP_H
#define MAP_H


// pair of string key and integer value
struct pairStruct {
    char key[11];
    void* value;
    int size;
    int capacity;
};

// array-based map
struct mapStruct {
    struct pairStruct* data;
    int size;
    int capacity;
};

// initialize a new map
struct mapStruct* map_init(void);

// retrieve a pointer to a key-value pair by key
struct pairStruct* map_get(const struct mapStruct *map, const char key[11]);

int is_subscribed(struct pairStruct* current_pair, char topic[51]);

// add a new key-value pair; returns 0 on success, 1 if key already exists
int map_add_int(struct mapStruct *map, const char key[11], int value);

int map_add_string_to_array(struct mapStruct *map, char key[10], char* str);

// remove a key-value pair by key
void map_remove(struct mapStruct *map, const char key[11]);

void remove_topic(struct mapStruct *map, char key[10], char* topic);

// free the entire map
void map_free(struct mapStruct *map);

#endif
