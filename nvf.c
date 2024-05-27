#include "nvf.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>


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

nvf_err nvf_get_int(const char **names, int64_t *out) {

	return NVF_OK;
}

// We reallocate instead of mallocing just in case the old pointer points to allocated memory.
// That means we don't really need to do cleanup if the old pointer points to allocated memory.
// That implies we need to zero all the pointers in the struct before they are passed into this function.
// TODO: Add a bool to the nvf_root to indicate it's initialized.
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
			uintptr_t value_len = (data + d_i) - value;
			if (is_float) {

			} else {
				char *end; 
				int64_t bin_value = strtoll(value, &end, 10);
				if (bin_value == LLONG_MAX || bin_value == LLONG_MIN) {
					return NVF_NUM_OVF;
				}
				if (value == end) {
					// The integer didn't parse.
					return NVF_BAD_VALUE_FMT;
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
				cur_map->values[cur_map->num].p_val.v_int = bin_value;
				cur_map->value_types[cur_map->num] = NVF_INT;
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
