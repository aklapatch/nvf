#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum {
	NVF_OK = 0,
	NVF_ERROR,
	NVF_BAD_ALLOC,
	NVF_BUF_OVF,
	NVF_BAD_VALUE_FMT,
	NVF_BAD_ARG,
	NVF_BAD_VALUE_TYPE,
	NVF_NOT_INIT,
	NVF_NUM_OVF,
	NVF_NUM_ERRS,
} nvf_err;

typedef enum {
	NVF_FLOAT = 0,
	NVF_INT,
	NVF_BLOB,
	NVF_STRING,
	NVF_MAP,
	NVF_ARRAY,
	NVF_NUM_TYPES,
} nvf_data_type;

// This is an index into the array of names..
typedef uint32_t nvf_name_i;
// This is used to keep track of how many elements are in something
typedef uint32_t nvf_num;

typedef struct nvf_blob {
	nvf_num len;
	uint8_t data[];
} nvf_blob;

typedef union nvf_value {
	int64_t v_int;
	double v_float;
	char *v_string;
	nvf_blob *v_blob;
} nvf_value;


typedef struct nvf_array {
	nvf_num   num, 
		  cap;
	nvf_value *values;
} nvf_array;

// TODO: Try to find a more efficient memory layout. I'm fairly sure we could make this better.
typedef struct nvf_map {
	nvf_num num,
		cap;
	nvf_name_i *value_name_is;
} nvf_map;

typedef void* (*realloc_fn)(void *, size_t);
typedef void (*free_fn)(void *);

#define NVF_INIT_VAL (0x72)

// These are in one struct because the allocator and the memory used in the base map are linked.
// You neeed to keep the allocators and the map together because you'll need to manipulate the memory
// allocated with the same allocator.
typedef struct nvf_root {
	realloc_fn realloc_inst;
	free_fn free_inst;
	nvf_num name_num,
	        name_cap; 
	uint8_t *value_types;
	char **names;
	nvf_num *name_value_i;

	nvf_num value_num,
	        value_cap; 
	nvf_value *values;

	nvf_num array_num,
	        array_cap; 
	nvf_array *arrays;

	nvf_num map_num,
	        map_cap; 
	nvf_map *maps;
	uint8_t init_val;
} nvf_root;

nvf_root nvf_root_init(realloc_fn realloc_inst, free_fn free_inst);

nvf_root nvf_root_default_init(void);

nvf_err nvf_parse_buf(const char *data, uintptr_t data_len, nvf_root *out_root);

nvf_err nvf_deinit(nvf_root *n_r);
