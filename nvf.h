#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum {
	NVF_OK,
	NVF_ERROR,
	NVF_ALLOC_FAIL,
	NVF_NUM_ERRS,
} nvf_err;

typedef enum {
	NVF_FLOAT,
	NVF_INT,
	NVF_BLOB,
	NVF_STRING,
	NVF_MAP,
	NVF_ARRAY,
} nvf_data_type;

typedef union nvf_value {
	int64_t v_int;
	double v_float;
	char *v_string;
} nvf_value;

typedef struct nvf_array {
	uintptr_t num;
	// I can't think of a way to nest maps in an array.
	// Don't allow that for now.
	union { 
		nvf_value val;
		struct nvf_array *array;
	} data[];
} nvf_array;

typedef struct nvf_map {
	uintptr_t num;
	char **keys;
	union {
		nvf_value val;
		struct nvf_map *map;
		struct nvf_array *array;
	} *values;
} nvf_map;

typedef void* (*realloc_fn)(void *, size_t);
typedef void (*free_fn)(void *);

typedef struct nvf_alloc {
	realloc_fn realloc_inst;
	free_fn free_inst;
} nvf_alloc;
