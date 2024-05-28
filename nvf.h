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
	NVF_NOT_FOUND,
	NVF_DUP_NAME,
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

typedef union nvf_primitive_value {
	int64_t v_int;
	double v_float;
	char *v_string;
	nvf_blob *v_blob;
} nvf_primitive_value;


// Arrays can't contain maps. I think that's fine for now.
typedef struct nvf_array {
	nvf_num   num, 
		  cap;
	uint8_t *value_types;
	union {
		nvf_num array_i;
		nvf_primitive_value p_val;
	} *values;
} nvf_array;

// TODO: Try to find a more efficient memory layout. I'm fairly sure we could make this better.
typedef struct nvf_map {
	nvf_num num,
		cap;
	char **names;
	uint8_t *value_types;
	union {
		nvf_num map_i;
		nvf_num array_i;
		nvf_primitive_value p_val;
	} *values;
} nvf_map;

typedef void* (*realloc_fn)(void *, size_t);
typedef void (*free_fn)(void *);

#define NVF_INIT_VAL (0x72)

#define IF_RET(expr, rv) \
	do {			  \
		if ((expr)) {     \
			return rv;\
		}		  \
	} while (0)

// These are in one struct because the allocator and the memory used in the base map are linked.
// You neeed to keep the allocators and the map together because you'll need to manipulate the memory
// allocated with the same allocator.
// NOTE: I tried to make this structure flat, using arrays to hold differently typed values, but I ran into a problem.
// I couldn't know how deep an item was in a structure. To search for a named value, I could look through the array of all the names,
// but that array didn't have the nesting level of the name. I could add a nesting level, but that could
// make search times bad for an unsuccessful linear search.
// I may go back to having a root map, and storing items in tagged unions.
// I'll probably have to use void* to nest maps in maps. Making maps self referential is difficult otherwise.
// I had an epiphany. Only the names need to be nested. The values don't.
// I can store all the blobs and types and values in one array, with indicies referencing it if necessary.
// I'm not sure how efficient that will be though. I need to think about this further.
// There's only an extra storage cost if I add an extra pointer to something.
// BLOBS don't cost extra because I need to store data behind a pointer anyway. A reference to a map might cost extra because that could be an index into an array of maps. 
// Same for arrays. There will be pointers to those, but they could be indexes into an array of arrays.
typedef struct nvf_root {
	realloc_fn realloc_inst;
	free_fn free_inst;
	nvf_map root_map;

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

nvf_err nvf_get_int(nvf_root *root, const char **names, nvf_num name_depth, int64_t *out);

nvf_err nvf_get_map(nvf_root * root, const char **m_names, nvf_num name_depth, nvf_map *map_out);

nvf_err nvf_deinit(nvf_root *n_r);

nvf_err nvf_get_float(nvf_root *root, const char **names, nvf_num name_depth, double *out);

nvf_err nvf_get_str(nvf_root *root, const char **names, nvf_num name_depth, char *str_out, uintptr_t *str_out_len);
