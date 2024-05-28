#include "nvf.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>


/* Format example:
   # This is a comment
   #[ This is a multiline comment.
      It takes up more than one line. ]#
   int_ex 3
   float_ex 5.7
   str_ex "whee"
   multiline_str_ex "line one"
                    "line two"
   other_str_ex |line one again
                |line two again
   BLOB_ex bx010203040506070809
   array_ex [ 1 2 "elem" ]
   map_ex {
   	key1 "val1"
	key2 "val2"
   }
*/
   
nvf_root nvf_root_init(realloc_fn realloc_inst, free_fn free_inst) {
	nvf_root r = {
		.realloc_inst = realloc_inst,
		.free_inst = free_inst,
		.init_val = NVF_INIT_VAL,
	};
	return r;
}

nvf_root nvf_root_default_init(void) {
	return nvf_root_init(realloc, free);
}

nvf_err nvf_deinit_array(nvf_root *n_r, nvf_array *a) {
	IF_RET(n_r->init_val != NVF_INIT_VAL, NVF_NOT_INIT);
	free_fn f_fn = n_r->free_inst;
	IF_RET(f_fn == NULL, NVF_BAD_ARG);

	f_fn(a->value_types);
	f_fn(a->values);
	
	bzero(a, sizeof(*a));
	return NVF_OK;
}
nvf_err nvf_deinit_map(nvf_root *n_r, nvf_map *m) {
	IF_RET(n_r->init_val != NVF_INIT_VAL, NVF_NOT_INIT);
	free_fn f_fn = n_r->free_inst;
	IF_RET(f_fn == NULL, NVF_BAD_ARG);

	f_fn(m->value_types);
	f_fn(m->values);

	for (nvf_num i = 0; i < m->num; ++i) {
		f_fn(m->names[i]);
	}
	f_fn(m->names);

	bzero(m, sizeof(*m));
	return NVF_OK;
}

nvf_err nvf_deinit(nvf_root *n_r) {
	IF_RET(n_r->init_val != NVF_INIT_VAL, NVF_NOT_INIT);
	free_fn f_fn = n_r->free_inst;
	IF_RET(f_fn == NULL, NVF_BAD_ARG);
	
	for (nvf_num a_i = 0; a_i < n_r->array_num; ++a_i){
		nvf_err r = nvf_deinit_array(n_r, &n_r->arrays[a_i]);
		IF_RET(r != NVF_OK, r);
	}
	f_fn(n_r->arrays);

	nvf_err r = nvf_deinit_map(n_r, &n_r->root_map);
	IF_RET(r != NVF_OK, r);
	for (nvf_num m_i = 0; m_i < n_r->map_num; ++m_i){
		r = nvf_deinit_map(n_r, &n_r->maps[m_i]);
		IF_RET(r != NVF_OK, r);
	}
	
	// This zeros out the init_value member too, which we absolutely want.
	// That prevents this struct from being passed into another function.
	bzero(n_r, sizeof(*n_r));
	return NVF_OK;	
}

nvf_err nvf_get_map(nvf_root * root, const char **m_names, nvf_num name_depth, nvf_map *map_out) {
	IF_RET(root == NULL || m_names == NULL || map_out == NULL, NVF_BAD_ARG);

	nvf_map *cur_map = &root->root_map;
	nvf_num n_i = 0;
	for (; n_i < name_depth; ++n_i) {
		nvf_num m_i = 0;
		for (; m_i < cur_map->num; ++m_i) {
			if (strcmp(m_names[n_i], cur_map->names[m_i]) == 0) {
				if (cur_map->value_types[m_i] == NVF_MAP) {
					nvf_num new_map_i = cur_map->values[m_i].map_i;
					cur_map = &root->maps[new_map_i];
					break;
				} else {
					return NVF_NOT_FOUND;
				}
			}
		}
		// If we didn't find a name after going through the whole list, it's not here.
		// That's an error.
		IF_RET(m_i >= cur_map->num, NVF_NOT_FOUND);
	}
	if (n_i == name_depth) {
		*map_out = *cur_map;
		return NVF_OK;
	}

	return NVF_NOT_FOUND;
}

// TODO: Consolidate the get_primitive() functions.
nvf_err nvf_get_float(nvf_root *root, const char **names, nvf_num name_depth, double *out) {
	IF_RET(root == NULL || names == NULL || out == NULL, NVF_BAD_ARG);

	// We assume this array of names is null terminated.
	nvf_map parent_map;
	nvf_err e = nvf_get_map(root, names, name_depth - 1, &parent_map);
	IF_RET(e != NVF_OK, e);

	nvf_num n_i = 0;
	const char *name = names[name_depth - 1];
	for (; n_i < parent_map.num; ++n_i) {
		if (strcmp(name, parent_map.names[n_i]) == 0) {
			if (parent_map.value_types[n_i] == NVF_FLOAT) {
				*out = parent_map.values[n_i].p_val.v_float;
				return NVF_OK;
			} else {
				return NVF_BAD_VALUE_TYPE;
			}
		}

	}

	return NVF_NOT_FOUND;
}

nvf_err nvf_get_int(nvf_root *root, const char **names, nvf_num name_depth, int64_t *out) {
	IF_RET(root == NULL || names == NULL || out == NULL, NVF_BAD_ARG);

	// We assume this array of names is null terminated.
	nvf_map parent_map;
	nvf_err e = nvf_get_map(root, names, name_depth - 1, &parent_map);
	IF_RET(e != NVF_OK, e);

	nvf_num n_i = 0;
	const char *name = names[name_depth - 1];
	for (; n_i < parent_map.num; ++n_i) {
		if (strcmp(name, parent_map.names[n_i]) == 0) {
			if (parent_map.value_types[n_i] == NVF_INT) {
				*out = parent_map.values[n_i].p_val.v_int;
				return NVF_OK;
			} else {
				return NVF_BAD_VALUE_TYPE;
			}
		}

	}

	return NVF_NOT_FOUND;
}

// We reallocate instead of mallocing just in case the old pointer points to allocated memory.
// That means we don't really need to do cleanup if the old pointer points to allocated memory.
// That implies we need to zero all the pointers in the struct before they are passed into this function.
// TODO: Re-think how to make cleanup simpler if an allocation fails.
nvf_err nvf_parse_buf(const char *data, uintptr_t data_len, nvf_root *out_root) {
	IF_RET(out_root == NULL, NVF_BAD_ARG);
	IF_RET(out_root->init_val != NVF_INIT_VAL, NVF_NOT_INIT);

	nvf_map *cur_map = &out_root->root_map;
	for (uintptr_t d_i = 0; d_i < data_len; ++d_i) {
		if (isspace(data[d_i])) {
			continue;
		}

		const char *name = &data[d_i];
		while (!isspace(data[d_i]) && d_i < data_len) {
			d_i++;
		}
		if (data_len < d_i){
			return NVF_BUF_OVF;
		}
		uintptr_t name_len = (data + d_i) - name;
		// Make sure the name doesn't collide with anything we already have.
		nvf_num n_i = 0;
		for (; n_i < cur_map->num; ++n_i) {
			// The name we inferred isn't null terminated.
			// That means we can't use strcmp.
			// Do it more manually instead.
			// TODO: Add a test for this.
			size_t m_name_len = strlen(cur_map->names[n_i]);
			if (m_name_len == name_len && memcmp(cur_map->names[n_i], name, m_name_len) == 0) {
				return NVF_DUP_NAME;
			}
		}

		while (isspace(data[d_i]) && d_i < data_len) {
			d_i++;
		}

		// We've found value. Parse it depending on what it is.
		const char *value = &data[d_i];
		if (data[d_i] == '-' || isdigit(data[d_i])){
			
			// It's a number.
			bool is_float = false;
			for (; isdigit(data[d_i]) || data[d_i] == '.'; ++d_i) {
				if (data[d_i] == '.') {
					if (is_float) {
						// That's a badly formatted floating point number..
						return NVF_BAD_VALUE_FMT;
					}
					is_float = true;
				}
			}
			nvf_primitive_value npv = {0};
			uintptr_t value_len = (data + d_i) - value;
			nvf_data_type npt = NVF_NUM_TYPES;
			if (is_float) {
				char *end = (char*)value; 
				npv.v_float = strtod(value, &end);
				if (npv.v_float == HUGE_VAL || npv.v_float == HUGE_VALF || npv.v_float == HUGE_VALL) {
					return NVF_NUM_OVF;
				}
				if (value == end) {
					// The float didn't parse.
					return NVF_BAD_VALUE_FMT;
				}
				npt = NVF_FLOAT;
			} else {
				char *end = (char*)value; 
				npv.v_int = strtoll(value, &end, 10);
				if (npv.v_int == LLONG_MAX || npv.v_int == LLONG_MIN) {
					return NVF_NUM_OVF;
				}
				if (value == end) {
					// The integer didn't parse.
					return NVF_BAD_VALUE_FMT;
				}
				npt = NVF_INT;
			}

			// Grow the current map if we need to.
			if (cur_map->num + 1 > cur_map->cap) {
				nvf_num next_cap = cur_map->cap*2 + 8;
				char **new_names = out_root->realloc_inst(cur_map->names, next_cap * sizeof(*new_names));
				IF_RET(new_names == NULL, NVF_BAD_ALLOC);
				cur_map->names = new_names;

				uint8_t *new_types = out_root->realloc_inst(cur_map->value_types, next_cap * sizeof(*new_types));
				IF_RET(new_types == NULL, NVF_BAD_ALLOC);
				cur_map->value_types = new_types;

				void* new_values = out_root->realloc_inst(cur_map->values, next_cap * sizeof(*cur_map->values));
				IF_RET(new_values == NULL, NVF_BAD_ALLOC);
				cur_map->values = new_values;
				cur_map->cap = next_cap;
			}
			// Now that we have enough memory, add the integer value to the map.
			cur_map->values[cur_map->num].p_val = npv;
			cur_map->value_types[cur_map->num] = npt;

		} else if (data[d_i] == '"') {
			// Find the end of the string
			for (; data[d_i] != '"' && d_i < data_len; ++d_i) {
				// Skip escape sequences.
				if (data[d_i] == '\\') {
					d_i++;
				}
			}
			if (d_i >= data_len) {
				return NVF_BUF_OVF;
			}
		} else {
			return NVF_BAD_VALUE_TYPE;
		}
		char *name_mem = out_root->realloc_inst(cur_map->names[cur_map->num], name_len + 1);
		IF_RET(name_mem == NULL, NVF_BAD_ALLOC);
		cur_map->names[cur_map->num]= name_mem;
		// Make sure we have a null terminator like all good C strings do.
		cur_map->names[cur_map->num][name_len] = '\0';
		memcpy(cur_map->names[cur_map->num], name, name_len);
		cur_map->num++;
	}

	return NVF_OK;
}
