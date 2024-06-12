#include "nvf.h"

#include <stdio.h>
#include <string.h>

#define IF_GOTO_PRINT(cond, label, rc, goto_dest)                         \
    do {                                                                  \
        if (cond) {                                                       \
            printf("%s failed with error %s!\n", label, nvf_err_str(rc)); \
            goto goto_dest;                                               \
        }                                                                 \
    } while (0)

int main(int argc, char* argv[])
{
    const char example_buf[] = "# This is a line comment.\n"
                               "#[ This is a multi-line \n"
                               " comment. ]#\n"
                               "int 32343\n"
                               "hex_int 0x32343\n"
                               "octal_int 032343\n"
                               "float 2.0\n"
                               "hex_float 0x2.0\n"
                               "string \"string\"\n"
                               "multiline_string \" multiline \\n\"\n"
                               " 	 \" string\"\n"
                               "BLOB bx010203040506070809\n"
                               "array [32 2.0 \"string\" bx0708 [67]]\n"
                               "map {\n"
                               "	int 72333\n"
                               "	float 0.8\n"
                               "	string \"other string\"\n"
                               "	BLOB bx05060708090a0b0c0d\n"
                               "	array [128 0.4 \"a string\" bx090a0b]\n"
                               "}";
    uintptr_t example_len = strlen(example_buf);

    nvf_root root = nvf_root_default_init();
    nvf_err rc = NVF_OK;
    {
        printf("* Parsing the buffer\n");
        nvf_err_data_i rd = nvf_parse_buf(example_buf, example_len, &root);
        IF_GOTO_PRINT(rd.err != NVF_OK, "Parsing the buffer", rd.err, deinit);
    }

    {
        int64_t bin_int = 0;
        const char* names[] = { "int" };
        printf("* Getting an int\n");
        rc = nvf_get_int(&root, names, 1, &bin_int);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting an int", rc, deinit);
        printf("* The int is %lu\n", bin_int);
    }
    {
        int64_t bin_int = 0;
        const char* names[] = { "map", "int" };
        printf("* Getting a nested int\n");
        rc = nvf_get_int(&root, names, 2, &bin_int);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a nested int", rc, deinit);
        printf("* The nested int is %lu\n", bin_int);
    }

    {
        double bin_f = 0;
        const char* f_names[] = { "float" };
        printf("* Getting a float int\n");
        rc = nvf_get_float(&root, f_names, 1, &bin_f);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a float", rc, deinit);
        printf("* The float is %f\n", bin_f);
    }
    {
        double bin_f = 0;
        const char* f_names[] = { "map", "float" };
        printf("* Getting a nested float\n");
        rc = nvf_get_float(&root, f_names, 2, &bin_f);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a nested float", rc, deinit);
        printf("* The nested float is %f.\n", bin_f);
    }
    {
        char str_out[32] = { 0 };
        uintptr_t out_len = sizeof(str_out);
        const char* s_names[] = { "string" };
        printf("* Getting a string\n");
        rc = nvf_get_str(&root, s_names, 1, str_out, &out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a string", rc, deinit);
        printf("* The string is \"%s\".\n", str_out);
    }
    {
        char str_out[32] = { 0 };
        uintptr_t out_len = sizeof(str_out);
        const char* s_names[] = { "multiline_string" };
        printf("* Getting a multiline string\n");
        rc = nvf_get_str(&root, s_names, 1, str_out, &out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a string", rc, deinit);
        printf("* The string is \"%s\".\n", str_out);
    }
    {
        uint8_t bin_out[32] = { 0 };
        uintptr_t bin_out_len = sizeof(bin_out);
        const char* b_names[] = { "BLOB" };
        rc = nvf_get_blob(&root, b_names, 1, bin_out, &bin_out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a BLOB", rc, deinit);
        printf("* The BLOB is 0x");
        printf("\n");
    }

deinit:
    rc = nvf_deinit(&root);
    if (rc != NVF_OK) {
        printf("Got error %s from deiniting the root\n", nvf_err_str(rc));
    }

    printf("Done\n");
    return rc == NVF_OK ? 0 : 1;
    ;
}
