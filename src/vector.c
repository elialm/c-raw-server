#include "vector.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifdef VEC_EXIT_ON_OUT_OF_BOUNDS
#include <stdio.h>
#endif

#ifndef VEC_EXIT_ON_OUT_OF_BOUNDS
#define VEC_OUT_OF_BOUNDS_ACTION() return VEC_ERR_OUT_OF_BOUNDS
#else
#define VEC_OUT_OF_BOUNDS_ACTION()                                  \
do {                                                                \
    fprintf(stderr, "%u is out of bounds for a vector of size %u",  \
            index, vec->el_count);                                  \
    exit(-1);                                                       \
} while(0)
#endif

#define _VEC_GET(vec, i) ((vec)->mem + ((vec)->el_size * (i)))

// realloc the vec memory with new_capacity
vec_err_t _vec_realloc(vec_t* vec, const unsigned int new_capacity)
{
    vec->capacity = new_capacity;
    vec->mem = realloc(vec->mem, vec->el_size * new_capacity);
    return vec->mem == NULL ? VEC_ERR_ALLOC : VEC_ERR_SUCCESS;
}

vec_err_t vec_create(vec_t* vec, const unsigned int el_size, const unsigned int initial_capacity)
{
    if (vec == NULL) {
        return VEC_ERR_NULLPTR;
    } 

    if (el_size == 0 || initial_capacity == 0) {
        return VEC_ERR_NOT_CREATED;
    }

    // intialise vec
    vec->el_count = 0;
    vec->el_size = el_size;
    vec->capacity = initial_capacity;
    vec->mem = malloc(el_size * initial_capacity);

    if (vec->mem == NULL) {
        return VEC_ERR_ALLOC;
    }

    return VEC_ERR_SUCCESS;
}

void vec_destroy(vec_t* vec)
{
    if (vec != NULL) {
        // free memory and mark vec as destroyed
        free(vec->mem);
        vec->mem = NULL;
    }
}

vec_err_t vec_push_back(vec_t* vec, const void* el)
{
    if (vec == NULL || el == NULL) {
        return VEC_ERR_NULLPTR;
    }

    if (vec->mem == NULL) {
        return VEC_ERR_NO_ARRAY;
    }

    // check if capacity needs to be increased
    vec_err_t err = vec->el_count == vec->capacity 
                ? _vec_realloc(vec, vec->capacity + CAPACITY_INCREASE)
                : VEC_ERR_SUCCESS;

    if (err != VEC_ERR_SUCCESS) {
        return err;
    } else {
        // push el to the end of the array
        memcpy(_VEC_GET(vec, vec->el_count), el, vec->el_size);
        vec->el_count++;
    }

    return VEC_ERR_SUCCESS;
}

vec_err_t vec_pop_back(vec_t* vec, void* el)
{
    if (vec == NULL || el == NULL) {
        return VEC_ERR_NULLPTR;
    }

    if (vec->mem == NULL) {
        return VEC_ERR_NO_ARRAY;
    }

    if (vec->el_count == 0) {
        return VEC_ERR_ILLEGAL_OP;
    }

    // pop last element
    memcpy(el, _VEC_GET(vec, vec->el_count - 1), vec->el_size);
    vec->el_count--;

    return VEC_ERR_SUCCESS;
}

vec_err_t vec_remove(vec_t* vec, const unsigned index)
{
    if (vec == NULL) {
        return VEC_ERR_NULLPTR;
    }

    if (vec->mem == NULL) {
        return VEC_ERR_NO_ARRAY;
    }

    if (vec->el_count == 0) {
        return VEC_ERR_ILLEGAL_OP;
    }

    if (index >= vec->el_count) {
        VEC_OUT_OF_BOUNDS_ACTION();
    }

    unsigned int diff = vec->el_count - index - 1;
    if (diff != 0) {
        memmove(_VEC_GET(vec, index), _VEC_GET(vec, index + 1), diff * vec->el_size);
    }
    vec->el_count--;

    return VEC_ERR_SUCCESS;
}

vec_err_t vec_get(const vec_t* vec, void* el, const unsigned int index)
{
    if (vec == NULL || el == NULL) {
        return VEC_ERR_NULLPTR;
    }

    if (index >= vec->el_count) {
        VEC_OUT_OF_BOUNDS_ACTION();
    }

    memcpy(el, _VEC_GET(vec, index), vec->el_size);
    return VEC_ERR_SUCCESS;
}

vec_err_t vec_get_ref(vec_t* vec, void** el, const unsigned int index)
{
    if (vec == NULL || el == NULL) {
        return VEC_ERR_NULLPTR;
    }

    if (index >= vec->el_count) {
        VEC_OUT_OF_BOUNDS_ACTION();
    }

    *el = _VEC_GET(vec, index);
    return VEC_ERR_SUCCESS;
}

vec_err_t vec_set(vec_t* vec, const void* el, const unsigned int index)
{
    if (vec == NULL || el == NULL) {
        return VEC_ERR_NULLPTR;
    }

    if (index >= vec->el_count) {
        VEC_OUT_OF_BOUNDS_ACTION();
    }

    memcpy(_VEC_GET(vec, index), el, vec->el_size);
    return VEC_ERR_SUCCESS;
}

unsigned int vec_size(vec_t* vec)
{
    return vec->el_count;
}