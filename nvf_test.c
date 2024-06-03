
#include "nvf.h"
#include <string.h>
#include <stdio.h>

#define ASSERT_INT(r, exp, ret_val, label)\
	do {\
		int _tmp_r = (r);\
		int _tmp_exp = (exp);\
		if (_tmp_r != _tmp_exp) {\
			printf("%s:%u. %s failed. Expected %d (%s), got %d (%s)!\n", __FILE__, __LINE__, label, _tmp_exp, #exp, _tmp_r, #r);\
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
		"a_name [ 32 1.32 \"str\" bx0708 ]\n"
		"m_name {\n"
		"	i_name 72333\n"
		"	f_name 89.32\n"
		"	s_name \"other test str\"\n"
		"	b_name bx05060708090a0b0c0d\n"
		"	a_name [ 32 1.32 \"str\" bx0708 ]\n"
		"}";
	uintptr_t test_len = strlen(int_test);

	nvf_err_data_i rd = nvf_parse_buf(int_test, test_len, &root);
	// I tried using memcmp(), but it compares uninitialized bytes and fails because of that.
	// Compare the variables directly to avoid this.
	ASSERT_INT(rd.data_i, 0, 1, "Testing init failure data index");
	ASSERT_INT(rd.err  , NVF_NOT_INIT, 1, "Testing init failure error");

	root = nvf_root_default_init();

	rd = nvf_parse_buf(int_test, test_len, &root);
	ASSERT_INT(rd.err, NVF_OK, 1, "Testing parsing");
	ASSERT_INT(rd.data_i >= test_len, 1, 1, "Testing parsing data index");

	int64_t bin_int = 0;
	const char *names[] = {"i_name"};
	nvf_err rc = nvf_get_int(&root, names, 1, &bin_int);
	ASSERT_INT(rc, NVF_OK, 1, "Getting an int");

	ASSERT_INT((int)bin_int, 32343, 1, "Comparing int values");

	const char *m_i_names[] = {"m_name", "i_name"};
	rc = nvf_get_int(&root, m_i_names, 2, &bin_int);
	ASSERT_INT(rc, NVF_OK, 1, "Getting a nested int");

	ASSERT_INT((int)bin_int, 72333, 1, "Comparing int values");


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

	nvf_array arr = {0};
	const char *a_names = "a_name";
	rc = nvf_get_array(&root, &a_names, 1, &arr);
	ASSERT_INT(rc, NVF_OK, 1, "Getting an array");

	nvf_array_iter a_i = nvf_iter_init(&arr);

	nvf_tag_value tv = nvf_get_next(&a_i);
	ASSERT_INT(tv.type, NVF_INT, 1, "Getting int type from array");
	ASSERT_INT(tv.val.v_int, 32, 1, "Getting int value from array");

	nvf_tag_value tv_f = nvf_get_next(&a_i);
	ASSERT_INT(tv_f.type, NVF_FLOAT, 1, "Getting float type from array");
	ASSERT_INT(tv_f.val.v_float == 1.32, 1, 1, "Getting float value from array");

	nvf_tag_value tv_s = nvf_get_next(&a_i);
	ASSERT_INT(tv_s.type, NVF_STRING, 1, "Getting string type from array");
	ASSERT_INT(strcmp(tv_s.val.v_string, "str"), 0, 1, "Getting string value from array");

	nvf_tag_value tv_b = nvf_get_next(&a_i);
	ASSERT_INT(tv_b.type, NVF_BLOB, 1, "Getting blob type from array");
	ASSERT_INT(tv_b.val.v_blob->len, 2, 1, "Getting blob length from array");
	uint8_t b_exp_val[] = {7, 8};
	ASSERT_INT(memcmp(tv_b.val.v_blob->data, b_exp_val, sizeof(b_exp_val)), 0, 1, "Getting blob value from array");

	nvf_tag_value tv_n = nvf_get_next(&a_i);
	ASSERT_INT(tv_n.type, NVF_NONE, 1, "Getting none value from an iterator");

	rc = nvf_deinit(&root);
	ASSERT_INT(rc, NVF_OK, 1, "Deiniting the root");

	nvf_root zero_root = {0};
	ASSERT_INT(memcmp(&zero_root, &root, sizeof(zero_root)), 0, 1, "Checking deinited root is zero");

	printf("All tests passed.\n");
	return 0;
}
