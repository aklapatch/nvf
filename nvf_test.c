
#include "nvf.h"
#include <string.h>
#include <stdio.h>

#define ASSERT_INT(r, exp, ret_val, label)\
	do {\
		int _tmp_r = (r);\
		int _tmp_exp = (exp);\
		if (_tmp_r != _tmp_exp) {\
			printf("%s failed. Expected %d (%s), got %d (%s)!\n", label, _tmp_exp, #exp, _tmp_r, #r);\
			return ret_val;\
		}\
	} while (0)

int main(int argc, char *argv[]){
	nvf_root root = {0};

	// TODO: Add tests where all values are at the end of the string..
	const char int_test[] = 
		"i_name 32343\n"
		"f_name 3.423\n"
		"s_name \"test str\"\n"
		"b_name bx01020304\n"
		"{\n"
		"	i_name 72333\n"
		"	f_name 89.32\n"
		"	s_name \"other test str\"\n"
		"	b_name bx05060708090a0b0c0d\n"
		"}";
	uintptr_t test_len = strlen(int_test);

	nvf_err_data_i rd = nvf_parse_buf(int_test, test_len, &root);
	// I tried using memcmp(), but it compares uninitialized bytes and fails because of that.
	// Compare the variables directly to avoid this.
	ASSERT_INT(rd.data_i , 0, 1, "Testing init failure data index");
	ASSERT_INT(rd.err  , NVF_NOT_INIT, 1, "Testing init failure error");

	root = nvf_root_default_init();

	rd = nvf_parse_buf(int_test, test_len, &root);
	ASSERT_INT(rd.data_i, test_len, 1, "Testing parsing failure data index");
	ASSERT_INT(rd.err, NVF_OK, 1, "Testing parsing failure error");

	int64_t bin_int = 0;
	const char *names[] = {"i_name"};
	nvf_err rc = nvf_get_int(&root, names, 1, &bin_int);
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

	char bin_out[32] = {0};
	uintptr_t bin_out_len = sizeof(bin_out); 
	const char *b_names[] = {"b_name"};
	rc = nvf_get_blob(&root, b_names, 1, bin_out, &bin_out_len);
	ASSERT_INT(rc, NVF_OK, 1, "Getting a BLOB");
	uint8_t bin_exp[] = { 1, 2, 3, 4};
	ASSERT_INT(bin_out_len, sizeof(bin_exp), 1, "Checking BLOB size");
	ASSERT_INT(memcmp(bin_exp, bin_out, bin_out_len), 0, 1, "Checking BLOB results");

	rc = nvf_deinit(&root);
	ASSERT_INT(rc, NVF_OK, 1, "Deiniting the root");

	nvf_root zero_root = {0};
	ASSERT_INT(memcmp(&zero_root, &root, sizeof(zero_root)), 0, 1, "Checking deinited root is zero");

	printf("All tests passed.\n");
	return 0;
}
