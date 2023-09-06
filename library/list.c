#include "list.h"
#include <assert.h>
#include <stdlib.h>

const size_t SCALING_FACTOR = 2;

/**
 * Properties of an array list.
 * data: the underlying array data structure that stores our data
 * size: the number of elements currently in our array
 * capacity: the maximum number of elements that can be stored in our array
 */
typedef struct list {
    void **data;
    size_t size;
    size_t capacity;
    free_func_t freer;
} list_t;

list_t *list_init(size_t initial_capacity, free_func_t freer) {
    list_t *list = malloc(sizeof(list_t));
    assert(list);
    list->size = 0;
    assert(list->data);
    if (initial_capacity <= 0) {
        list->capacity = 1;
    } else {
        list->capacity = initial_capacity;
    }
    list->data = malloc(sizeof(void *) * list->capacity);
    list->freer = freer;
    return list;
}

void list_free_data(list_t *list) {
    if (list->freer) {
        for (size_t i = 0; i < list->size; i++) {
            list->freer(list->data[i]);
        }
    }
}

void list_free(list_t *list) {
    list_free_data(list);
    free(list->data);
    free(list);
}

void list_clear(list_t *list) {
    list_free_data(list);
    list->size = 0;
}

size_t list_size(list_t *list) {
    return list->size;
}

void *list_get(list_t *list, size_t index) {
    assert(index >= 0 && index < list->size);
    return list->data[index];
}

/**
 * Helper function.
 * Increases the capacity of a list by a constant scaling factor.
 * @param list the list we want to resize
 */
void list_increase_capacity(list_t *list) {
    list->capacity *= SCALING_FACTOR;
    list->data = realloc(list->data, sizeof(void *) * list->capacity);
}

void list_add(list_t *list, void *value) {
    assert(value != NULL);
    if (list->size >= list->capacity) {
        list_increase_capacity(list);
    }
    list->data[list->size] = value;
    list->size++;
}

list_t *list_append(list_t *list1, list_t *list2) {
    // the combined list should not be responsible for freeing the contents
    // of list 1 or list 2
    list_t *combined_list =
        list_init(list_size(list1) + list_size(list2), NULL);
    for (size_t i = 0; i < list_size(list1); i++) {
        list_add(combined_list, list_get(list1, i));
    }
    for (size_t i = 0; i < list_size(list2); i++) {
        list_add(combined_list, list_get(list2, i));
    }
    return combined_list;
}

size_t list_index_of(list_t *list, void *value) {
    for (size_t i = 0; i < list->size; i++) {
        if (list->data[i] == value) {
            return i;
        }
    }
    return -1;
}

void *list_remove(list_t *list, size_t index) {
    assert(index >= 0 && index < list->size);
    void *res = list->data[index];
    list->size--;
    for (size_t i = index; i < list->size; i++) {
        list->data[i] = list->data[i + 1];
    }
    return res;
}

void *list_remove_back(list_t *list) {
    return list_remove(list, list->size - 1);
}

void *list_remove_element(list_t *list, void *value) {
    size_t index = list_index_of(list, value);
    if (index == -1) {
        return NULL;
    }
    return list_remove(list, index);
}

list_t *list_copy(list_t *list, copy_func_t copier) {
    list_t *result = list_init(list->capacity, list->freer);
    for (size_t i = 0; i < list->size; i++) {
        list_add(result, copier(list_get(list, i)));
    }
    return result;
}
