
#include "nvf.h"
#include <string.h>
#include <stdio.h>

#define ASSERT_INT(r, exp, ret_val, label)\
	do {\
		int _tmp_r = (r);\
		int _tmp_exp = (exp);\
		if (_tmp_r != _tmp_exp) {\
			printf("%s failed. Expected %d (%s), got %d (%s)!\n", label, exp, #exp, r, #r);\
			return ret_val;\
		}\
	} while (0)

int main(int argc, char *argv[]){
	nvf_root root = {0};

	const char int_test[] = "i_name 32343\nf_name 3.423";

	nvf_err rc = nvf_parse_buf(int_test, strlen(int_test), &root);
	ASSERT_INT(rc, NVF_NOT_INIT, 1, "Testing init failure");

	root = nvf_root_default_init();

	rc = nvf_parse_buf(int_test, strlen(int_test), &root);
	ASSERT_INT(rc, NVF_OK, 1, "Parsing data");

	int64_t bin_int = 0;
	const char *names[] = {"i_name"};
	rc = nvf_get_int(&root, names, 1, &bin_int);
	ASSERT_INT(rc, NVF_OK, 1, "Getting an int");

	int exp_val = 32343;
	ASSERT_INT((int)bin_int, exp_val, 1, "Comparing int values");

	rc = nvf_deinit(&root);
	ASSERT_INT(rc, NVF_OK, 1, "Deiniting the root");
	// TODO: Add a test that the root is zeroed.

	printf("All tests passed.\n");
	return 0;
}
