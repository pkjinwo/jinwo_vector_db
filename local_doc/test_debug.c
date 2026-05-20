/*
 * test_debug.c - Debug test for quantization
 */

#include "include/jw_vector.h"
#include "include/jw_arena.h"
#include "include/jw_types.h"
#include <stdio.h>

int main(void)
{
    printf("Starting debug test...\n");
    fflush(stdout);

    printf("Creating arena...\n");
    fflush(stdout);

    jw_arena_t *arena;
    jw_status_t status = jw_arena_create(4096 * 1024, &arena);
    if (status != JW_SUCCESS) {
        printf("Pool creation failed with status: %d\n", status);
        return 1;
    }

    printf("Pool created successfully\n");
    fflush(stdout);

    jw_dim_t dim = 8;
    printf("Allocating vector of size %u...\n", dim);
    fflush(stdout);

    jw_vec_t vec = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    if (vec == NULL) {
        printf("Vector allocation failed\n");
        jw_arena_destroy(arena);
        return 1;
    }

    printf("Vector allocated successfully\n");
    fflush(stdout);

    printf("Generating random vector...\n");
    fflush(stdout);

    for (jw_dim_t i = 0; i < dim; i++) {
        vec[i] = -10.0f + jw_rand_float() * 20.0f;
    }

    printf("Random vector generated: ");
    for (jw_dim_t i = 0; i < dim; i++) {
        printf("%.2f ", vec[i]);
    }
    printf("\n");
    fflush(stdout);

    printf("Creating SQ quantizer...\n");
    fflush(stdout);

    jw_sq_t *sq;
    status = jw_sq_create(arena, dim, &sq);
    if (status != JW_SUCCESS) {
        printf("SQ create failed with status: %d\n", status);
        jw_arena_destroy(arena);
        return 1;
    }

    printf("SQ quantizer created successfully\n");
    fflush(stdout);

    printf("Training SQ quantizer...\n");
    fflush(stdout);

    status = jw_sq_train(sq, vec, 1);
    if (status != JW_SUCCESS) {
        printf("SQ train failed with status: %d\n", status);
        jw_arena_destroy(arena);
        return 1;
    }

    printf("SQ quantizer trained successfully\n");
    fflush(stdout);

    printf("Destroying arena...\n");
    fflush(stdout);

    jw_arena_destroy(arena);

    printf("Pool destroyed successfully\n");
    fflush(stdout);

    printf("Debug test completed successfully\n");
    return 0;
}