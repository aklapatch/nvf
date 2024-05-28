
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

	// TODO: Add tests where all values are at the end of the string..
	const char int_test[] = "i_name 32343\nf_name 3.423\ns_name \"test str\"";

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

	double bin_f = 0;
	const char *f_names[] = {"f_name"};
	rc = nvf_get_float(&root, f_names, 1, &bin_f);
	ASSERT_INT(rc, NVF_OK, 1, "Getting a float");

	char str_out[32] = {0};
	uintptr_t out_len = sizeof(str_out); 
	const char *s_names[] = {"s_name"};
	rc = nvf_get_str(&root, s_names, 1, str_out, &out_len);
	ASSERT_INT(rc, NVF_OK, 1, "Getting a str");

	ASSERT_INT(strcmp("test str", str_out), 0, 1, "Comparing string results");

	rc = nvf_deinit(&root);
	ASSERT_INT(rc, NVF_OK, 1, "Deiniting the root");

	nvf_root zero_root = {0};
	ASSERT_INT(memcmp(&zero_root, &root, sizeof(zero_root)), 0, 1, "Checking deinited root is zero");

	printf("All tests passed.\n");
	return 0;
}
