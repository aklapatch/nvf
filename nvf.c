#include "nvf.h"
#include <stdlib.h>
#include <ctype.h>


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
		.base_map = {0},
	};
	return r;
}

nvf_root nvf_root_default_init(void) {
	nvf_root r = {
		.realloc_inst = realloc,
		.free_inst = free,
		.base_map = {0},
	};
	return r;
}

nvf_err nvf_parse_buf(const char *data, uintptr_t data_len, nvf_root *out_root) {

	for (uintptr_t d_i = 0; d_i < data_len; ++d_i) {
		if (isspace(data[d_i])) {
			continue;
		}


	}
	

	return NVF_OK;
}
