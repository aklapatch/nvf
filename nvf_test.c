
#include "nvf.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]){
	int num_tests = 0;
	int passed_tests = 0;
	nvf_root root = {0};

	const char int_test[] = "i_name 32343";

	num_tests++;
	nvf_err rc = nvf_parse_buf(int_test, strlen(int_test), &root);
	if (rc != NVF_NOT_INIT){
		printf("Init should have failed! rc=%u\n", rc);
	} else {
		passed_tests++;
	}

	root = nvf_root_default_init();

	num_tests++;
	rc = nvf_parse_buf(int_test, strlen(int_test), &root);
	if (rc != NVF_OK){
		printf("Parsing an int failed! rc=%u\n", rc);
	} else {
		passed_tests++;
	}

	num_tests++;
	rc = nvf_deinit(&root);
	// TODO: Add a test that the root is zeroed.
	if (rc != NVF_OK){
		printf("Deiniting the root failed! rc=%u\n", rc);
	} else {
		passed_tests++;
	}

	printf("%d/%d tests passed\n", num_tests, passed_tests);
	return 0;
}
