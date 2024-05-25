#include "nvf.h"


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
   
nvf_alloc nvf_alloc_init(realloc_fn realloc_inst, free_fn free_inst) {
	nvf_alloc r = {
		.realloc_inst = realloc_inst,
		.free_inst = free_inst,
	};
	return r;
}
