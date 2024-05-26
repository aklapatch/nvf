#include "nvf.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
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
   BLOB_ex Bx010203040506070809
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
	};
	return r;
}

nvf_root nvf_root_default_init(void) {
	return nvf_root_init(realloc, free);
}

nvf_err nvf_parse_buf(const char *data, uintptr_t data_len, nvf_root *out_root) {

	for (uintptr_t d_i = 0; d_i < data_len; ++d_i) {
		if (isspace(data[d_i])) {
			continue;
		}

		const char *name = &data[d_i];
		while (!isspace(data[d_i]) && d_i < data_len) {
			d_i++;
		}
		if (data_len >= d_i){
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
				if (out_root->name_num + 1 > out_root->name_cap) {
					nvf_num next_cap = out_root->name_cap*2 + 8;
					char **new_names = out_root->realloc_inst(out_root->names, next_cap * sizeof(*new_names));
					if (new_names == NULL) {
						return NVF_BAD_ALLOC;
					}
					out_root->names = new_names;

					uint8_t *new_types = out_root->realloc_inst(out_root->value_types, next_cap * sizeof(out_root->value_types));
					if (new_types == NULL) {
						return NVF_BAD_ALLOC;
					}
					out_root->value_types = new_types;
					out_root->name_cap = next_cap;
				}
				if (out_root->value_num + 1 > out_root->value_cap) {
					nvf_num next_cap = out_root->value_cap*2 + 8;
					nvf_value *new_values = out_root->realloc_inst(out_root->values, next_cap * sizeof(*out_root->values));
					if (new_values == NULL) {
						return NVF_BAD_ALLOC;
					}
					out_root->values = new_values;
					out_root->value_cap = next_cap;
				}
				out_root->values[out_root->value_num].v_int = bin_value;
				out_root->value_types[out_root->value_num] = NVF_INT;
				out_root->value_num++;
			}


			
		}

	}

	return NVF_OK;
}
