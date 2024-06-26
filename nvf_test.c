
#include "nvf.h"

#include <stdio.h>
#include <string.h>

#define ASSERT_FLOAT(r, exp, ret_val, label)                                   \
    do {                                                                       \
        double _tmp_r = (r);                                                   \
        double _tmp_exp = (exp);                                               \
        if (_tmp_r != _tmp_exp) {                                              \
            printf("%s:%u. %s failed. Expected %f (%s), got %f (%s)!\n",       \
                   __FILE__, __LINE__, label, _tmp_exp, #exp, _tmp_r, #r);     \
            return ret_val;                                                    \
        }                                                                      \
    } while (0)

#define ASSERT_INT(r, exp, ret_val, label)                                     \
    do {                                                                       \
        int _tmp_r = (r);                                                      \
        int _tmp_exp = (exp);                                                  \
        if (_tmp_r != _tmp_exp) {                                              \
            printf("%s:%u. %s failed. Expected %d (%s), got %d (%s)!\n",       \
                   __FILE__, __LINE__, label, _tmp_exp, #exp, _tmp_r, #r);     \
            return ret_val;                                                    \
        }                                                                      \
    } while (0)

int main(int argc, char *argv[]) {
    nvf_root root = {0};

    // TODO: Add tests where all values are at the end of the string..
    const char int_test[] = "i_name 32343 # A comment\n"
                            "ix_name #[ another comment ]# 0x32343\n"
                            "io_name 032343\n"
                            "f_name 2.0\n"
                            "fx_name 0x2.0\n"
                            "s_name \"test\\nstr\"\n"
                            "ms_name \"multiline\"\n"
                            " 	 \"str\"\n"
                            "b_name bx01020304\n"
                            "a_name [32 2.0 \"str\" bx0708 [67]]\n"
                            "m_name {\n"
                            "	i_name 72333\n"
                            "	f_name 0.8\n"
                            "	s_name \"other test str\"\n"
                            "	b_name bx05060708090a0b0c0d\n"
                            "	a_name [128 0.4 \"str2\" bx090a0b]\n"
                            "}";
    uintptr_t test_len = strlen(int_test);

    nvf_err_data_i rd = nvf_parse_buf(int_test, test_len, &root);
    // I tried using memcmp(), but it compares uninitialized bytes and fails
    // because of that. Compare the variables directly to avoid this.
    ASSERT_INT(rd.data_i, 0, 1, "Testing init failure data index");
    ASSERT_INT(rd.err, NVF_NOT_INIT, 1, "Testing init failure error");

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
    ASSERT_FLOAT(bin_f, 2.0, 1, "Checking the float's value");

    const char *m_f_names[] = {"m_name", "f_name"};
    rc = nvf_get_float(&root, m_f_names, 2, &bin_f);
    ASSERT_INT(rc, NVF_OK, 1, "Getting a float");
    ASSERT_FLOAT(bin_f, 0.8, 1, "Checking the float's value");

    {
        char str_out[32] = {0};
        uintptr_t out_len = sizeof(str_out);
        const char *s_names[] = {"s_name"};
        rc = nvf_get_str(&root, s_names, 1, str_out, &out_len);
        ASSERT_INT(rc, NVF_OK, 1, "Getting a str");
        ASSERT_INT(strcmp("test\nstr", str_out), 0, 1,
                   "Comparing string results");
        ASSERT_INT(strlen("test\nstr") + 1, out_len, 1,
                   "Comparing string lengths");
    }
    {
        char str_out[32] = {0};
        uintptr_t out_len = sizeof(str_out);
        const char *s_names[] = {"ms_name"};
        rc = nvf_get_str(&root, s_names, 1, str_out, &out_len);
        ASSERT_INT(rc, NVF_OK, 1, "Getting a str");
        ASSERT_INT(strcmp("multilinestr", str_out), 0, 1,
                   "Comparing string results");
        ASSERT_INT(strlen("multilinestr") + 1, out_len, 1,
                   "Comparing string lengths");
    }
    {
        char str_out[32] = {0};
        uintptr_t out_len = sizeof(str_out);
        const char *s_names[] = {"m_name", "s_name"};
        rc = nvf_get_str(&root, s_names, 2, str_out, &out_len);
        ASSERT_INT(rc, NVF_OK, 1, "Getting a str");
        ASSERT_INT(strcmp("other test str", str_out), 0, 1,
                   "Comparing string results");
    }
    {
        uint8_t bin_out[32] = {0};
        uintptr_t bin_out_len = sizeof(bin_out);
        const char *b_names[] = {"b_name"};
        rc = nvf_get_blob(&root, b_names, 1, bin_out, &bin_out_len);
        ASSERT_INT(rc, NVF_OK, 1, "Getting a BLOB");
        uint8_t bin_exp[] = {1, 2, 3, 4};
        ASSERT_INT(bin_out_len, sizeof(bin_exp), 1, "Checking BLOB size");
        ASSERT_INT(memcmp(bin_exp, bin_out, bin_out_len), 0, 1,
                   "Checking BLOB results");
    }
    {
        uint8_t bin_out[32] = {0};
        uintptr_t bin_out_len = sizeof(bin_out);
        const char *b_names[] = {"m_name", "b_name"};
        rc = nvf_get_blob(&root, b_names, 2, bin_out, &bin_out_len);
        ASSERT_INT(rc, NVF_OK, 1, "Getting a BLOB");
        uint8_t bin_exp[] = {5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd};
        ASSERT_INT(bin_out_len, sizeof(bin_exp), 1, "Checking BLOB size");
        ASSERT_INT(memcmp(bin_exp, bin_out, bin_out_len), 0, 1,
                   "Checking BLOB results");
    }

    nvf_array arr = {0};
    const char *a_names = "a_name";
    rc = nvf_get_array(&root, &a_names, 1, &arr);
    ASSERT_INT(rc, NVF_OK, 1, "Getting an array");

    nvf_tag_value tv = nvf_array_get_item(&arr, 0);
    ASSERT_INT(tv.type, NVF_INT, 1, "Getting int type from array");
    ASSERT_INT(tv.val.v_int, 32, 1, "Getting int value from array");

    nvf_tag_value tv_f = nvf_array_get_item(&arr, 1);
    ASSERT_INT(tv_f.type, NVF_FLOAT, 1, "Getting float type from array");
    ASSERT_FLOAT(tv_f.val.v_float, 2.0, 1, "Getting float value from array");

    nvf_tag_value tv_s = nvf_array_get_item(&arr, 2);
    ASSERT_INT(tv_s.type, NVF_STRING, 1, "Getting string type from array");
    ASSERT_INT(strcmp(tv_s.val.v_string, "str"), 0, 1,
               "Getting string value from array");

    nvf_tag_value tv_b = nvf_array_get_item(&arr, 3);
    ASSERT_INT(tv_b.type, NVF_BLOB, 1, "Getting blob type from array");
    ASSERT_INT(tv_b.val.v_blob->len, 2, 1, "Getting blob length from array");
    uint8_t b_exp_val[] = {7, 8};
    ASSERT_INT(memcmp(tv_b.val.v_blob->data, b_exp_val, sizeof(b_exp_val)), 0,
               1, "Getting blob value from array");

    nvf_tag_value tv_a = nvf_array_get_item(&arr, 4);
    ASSERT_INT(tv_a.type, NVF_ARRAY, 1, "Getting array value from an iterator");

    {
        nvf_array t_arr = {0};
        rc = nvf_get_array_from_i(&root, tv_a.val.array_i, &t_arr);
        ASSERT_INT(rc, NVF_OK, 1, "Getting array index from array");

        nvf_tag_value n_tv_i = nvf_array_get_item(&t_arr, 0);
        ASSERT_INT(n_tv_i.type, NVF_INT, 1, "Checking nested array type");
        ASSERT_INT(n_tv_i.val.v_int, 67, 1, "Checking nested array value");
    }

    nvf_tag_value tv_n = nvf_array_get_item(&arr, 5);
    ASSERT_INT(tv_n.type, NVF_NONE, 1, "Getting none value from an iterator");

    {
        const char *am_names[] = {"m_name", "a_name"};
        rc = nvf_get_array(&root, am_names, 2, &arr);
        ASSERT_INT(rc, NVF_OK, 1, "Getting a nested array");

        nvf_tag_value m_tv = nvf_array_get_item(&arr, 0);
        ASSERT_INT(m_tv.type, NVF_INT, 1, "Getting int type from array");
        ASSERT_INT(m_tv.val.v_int, 128, 1, "Getting int value from array");

        nvf_tag_value m_tv_f = nvf_array_get_item(&arr, 1);
        ASSERT_INT(m_tv_f.type, NVF_FLOAT, 1, "Getting float type from array");
        ASSERT_FLOAT(m_tv_f.val.v_float, 0.4, 1,
                     "Getting float value from array");

        nvf_tag_value m_tv_s = nvf_array_get_item(&arr, 2);
        ASSERT_INT(m_tv_s.type, NVF_STRING, 1,
                   "Getting string type from array");
        ASSERT_INT(strcmp(m_tv_s.val.v_string, "str2"), 0, 1,
                   "Getting string value from array");

        nvf_tag_value m_tv_b = nvf_array_get_item(&arr, 3);
        ASSERT_INT(m_tv_b.type, NVF_BLOB, 1, "Getting blob type from array");
        ASSERT_INT(m_tv_b.val.v_blob->len, 3, 1,
                   "Getting blob length from array");
        uint8_t m_b_exp_val[] = {9, 0xa, 0xb};
        ASSERT_INT(
            memcmp(m_tv_b.val.v_blob->data, m_b_exp_val, sizeof(m_b_exp_val)),
            0, 1, "Getting blob value from array");

        nvf_tag_value m_tv_n = nvf_array_get_item(&arr, 4);
        ASSERT_INT(m_tv_n.type, NVF_NONE, 1,
                   "Getting none value from an iterator");
    }
    {
        const char *b_names[] = {"m_name", "b_name"};
        uint8_t *bin_out = NULL;
        uintptr_t bin_out_len;
        uint8_t bin_exp[] = {5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd};
        rc = nvf_get_blob_alloc(&root, b_names, 2, &bin_out, &bin_out_len);
        ASSERT_INT(rc, NVF_OK, 1, "Getting an allocated BLOB");
        ASSERT_INT(bin_out_len, sizeof(bin_exp), 1, "Checking BLOB size");
        ASSERT_INT(memcmp(bin_exp, bin_out, bin_out_len), 0, 1,
                   "Checking the blob value");

        root.free_inst(bin_out);
    }
    {
        const char *s_names[] = {"m_name", "s_name"};
        char *str_out = NULL;
        uintptr_t str_out_len;
        uint8_t str_exp[] = "other test str";
        rc = nvf_get_str_alloc(&root, s_names, 2, &str_out, &str_out_len);
        ASSERT_INT(rc, NVF_OK, 1, "Getting an allocated string");
        ASSERT_INT(str_out_len, sizeof(str_exp), 1, "Checking string length");
        ASSERT_INT(memcmp(str_exp, str_out, str_out_len), 0, 1,
                   "Checking the string value");

        root.free_inst(str_out);
    }
    {
        char *str_out = NULL;
        uintptr_t str_len = 0;
        rc = nvf_default_root_to_str(&root, &str_out, &str_len);
        ASSERT_INT(rc, NVF_OK, 1, "Converting the map to a str");
        printf("Rendered map: (\n%s\n)\n", str_out);
        ASSERT_INT(str_len, strlen(str_out) + 1, 1,
                   "Checking the output string length");

        root.free_inst(str_out);
    }

    rc = nvf_deinit(&root);
    ASSERT_INT(rc, NVF_OK, 1, "Deiniting the root");

    nvf_root zero_root = {0};
    ASSERT_INT(memcmp(&zero_root, &root, sizeof(zero_root)), 0, 1,
               "Checking deinited root is zero");

    rc = NVF_OK;
    for (const char *es = nvf_err_str(rc); rc <= NVF_ERR_END;
         ++rc, es = nvf_err_str(rc)) {
        ASSERT_INT(es != NULL, 1, 1, "Error string isn't NULL");
        printf("%u = %s\n", rc, es);
    }

    printf("All tests passed.\n");
    return 0;
}
