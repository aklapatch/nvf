#include "nvf.h"

#include <stdio.h>
#include <string.h>

/**
  \file nvf_example.c
  Holds examples for the use of NVF functions.
*/

#define IF_GOTO_PRINT(cond, label, rc, goto_dest)                              \
    do {                                                                       \
        if (cond) {                                                            \
            printf("%s failed with error %s!\n", label, nvf_err_str(rc));      \
            goto goto_dest;                                                    \
        }                                                                      \
    } while (0)

void hexdump(uint8_t *data, uintptr_t len) {
    for (uintptr_t i = 0; i < len; ++i) {
        printf("%c", nvf_bin_to_char(data[i] >> 4));
        printf("%c", nvf_bin_to_char(data[i] & 0xf));
    }
}

int main(int argc, char *argv[]) {
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
        const char *names[] = {"int"};
        printf("* Getting an int\n");
        rc = nvf_get_int(&root, names, 1, &bin_int);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting an int", rc, deinit);
        printf("* The int is %lu\n", bin_int);
    }
    {
        int64_t bin_int = 0;
        const char *names[] = {"map", "int"};
        printf("* Getting a nested int\n");
        rc = nvf_get_int(&root, names, 2, &bin_int);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a nested int", rc, deinit);
        printf("* The nested int is %lu\n", bin_int);
    }

    {
        double bin_f = 0;
        const char *f_names[] = {"float"};
        printf("* Getting a float int\n");
        rc = nvf_get_float(&root, f_names, 1, &bin_f);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a float", rc, deinit);
        printf("* The float is %f\n", bin_f);
    }
    {
        double bin_f = 0;
        const char *f_names[] = {"map", "float"};
        printf("* Getting a nested float\n");
        rc = nvf_get_float(&root, f_names, 2, &bin_f);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a nested float", rc, deinit);
        printf("* The nested float is %f.\n", bin_f);
    }
    {
        char str_out[32] = {0};
        uintptr_t out_len = sizeof(str_out);
        const char *s_names[] = {"string"};
        printf("* Getting a string\n");
        rc = nvf_get_str(&root, s_names, 1, str_out, &out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a string", rc, deinit);
        printf("* The string is \"%s\".\n", str_out);
    }
    {
        char *str_out = NULL;
        uintptr_t out_len = 0;
        const char *s_names[] = {"string"};
        printf("* Getting an allocated string\n");
        rc = nvf_get_str_alloc(&root, s_names, 1, &str_out, &out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting an allocated string", rc, deinit);
        printf("* The allocatedstring is \"%s\".\n", str_out);
        root.free_inst(str_out);
    }
    {
        char str_out[32] = {0};
        uintptr_t out_len = sizeof(str_out);
        const char *s_names[] = {"multiline_string"};
        printf("* Getting a multiline string\n");
        rc = nvf_get_str(&root, s_names, 1, str_out, &out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a string", rc, deinit);
        printf("* The string is \"%s\".\n", str_out);
    }
    {
        uint8_t bin_out[32] = {0};
        uintptr_t bin_out_len = sizeof(bin_out);
        const char *b_names[] = {"BLOB"};
        printf("* Getting a BLOB\n");
        rc = nvf_get_blob(&root, b_names, 1, bin_out, &bin_out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a BLOB", rc, deinit);
        printf("* The BLOB is 0x");
        hexdump(bin_out, bin_out_len);
        printf("\n");
    }
    {
        uint8_t bin_out[32] = {0};
        uintptr_t bin_out_len = sizeof(bin_out);
        const char *b_names[] = {"map", "BLOB"};
        printf("* Getting a nested BLOB\n");
        rc = nvf_get_blob(&root, b_names, 2, bin_out, &bin_out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a nested BLOB", rc, deinit);
        printf("* The nested BLOB is 0x");
        hexdump(bin_out, bin_out_len);
        printf("\n");
    }
    {
        uint8_t *bin_out = NULL;
        uintptr_t bin_out_len = 0;
        const char *b_names[] = {"map", "BLOB"};
        printf("* Getting a nested, allocated BLOB\n");
        rc = nvf_get_blob_alloc(&root, b_names, 2, &bin_out, &bin_out_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting a nested, allocated BLOB", rc,
                      deinit);
        printf("* The nested, allocated BLOB is 0x");
        hexdump(bin_out, bin_out_len);
        printf("\n");
        root.free_inst(bin_out);
    }
    {
        nvf_array arr = {0};
        const char *a_names = "array";
        printf("* Getting an nested array\n");
        rc = nvf_get_array(&root, &a_names, 1, &arr);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting an array", rc, deinit);

        printf("\tGetting an array's value\n");
        nvf_tag_value tv = nvf_array_get_item(&arr, 0);
        printf("\tThe value has type %s and is %ld\n", nvf_type_str(tv.type),
               tv.val.v_int);

        printf("\tGetting an array's second value\n");
        nvf_tag_value tv_f = nvf_array_get_item(&arr, 1);
        printf("\tThe value has type %u and is %ld\n", tv_f.type,
               tv_f.val.v_int);

        printf("\tGetting an array's third value\n");
        nvf_tag_value tv_s = nvf_array_get_item(&arr, 2);
        printf("\tThe value has type %u and is %s\n", tv_s.type,
               tv_s.val.v_string);

        printf("\tGetting an array's fourth value\n");
        nvf_tag_value tv_b = nvf_array_get_item(&arr, 3);
        printf("\tThe value has type %u and is 0x", tv_b.type);
        hexdump(tv_b.val.v_blob->data, tv_b.val.v_blob->len);
        printf("\n");

        printf("\tGetting an array's fifth value\n");
        nvf_tag_value tv_a = nvf_array_get_item(&arr, 4);
        printf("\tThe value's type is %u\n", tv_a.type);

        {
            nvf_array t_arr = {0};
            rc = nvf_get_array_from_i(&root, tv_a.val.array_i, &t_arr);
            IF_GOTO_PRINT(rc != NVF_OK, "Getting a nested array", rc, deinit);

            printf("\t\tGetting a nested nested int\n");
            nvf_tag_value n_tv_i = nvf_array_get_item(&t_arr, 0);
            printf("\t\tThe data's type is %u and is %ld\n", n_tv_i.type,
                   n_tv_i.val.v_int);

            printf("\t\tGetting a NONE from the end of an array\n");
            nvf_tag_value tv_n = nvf_array_get_item(&t_arr, 1);
            printf("\t\tThis data's type is %u\n", tv_n.type);
        }
    }

    {
        nvf_array arr = {0};
        const char *a_names[] = {"map", "array"};
        printf("* Getting a nested array\n");
        rc = nvf_get_array(&root, a_names, 2, &arr);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting an array", rc, deinit);

        printf("\tGetting an array's value\n");
        nvf_tag_value tv = nvf_array_get_item(&arr, 0);
        printf("\tThe value has type %u and is %ld\n", tv.type, tv.val.v_int);

        printf("\tGetting an array's second value\n");
        nvf_tag_value tv_f = nvf_array_get_item(&arr, 1);
        printf("\tThe value has type %u and is %f\n", tv_f.type,
               tv_f.val.v_float);

        printf("\tGetting an array's third value\n");
        nvf_tag_value tv_s = nvf_array_get_item(&arr, 2);
        printf("\tThe value has type %u and is %s\n", tv_s.type,
               tv_s.val.v_string);

        printf("\tGetting an array's fourth value\n");
        nvf_tag_value tv_b = nvf_array_get_item(&arr, 3);
        printf("\tThe value has type %u and is 0x", tv_b.type);
        hexdump(tv_b.val.v_blob->data, tv_b.val.v_blob->len);
        printf("\n");

        printf("\tGetting a NONE from the end of an array\n");
        nvf_tag_value tv_n = nvf_array_get_item(&arr, 4);
        printf("\tThis data's type is %u\n", tv_n.type);
    }

    {
        char *str_out = NULL;
        uintptr_t str_len = 0;
        printf("* Getting map as a string\n");
        rc = nvf_default_root_to_str(&root, &str_out, &str_len);
        IF_GOTO_PRINT(rc != NVF_OK, "Getting an array", rc, deinit);
        printf("Rendered map: (\n%s\n)\n", str_out);
    }

deinit:
    rc = nvf_deinit(&root);
    if (rc != NVF_OK) {
        printf("Got error %s from deiniting the root\n", nvf_err_str(rc));
    }

    printf("Done\n");
    return rc == NVF_OK ? 0 : 1;
}
