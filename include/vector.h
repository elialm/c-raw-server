#ifndef __VECTOR_H__
#define __VECTOR_H__

#define DEFAULT_CAPACITY 16
#define CAPACITY_INCREASE 16

typedef enum vec_err {
    VEC_ERR_SUCCESS = 0,
    VEC_ERR_NULLPTR,
    VEC_ERR_NO_ARRAY,
    VEC_ERR_NOT_CREATED,
    VEC_ERR_ALLOC,
    VEC_ERR_OUT_OF_BOUNDS,
    VEC_ERR_ILLEGAL_OP
} vec_err_t;

typedef struct vec {
    unsigned int el_count;
    unsigned int el_size;
    unsigned int capacity;
    void* mem;
} vec_t;


#ifdef __cplusplus
extern "C" {
#endif

vec_err_t vec_create(vec_t* vec, const unsigned int el_size, const unsigned int initial_capacity);
void vec_destroy(vec_t* vec);
vec_err_t vec_push_back(vec_t* vec, const void* el);
vec_err_t vec_pop_back(vec_t* vec, void* el);
vec_err_t vec_remove(vec_t* vec, const unsigned index);

vec_err_t vec_get(const vec_t* vec, void* el, const unsigned int index);
vec_err_t vec_get_ref(vec_t* vec, void** el, const unsigned int index);
vec_err_t vec_set(vec_t* vec, const void* el, const unsigned int index);

unsigned int vec_size(vec_t* vec);

#ifdef __cplusplus
}
#endif

#endif //__VECTOR_H__