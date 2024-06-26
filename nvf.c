#include "nvf.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CASE_STR(str)                                                          \
    case str:                                                                  \
        return #str

const char *nvf_type_str(nvf_data_type dt) {
    switch (dt) {
        CASE_STR(NVF_NONE);
        CASE_STR(NVF_FLOAT);
        CASE_STR(NVF_INT);
        CASE_STR(NVF_BLOB);
        CASE_STR(NVF_STRING);
        CASE_STR(NVF_MAP);
        CASE_STR(NVF_ARRAY);
        CASE_STR(NVF_TYPE_END);
    default:
        return NULL;
    }
}

const char *nvf_err_str(nvf_err e) {

    switch (e) {
        CASE_STR(NVF_OK);
        CASE_STR(NVF_ERROR);
        CASE_STR(NVF_BAD_ALLOC);
        CASE_STR(NVF_BUF_OVF);
        CASE_STR(NVF_BAD_VALUE_FMT);
        CASE_STR(NVF_BAD_ARG);
        CASE_STR(NVF_BAD_DATA);
        CASE_STR(NVF_BAD_VALUE_TYPE);
        CASE_STR(NVF_NOT_INIT);
        CASE_STR(NVF_NOT_FOUND);
        CASE_STR(NVF_DUP_NAME);
        CASE_STR(NVF_UNMATCHED_BRACE);
        CASE_STR(NVF_NUM_OVF);
        CASE_STR(NVF_ERR_END);
    default:
        return NULL;
    }
}

// Return UINT8_MAX on error.
uint8_t nvf_hex_char_to_u8(char in) {
    IF_RET(in >= '0' && in <= '9', in - '0');
    IF_RET(in >= 'a' && in <= 'f', 10 + in - 'a');
    IF_RET(in >= 'A' && in <= 'F', 10 + in - 'A');

    return UINT8_MAX;
}

nvf_root nvf_root_init(realloc_fn realloc_inst, free_fn free_inst) {
    nvf_root r = {
        .realloc_inst = realloc_inst,
        .free_inst = free_inst,
        .init_val = NVF_INIT_VAL,
    };
    return r;
}

nvf_root nvf_root_default_init(void) { return nvf_root_init(realloc, free); }

nvf_err nvf_deinit_array(nvf_root *n_r, nvf_array *a) {
    IF_RET(n_r->init_val != NVF_INIT_VAL, NVF_NOT_INIT);
    free_fn f_fn = n_r->free_inst;
    IF_RET(f_fn == NULL, NVF_BAD_ARG);

    for (nvf_num i = 0; i < a->num; ++i) {
        if (a->types[i] == NVF_STRING) {
            f_fn(a->values[i].v_string);
        } else if (a->types[i] == NVF_BLOB) {
            f_fn(a->values[i].v_blob);
        }
    }
    f_fn(a->types);
    f_fn(a->values);

    bzero(a, sizeof(*a));
    return NVF_OK;
}
nvf_err nvf_deinit_map(nvf_root *n_r, nvf_map *m) {
    IF_RET(n_r->init_val != NVF_INIT_VAL, NVF_NOT_INIT);
    free_fn f_fn = n_r->free_inst;
    IF_RET(f_fn == NULL, NVF_BAD_ARG);

    // We need end because we zero the 'values' array. Make a copy to keep
    // it from changing.
    for (nvf_num i = 0, end = m->arr.num; i < end; ++i) {
        f_fn(m->names[i]);
    }
    nvf_err r = nvf_deinit_array(n_r, &m->arr);
    IF_RET(r != NVF_OK, r);
    f_fn(m->names);

    bzero(m, sizeof(*m));
    return NVF_OK;
}

nvf_err nvf_deinit(nvf_root *n_r) {
    IF_RET(n_r->init_val != NVF_INIT_VAL, NVF_NOT_INIT);
    free_fn f_fn = n_r->free_inst;
    IF_RET(f_fn == NULL, NVF_BAD_ARG);

    for (nvf_num a_i = 0; a_i < n_r->array_num; ++a_i) {
        nvf_err r = nvf_deinit_array(n_r, &n_r->arrays[a_i]);
        IF_RET(r != NVF_OK, r);
    }
    f_fn(n_r->arrays);

    for (nvf_num m_i = 0; m_i < n_r->map_num; ++m_i) {
        nvf_err r = nvf_deinit_map(n_r, &n_r->maps[m_i]);
        IF_RET(r != NVF_OK, r);
    }
    f_fn(n_r->maps);

    // This zeros out the init_value member too, which we absolutely want.
    // That prevents this struct from being passed into another function.
    bzero(n_r, sizeof(*n_r));
    return NVF_OK;
}

nvf_err nvf_get_map(nvf_root *root, const char **m_names, nvf_num name_depth,
                    nvf_map *map_out) {
    IF_RET(root == NULL || m_names == NULL || map_out == NULL, NVF_BAD_ARG);

    nvf_map *cur_map = root->maps;
    nvf_num n_i = 0;
    for (; n_i < name_depth; ++n_i) {
        nvf_num m_i = 0;
        for (; m_i < cur_map->arr.num; ++m_i) {
            if (strcmp(m_names[n_i], cur_map->names[m_i]) == 0) {
                if (cur_map->arr.types[m_i] == NVF_MAP) {
                    nvf_num new_map_i = cur_map->arr.values[m_i].map_i;
                    cur_map = &root->maps[new_map_i];
                    goto search_next;
                } else {
                    return NVF_NOT_FOUND;
                }
            }
        }
        // If we didn't find a name after going through the whole list, it's not
        // here. That's an error.
        IF_RET(m_i >= cur_map->arr.num, NVF_NOT_FOUND);
    search_next:;
    }
    if (n_i == name_depth) {
        *map_out = *cur_map;
        return NVF_OK;
    }

    return NVF_NOT_FOUND;
}

nvf_err nvf_get_value(nvf_root *root, const char **names, nvf_num name_depth,
                      void *out, uintptr_t *out_len, nvf_data_type dt) {
    IF_RET(root == NULL || names == NULL || out == NULL || name_depth == 0 ||
               out_len == NULL,
           NVF_BAD_ARG);

    // We assume this array of names is null terminated.
    nvf_map parent_map;
    // NOTE: this can overflow if name_depth == 0
    nvf_err e = nvf_get_map(root, names, name_depth - 1, &parent_map);
    IF_RET(e != NVF_OK, e);

    nvf_num n_i = 0;
    const char *name = names[name_depth - 1];
    for (; n_i < parent_map.arr.num; ++n_i) {
        if (strcmp(name, parent_map.names[n_i]) == 0) {
            if (parent_map.arr.types[n_i] == dt) {
                if (dt == NVF_BLOB) {
                    uintptr_t stored_len =
                        parent_map.arr.values[n_i].v_blob->len;
                    if (stored_len > *out_len) {
                        *out_len = stored_len;
                        return NVF_BUF_OVF;
                    }
                    *out_len = stored_len;
                    memcpy(out, parent_map.arr.values[n_i].v_blob->data,
                           stored_len);
                } else if (dt == NVF_STRING) {
                    uintptr_t stored_len =
                        strlen(parent_map.arr.values[n_i].v_string) + 1;
                    if (stored_len > *out_len) {
                        *out_len = stored_len;
                        return NVF_BUF_OVF;
                    }
                    *out_len = stored_len;
                    memcpy(out, parent_map.arr.values[n_i].v_string,
                           stored_len);
                } else if (dt == NVF_INT) {
                    int64_t *i_out = out;
                    *i_out = parent_map.arr.values[n_i].v_int;
                    *out_len = sizeof(*i_out);
                } else if (dt == NVF_FLOAT) {
                    double *f_out = out;
                    *f_out = parent_map.arr.values[n_i].v_float;
                    *out_len = sizeof(*f_out);
                } else if (dt == NVF_ARRAY) {
                    nvf_array *a_out = out;
                    nvf_num arr_out_i = parent_map.arr.values[n_i].array_i;
                    *a_out = root->arrays[arr_out_i];
                    *out_len = sizeof(*a_out);
                } else {
                    return NVF_BAD_VALUE_TYPE;
                }
                return NVF_OK;
            } else {
                return NVF_BAD_VALUE_TYPE;
            }
        }
    }

    return NVF_NOT_FOUND;
}

nvf_err nvf_get_value_alloc(nvf_root *root, const char **names,
                            nvf_num name_depth, void **out, uintptr_t *out_len,
                            nvf_data_type dt) {
    IF_RET(dt != NVF_BLOB && NVF_STRING != dt, NVF_BAD_ARG);
    // Call the get_value function once to get the item length before
    // allocating.
    uint8_t tmp_out[1] = {0};
    uintptr_t tmp_out_len = sizeof(tmp_out);
    nvf_err r =
        nvf_get_value(root, names, name_depth, tmp_out, &tmp_out_len, dt);
    IF_RET(r != NVF_OK && r != NVF_BUF_OVF, r);
    *out_len = tmp_out_len;

    // We have the length now. If it's one, our buffer already holds the answer.
    uint8_t *out_alloc = root->realloc_inst(NULL, tmp_out_len);
    IF_RET(out_alloc == NULL, NVF_BAD_ALLOC);
    *out = (void *)out_alloc;
    if (tmp_out_len == sizeof(tmp_out)) {
        *out_alloc = *tmp_out;
    } else {
        r = nvf_get_value(root, names, name_depth, *out, out_len, dt);
        IF_RET(r != NVF_OK, r);
    }
    return NVF_OK;
}

nvf_err nvf_get_blob_alloc(nvf_root *root, const char **names,
                           nvf_num name_depth, uint8_t **out,
                           uintptr_t *out_len) {
    return nvf_get_value_alloc(root, names, name_depth, (void **)out, out_len,
                               NVF_BLOB);
}

nvf_err nvf_get_str_alloc(nvf_root *root, const char **names,
                          nvf_num name_depth, char **out, uintptr_t *out_len) {
    return nvf_get_value_alloc(root, names, name_depth, (void **)out, out_len,
                               NVF_STRING);
}

nvf_tag_value nvf_array_get_item(const nvf_array *arr, nvf_num arr_i) {
    if (arr_i < arr->num) {
        nvf_tag_value r2 = {
            .val = arr->values[arr_i],
            .type = arr->types[arr_i],
        };
        return r2;
    }
    nvf_tag_value r = {
        .val = {0},
        .type = NVF_NONE,
    };
    return r;
}

nvf_err nvf_get_array_from_i(nvf_root *root, nvf_num arr_i, nvf_array *out) {
    IF_RET(arr_i >= root->array_num, NVF_BUF_OVF);
    *out = root->arrays[arr_i];
    return NVF_OK;
}

nvf_err nvf_get_array(nvf_root *root, const char **names, nvf_num name_depth,
                      nvf_array *out) {
    uintptr_t out_len = sizeof(*out);
    return nvf_get_value(root, names, name_depth, out, &out_len, NVF_ARRAY);
}

nvf_err nvf_get_float(nvf_root *root, const char **names, nvf_num name_depth,
                      double *out) {
    uintptr_t out_len = sizeof(*out);
    return nvf_get_value(root, names, name_depth, out, &out_len, NVF_FLOAT);
}

nvf_err nvf_get_blob(nvf_root *root, const char **names, nvf_num name_depth,
                     uint8_t *bin_out, uintptr_t *bin_out_len) {
    return nvf_get_value(root, names, name_depth, bin_out, bin_out_len,
                         NVF_BLOB);
}

// str_out_len should include the null terminator.
nvf_err nvf_get_str(nvf_root *root, const char **names, nvf_num name_depth,
                    char *str_out, uintptr_t *str_out_len) {
    return nvf_get_value(root, names, name_depth, str_out, str_out_len,
                         NVF_STRING);
}

nvf_err nvf_get_int(nvf_root *root, const char **names, nvf_num name_depth,
                    int64_t *out) {
    uintptr_t out_len = sizeof(*out);
    return nvf_get_value(root, names, name_depth, out, &out_len, NVF_INT);
}

uintptr_t nvf_next_token_i(const char *data, uintptr_t data_len) {
    uintptr_t d_i = 0;
    for (; d_i < data_len; ++d_i) {
        if (isspace(data[d_i])) {
            continue;
        }
        if (data[d_i] == '#') {
            ++d_i;
            if (d_i < data_len && data[d_i] == '[') {
            scan_till_bracket:
                for (; d_i < data_len && data[d_i] != ']'; ++d_i) {
                }
                ++d_i;
                // Keep looking for the end of the comment.
                if (d_i < data_len && data[d_i] != '#') {
                    goto scan_till_bracket;
                }
            } else {
                // Just go to a newline.
                for (; d_i < data_len && data[d_i] != '\n'; ++d_i) {
                }
            }
            continue;
        }
        break;
    }
    return d_i;
}

nvf_err nvf_ensure_array_cap(nvf_root *root, nvf_array *arr) {
    IF_RET(root == NULL || arr == NULL, NVF_BAD_ARG);

    if (arr->num + 1 > arr->cap) {
        nvf_num next_cap = arr->cap * 2 + 4;
        uint8_t *new_types =
            root->realloc_inst(arr->types, next_cap * sizeof(*arr->types));
        IF_RET(new_types == NULL, NVF_BAD_ALLOC);
        bzero(new_types + arr->num,
              sizeof(*arr->types) * (next_cap - arr->num));
        arr->types = new_types;

        nvf_value *new_values =
            root->realloc_inst(arr->values, next_cap * sizeof(*arr->values));
        IF_RET(new_values == NULL, NVF_BAD_ALLOC);
        bzero(new_values + arr->num,
              sizeof(*arr->values) * (next_cap - arr->num));
        arr->values = new_values;
        arr->cap = next_cap;
    }
    return NVF_OK;
}

nvf_err nvf_ensure_map_cap(nvf_root *root, nvf_map *m) {
    IF_RET(root == NULL, NVF_BAD_ARG);

    nvf_num old_cap = m->arr.cap;
    nvf_err e = nvf_ensure_array_cap(root, &m->arr);
    IF_RET(e != NVF_OK, e);

    // Only resize if we need to (when the internal array resized).
    nvf_num next_cap = m->arr.cap;
    if (old_cap != next_cap) {
        nvf_num next_cap = m->arr.cap;
        char **new_names =
            root->realloc_inst(m->names, next_cap * sizeof(*m->names));
        IF_RET(new_names == NULL, NVF_BAD_ALLOC);
        // Zero the new allocated pointers.
        bzero(new_names + m->arr.num,
              sizeof(*new_names) * (next_cap - m->arr.num));
        m->names = new_names;
    }

    return NVF_OK;
}

nvf_err_data_i nvf_parse_buf_map_arr(const char *data, uintptr_t data_len,
                                     nvf_root *root, nvf_num map_arr_i,
                                     nvf_parse_type p_type) {
    nvf_err_data_i r = {
        .data_i = 0,
        .err = NVF_OK,
    };
    IF_RET_DATA(root == NULL, r, NVF_BAD_ARG);
    IF_RET_DATA(p_type != NVF_PARSE_MAP && p_type != NVF_PARSE_ARRAY, r,
                NVF_BAD_ARG);
    IF_RET_DATA(root->init_val != NVF_INIT_VAL, r, NVF_NOT_INIT);

    // TODO: make a better error code for this.
    bool ovf_cond = p_type == NVF_PARSE_MAP ? map_arr_i >= root->map_num
                                            : map_arr_i >= root->array_num;
    IF_RET_DATA(ovf_cond, r, NVF_BUF_OVF);
    nvf_map *cur_map = p_type == NVF_PARSE_MAP ? root->maps + map_arr_i : NULL;
    nvf_array *cur_arr =
        cur_map == NULL ? root->arrays + map_arr_i : &cur_map->arr;
    for (; r.data_i < data_len; ++r.data_i) {
        r.data_i += nvf_next_token_i(data + r.data_i, data_len - r.data_i);
        IF_RET_DATA(r.data_i >= data_len, r, NVF_OK);
        if (data[r.data_i] == '}' || data[r.data_i] == ']') {
            // When parsing an array, encountering a brace might be valid since
            // We don't put values in an array by defult.
            IF_RET_DATA(data[r.data_i] == '}' && map_arr_i == 0, r,
                        NVF_UNMATCHED_BRACE);
            // Skip over the brace so the calling function doesn't detect it.
            ++r.data_i;
            r.err = NVF_OK;
            return r;
        }
        // TODO: Add multiline comment scanning between the names and values.

        const char *name = NULL;
        uintptr_t name_len = 0;
        // Skip checking names if we're not storing them anyway.
        if (cur_map != NULL) {
            name = &data[r.data_i];
            while (r.data_i < data_len && !isspace(data[r.data_i])) {
                char ch = data[r.data_i];
                if (ch == '{' || ch == '[') {
                    // Account for the extra step we took to see the
                    // brace/bracket
                    --r.data_i;
                    break;
                }
                r.data_i++;
            }
            IF_RET_DATA(data_len <= r.data_i, r, NVF_BUF_OVF);
            name_len = (data + r.data_i) - name;
            // Make sure the name doesn't collide with anything we already have.
            nvf_num n_i = 0;
            for (; n_i < cur_arr->num; ++n_i) {
                // The name we inferred isn't null terminated.
                // That means we can't use strcmp.
                // Do it more manually instead.
                // TODO: Add a test for this.
                size_t m_name_len = strlen(cur_map->names[n_i]);
                IF_RET_DATA(m_name_len == name_len &&
                                memcmp(cur_map->names[n_i], name, m_name_len) ==
                                    0,
                            r, NVF_BUF_OVF);
            }
        }

        r.data_i += nvf_next_token_i(data + r.data_i, data_len - r.data_i);
        IF_RET_DATA(r.data_i >= data_len, r, NVF_BUF_OVF);

        // We've found value. Parse it depending on what it is.
        const char *value = &data[r.data_i];
        if (data[r.data_i] == '-' || isdigit(data[r.data_i])) {
            // Int parsing has to be first because ints are valid floats.
            nvf_value npv = {0};
            nvf_data_type npt = NVF_INT;
            char *end = (char *)value;
            npv.v_int = strtoll(value, &end, 0);
            IF_RET_DATA(npv.v_int == LLONG_MAX || npv.v_int == LLONG_MIN, r,
                        NVF_NUM_OVF);
            IF_RET_DATA(end == value, r, NVF_BAD_VALUE_FMT);
            char end_c = *end;
            bool int_parse_failed =
                !(isspace(end_c) || end_c == '[' || end_c == ']');
            if (int_parse_failed) {
                // Try parsing as an int first, if it doesn't work, try a float
                // .
                npt = NVF_FLOAT;
                npv.v_float = strtod(value, &end);
                IF_RET_DATA(npv.v_float == HUGE_VAL ||
                                npv.v_float == HUGE_VALF ||
                                npv.v_float == HUGE_VALL,
                            r, NVF_NUM_OVF);
                IF_RET_DATA(end == value, r, NVF_BAD_VALUE_FMT);

                end_c = *end;
                bool float_parse_failed =
                    !(isspace(end_c) || end_c == '[' || end_c == ']');
                IF_RET_DATA(float_parse_failed, r, NVF_BAD_VALUE_FMT);
            }
            // Subtract one because the for loop will increment it anyway.
            r.data_i = end - data - 1;

            // Grow the current map if we need to.
            if (cur_map == NULL) {
                r.err = nvf_ensure_array_cap(root, cur_arr);
            } else {
                r.err = nvf_ensure_map_cap(root, cur_map);
            }
            IF_RET_DATA(r.err != NVF_OK, r, r.err);
            // The array's arrays could have been reallocated.
            // Update the pointers so they point to the right location.
            // cur_arr = &root->maps[map_arr_i].arr;

            // Now that we have enough memory, add the integer value to the map.
            cur_arr->values[cur_arr->num] = npv;
            cur_arr->types[cur_arr->num] = npt;
        } else if (data[r.data_i] == '"') {
            // Grow the current map if we need to.
            r.err = cur_map == NULL ? nvf_ensure_array_cap(root, cur_arr)
                                    : nvf_ensure_map_cap(root, cur_map);
            uintptr_t tot_str_len = 0;
            // Allow multiline strings kind of like C does
            // The final result will be a concatenation of the lines.
            // string_name "line 1"
            //             "line 2"
            // 'string_name' will be "line 1line2".
            char *d_str = cur_arr->values[cur_arr->num].v_string;
            while (true) {
                // r.data_i should be the quote's index. Add one to get the
                // first character of the string.
                ++r.data_i;
                uintptr_t str_start = r.data_i;
                // Find the end of the string.
                for (; r.data_i < data_len && data[r.data_i] != '"';
                     ++r.data_i) {
                    // Skip escape sequences.
                    if (data[r.data_i] == '\\') {
                        r.data_i++;
                    }
                }
                IF_RET_DATA(r.data_i >= data_len, r, NVF_BUF_OVF);
                // Don't include the end quote.
                uintptr_t str_part_len = r.data_i - str_start;

                IF_RET_DATA(r.err != NVF_OK, r, r.err);

                uintptr_t new_str_len = tot_str_len + str_part_len;
                d_str = root->realloc_inst(d_str, new_str_len + 1);
                IF_RET_DATA(d_str == NULL, r, NVF_BAD_ALLOC);
                d_str[new_str_len] = '\0';
                const char *d_str_start = data + str_start;
                bool esc_char = false;
                for (nvf_num s_i = 0; s_i < str_part_len; ++s_i) {
                    if (d_str_start[s_i] == '\\') {
                        esc_char = true;
                    } else if (esc_char) {
                        esc_char = false;
                        bool assigned = false;
#define MATCH_ASSIGN(i_char, e_char, dest, src, assigned)                      \
    do {                                                                       \
        if (i_char == src) {                                                   \
            dest = e_char;                                                     \
            assigned = true;                                                   \
        }                                                                      \
    } while (0)
                        MATCH_ASSIGN('n', '\n', d_str[tot_str_len],
                                     d_str_start[s_i], assigned);
                        MATCH_ASSIGN('t', '\t', d_str[tot_str_len],
                                     d_str_start[s_i], assigned);
                        MATCH_ASSIGN('r', '\r', d_str[tot_str_len],
                                     d_str_start[s_i], assigned);
                        MATCH_ASSIGN('"', '"', d_str[tot_str_len],
                                     d_str_start[s_i], assigned);
                        MATCH_ASSIGN('\\', '\\', d_str[tot_str_len],
                                     d_str_start[s_i], assigned);
                        IF_RET_DATA(assigned == false, r, NVF_BAD_DATA);
                        ++tot_str_len;
                    } else {
                        d_str[tot_str_len] = d_str_start[s_i];
                        ++tot_str_len;
                    }
                }
                // We're sitting at the quote, get past it so getting the next
                // token works.
                ++r.data_i;
                r.data_i +=
                    nvf_next_token_i(r.data_i + data, data_len - r.data_i);
                // If the next token isn't a string, then we're done parsing
                // this string.
                if (data[r.data_i] != '"') {
                    // Decrement because the for loop will increment again.
                    --r.data_i;
                    break;
                }
            }
            // Now that we have enough memory, add the string to the map.
            cur_arr->values[cur_arr->num].v_string = d_str;
            cur_arr->types[cur_arr->num] = NVF_STRING;
        } else if (data[r.data_i] == 'b') {
            // This case could be a blob.
            // Make sure there's space for 'x' and one nibble of data.
            IF_RET_DATA(r.data_i + 2 >= data_len, r, NVF_BUF_OVF);
            ++r.data_i;
            IF_RET_DATA(data[r.data_i] != 'x', r, NVF_BAD_VALUE_FMT);
            ++r.data_i;
            uintptr_t blob_start = r.data_i;
            // Find the length of the BLOB before allocating memory.
            for (; r.data_i < data_len && isxdigit(data[r.data_i]);
                 ++r.data_i) {
            }
            uintptr_t blob_len = r.data_i - blob_start;
            IF_RET_DATA(blob_len == 0, r, NVF_BAD_VALUE_FMT);
            // The for loop will increment this later. decrement it to account
            // for that.
            --r.data_i;

            // Grow the current map if we need to.
            if (cur_map == NULL) {
                r.err = nvf_ensure_array_cap(root, cur_arr);
            } else {
                r.err = nvf_ensure_map_cap(root, cur_map);
            }
            IF_RET_DATA(r.err != NVF_OK, r, r.err);

            nvf_blob **map_blob = &cur_arr->values[cur_arr->num].v_blob;
            uintptr_t bin_blob_len = (blob_len + 1) / 2;
            nvf_blob *blob =
                root->realloc_inst(*map_blob, sizeof(*blob) + bin_blob_len);
            IF_RET_DATA(blob == NULL, r, NVF_BAD_ALLOC);
            blob->len = bin_blob_len;

            // Get the data into the memory we allocated.
            bool first_nibble = true;
            for (uintptr_t b_i = 0; b_i < blob_len; ++b_i) {
                uintptr_t b_d_i = blob_start + b_i;
                uint8_t bin_val = nvf_hex_char_to_u8(data[b_d_i]);
                IF_RET_DATA(bin_val == UINT8_MAX, r, NVF_BAD_VALUE_FMT);

                if (first_nibble) {
                    blob->data[b_i / 2] = bin_val << 4;
                    first_nibble = false;
                } else {
                    blob->data[b_i / 2] |= bin_val;
                    first_nibble = true;
                }
            }
            *map_blob = blob;
            cur_arr->types[cur_arr->num] = NVF_BLOB;
        } else if (data[r.data_i] == '{') {
            // Don't allow maps to be nested in arrays.
            IF_RET_DATA(p_type == NVF_PARSE_ARRAY || cur_map == NULL, r,
                        NVF_ERROR);
            ++r.data_i;
            // Allocate a new map, then parse the data in the new map.
            if (root->map_num + 1 > root->map_cap) {
                // TODO: Make a macro for the 8 constant.
                nvf_num new_cap = root->map_cap * 2 + 4;
                nvf_map *new_map =
                    root->realloc_inst(root->maps, new_cap * sizeof(*new_map));
                IF_RET_DATA(new_map == NULL, r, NVF_BAD_ALLOC);
                bzero(new_map + root->map_num,
                      (new_cap - root->map_num) * sizeof(*new_map));
                root->maps = new_map;
                root->map_cap = new_cap;
            }
            cur_map = root->maps + map_arr_i;
            cur_arr = &root->maps[map_arr_i].arr;

            // Grow the current map if we need to.
            if (cur_map == NULL) {
                r.err = nvf_ensure_array_cap(root, cur_arr);
            } else {
                r.err = nvf_ensure_map_cap(root, cur_map);
            }
            IF_RET_DATA(r.err != NVF_OK, r, r.err);

            // TODO: See if we should increment map_num before or after this
            // function.
            ++root->map_num;
            nvf_num new_map_num = root->map_num;
            nvf_err_data_i r2 =
                nvf_parse_buf_map_arr(data + r.data_i, data_len - r.data_i,
                                      root, new_map_num - 1, NVF_PARSE_MAP);
            r.data_i += r2.data_i;
            // The for loop will increment this. Decrement it to account for
            // that.
            --r.data_i;
            r.err = r2.err;
            IF_RET(r.err != NVF_OK, r);

            // The pointer to the map array may have moved, udpate it.
            if (cur_map == NULL) {
                cur_arr = root->arrays + map_arr_i;
            } else {
                cur_map = root->maps + map_arr_i;
                cur_arr = &cur_map->arr;
            }

            cur_arr->values[cur_arr->num].map_i = new_map_num - 1;
            cur_arr->types[cur_arr->num] = NVF_MAP;
        } else if (data[r.data_i] == '[') {
            ++r.data_i;
            // Allocate a new map, then parse the data in the new map.
            if (root->array_num + 1 > root->array_cap) {
                // TODO: Make a macro for the 8 constant.
                nvf_num new_cap = root->array_cap * 2 + 4;
                nvf_array *new_arr = root->realloc_inst(
                    root->arrays, new_cap * sizeof(*new_arr));
                IF_RET_DATA(new_arr == NULL, r, NVF_BAD_ALLOC);
                bzero(new_arr + root->array_num,
                      (new_cap - root->array_num) * sizeof(*new_arr));
                root->arrays = new_arr;
                root->array_cap = new_cap;
            }
            // The pointer value will be different depending on if we're parsing
            // an array or a map. Make sure we account for that here. If we're
            // parsing a map, our current array won't be changed.
            cur_arr = cur_map == NULL ? root->arrays + map_arr_i : cur_arr;

            if (cur_map == NULL) {
                r.err = nvf_ensure_array_cap(root, cur_arr);
            } else {
                r.err = nvf_ensure_map_cap(root, cur_map);
            }

            ++root->array_num;
            // The array number may change if there's nested arrays. Save it
            // just in case.
            nvf_num new_arr_num = root->array_num;

            nvf_err_data_i r2 =
                nvf_parse_buf_map_arr(data + r.data_i, data_len - r.data_i,
                                      root, new_arr_num - 1, NVF_PARSE_ARRAY);
            r.data_i += r2.data_i;
            // The for loop will increment this. Decrement it to account for
            // that.
            --r.data_i;
            r.err = r2.err;
            IF_RET(r.err != NVF_OK, r);

            // The pointer to the map array may have moved, udpate it.
            if (cur_map == NULL) {
                cur_arr = root->arrays + map_arr_i;
            } else {
                cur_map = root->maps + map_arr_i;
                cur_arr = &cur_map->arr;
            }

            cur_arr->values[cur_arr->num].array_i = new_arr_num - 1;
            cur_arr->types[cur_arr->num] = NVF_ARRAY;
        } else {
            r.err = NVF_BAD_VALUE_TYPE;
            return r;
        }
        if (cur_map != NULL) {
            char *name_mem =
                root->realloc_inst(cur_map->names[cur_arr->num], name_len + 1);
            IF_RET_DATA(name_mem == NULL, r, NVF_BAD_ALLOC);
            // Make sure we have a null terminator like all good C strings do.
            name_mem[name_len] = '\0';
            memcpy(name_mem, name, name_len);
            cur_map->names[cur_arr->num] = name_mem;
        }
        cur_arr->num++;
    }

    return r;
}

// We reallocate instead of mallocing just in case the old pointer points to
// allocated memory. That means we don't really need to do cleanup if the old
// pointer points to allocated memory. That implies we need to zero all the
// pointers in the struct before they are passed into this function.
// TODO: Re-think how to make cleanup simpler if an allocation fails.
nvf_err_data_i nvf_parse_buf(const char *data, uintptr_t data_len,
                             nvf_root *out_root) {
    nvf_err_data_i r = {
        .data_i = 0,
        .err = NVF_OK,
    };
    IF_RET_DATA(out_root == NULL, r, NVF_BAD_ARG);
    IF_RET_DATA(out_root->init_val != NVF_INIT_VAL, r, NVF_NOT_INIT);

    // Allocate space for the first map.
    if (out_root->map_cap == 0 || out_root->maps == NULL) {
        nvf_map *new_map = out_root->realloc_inst(NULL, sizeof(*new_map));
        IF_RET_DATA(new_map == NULL, r, NVF_BAD_ALLOC);
        bzero(new_map, sizeof(*new_map));
        out_root->maps = new_map;
        out_root->map_num = 1;
        out_root->map_cap = 1;
    }
    // Use map_num - 1 so we can try parsing again, or parse multiple buffers
    // with multiple function calls.
    return nvf_parse_buf_map_arr(data, data_len, out_root,
                                 out_root->map_num - 1, NVF_PARSE_MAP);
}

char nvf_bin_to_char(uint8_t byte) {
    IF_RET(byte >= 16, '\0');
    IF_RET(byte >= 10, byte - 10 + 'a');
    return byte + '0';
}

nvf_err nvf_map_arr_to_str(nvf_root *root, char **out, uintptr_t *out_len,
                           nvf_num map_arr_i, str_fmt_fn fmt_fn,
                           nvf_parse_type pt, nvf_num indent_i) {
    nvf_map *iter = NULL;
    nvf_array *arr = NULL;
    if (pt == NVF_PARSE_MAP) {
        iter = root->maps + map_arr_i;
        arr = &iter->arr;
    } else if (pt == NVF_PARSE_ARRAY) {
        arr = root->arrays + map_arr_i;
    } else {
        return NVF_BAD_ARG;
    }
    for (nvf_num m_i = 0; m_i < arr->num; ++m_i) {
        int len = 0;
        nvf_data_type dt = arr->types[m_i];
        char *name = iter == NULL ? "" : iter->names[m_i];
        nvf_value nv = arr->values[m_i];
        nvf_err r = NVF_OK;
        if (dt == NVF_INT) {
            len = fmt_fn(NULL, 0, "%s %ld\n", name, nv.v_int);
        } else if (dt == NVF_FLOAT) {
            len = fmt_fn(NULL, 0, "%s %f\n", name, nv.v_float);
        } else if (dt == NVF_STRING) {
            len = fmt_fn(NULL, 0, "%s \"%s\"\n", name, nv.v_string);
        } else if (dt == NVF_BLOB) {
            len = fmt_fn(NULL, 0, "%s bx\n", name);
            if (len < 0) {
                root->free_inst(*out);
                return NVF_ERROR;
            }
            len += 2 * nv.v_blob->len;
        } else if (dt == NVF_MAP || dt == NVF_ARRAY) {
            char start_c = dt == NVF_MAP ? '{' : '[';
            len = strlen(name) > 0 ? fmt_fn(NULL, 0, "%s %c\n", name, start_c)
                                   : fmt_fn(NULL, 0, "%c\n", start_c);
            if (len < 0) {
                root->free_inst(*out);
                return NVF_ERROR;
            }
            len += 1;
            uintptr_t new_len = *out_len + len;
            char *new_out = root->realloc_inst(*out, new_len);
            if (new_out == NULL) {
                root->free_inst(*out);
                return NVF_BAD_ALLOC;
            }
            *out = new_out;

            char *out_start = *out + *out_len - 1;
            memset(out_start, '\t', indent_i);
            out_start += indent_i;

            int fmt_r = strlen(name) > 0
                            ? fmt_fn(out_start, len, "%s %c\n", name, start_c)
                            : fmt_fn(out_start, len, "%c\n", start_c);
            if (fmt_r < 0) {
                root->free_inst(*out);
                return NVF_BAD_ALLOC;
            }
            *out_len += len + indent_i - 1;

            nvf_parse_type new_pt =
                dt == NVF_MAP ? NVF_PARSE_MAP : NVF_PARSE_ARRAY;
            nvf_num next_i = dt == NVF_MAP ? nv.map_i : nv.array_i;
            r = nvf_map_arr_to_str(root, out, out_len, next_i, fmt_fn, new_pt,
                                   indent_i + 1);
            if (r != NVF_OK) {
                root->free_inst(*out);
                return r;
            }
            // Account for the closing brace/bracket and a \n
            len = 2;
        } else {
            // We're assuming free is null safe here.
            root->free_inst(*out);
            return NVF_BAD_VALUE_TYPE;
        }
        if (len < 0) {
            root->free_inst(*out);
            return NVF_ERROR;
        }
        // Add one to the length to account for the NULL terminator.
        len += 1;

        if (len > 0) {
            uintptr_t new_len = *out_len + len + indent_i;
            char *new_out = root->realloc_inst(*out, new_len);
            if (new_out == NULL) {
                root->free_inst(*out);
                return NVF_BAD_ALLOC;
            }
            *out = new_out;
        }

        // Subtract one to account for the NULL terminator.
        char *out_end = *out + *out_len - 1;
        int fmt_r = 0;
        // Add the indent level we need
        memset(out_end, '\t', indent_i);
        out_end += indent_i;
        if (dt == NVF_INT) {
            fmt_r = fmt_fn(out_end, len, "%s %ld\n", name, nv.v_int);
        } else if (dt == NVF_FLOAT) {
            fmt_r = fmt_fn(out_end, len, "%s %f\n", name, nv.v_float);
        } else if (dt == NVF_STRING) {
            fmt_r = fmt_fn(out_end, len, "%s \"%s\"\n", name, nv.v_string);
        } else if (dt == NVF_BLOB) {
            fmt_r = fmt_fn(out_end, len, "%s bx", name);
            if (fmt_r < 0) {
                root->free_inst(*out);
                return NVF_ERROR;
            }
            uint8_t *bin_data = nv.v_blob->data;
            uintptr_t bin_len = nv.v_blob->len;
            char *hex_start = out_end + fmt_r;
            for (uintptr_t bin_i = 0; bin_i < bin_len; ++bin_i) {
                char tmp = nvf_bin_to_char(bin_data[bin_i] >> 4);
                if (tmp == '\0') {
                    root->free_inst(*out);
                    return NVF_BAD_DATA;
                }
                hex_start[2 * bin_i] = tmp;

                tmp = nvf_bin_to_char(bin_data[bin_i] & 0xf);
                if (tmp == '\0') {
                    root->free_inst(*out);
                    return NVF_BAD_DATA;
                }
                hex_start[2 * bin_i + 1] = tmp;
            }
            hex_start[2 * bin_len] = '\n';
        } else if (dt == NVF_MAP || dt == NVF_ARRAY) {
            char start_c = dt == NVF_MAP ? '}' : ']';
            fmt_r = fmt_fn(out_end, len, "%c\n", start_c);
        }
        if (fmt_r < 0) {
            root->free_inst(*out);
            return NVF_ERROR;
        }
        // NOTE: This function probably has a buffer overflow somehwere.
        // Doing this kind of stuff with C strings is hard for me.
        *out_len += len + indent_i - 1;
    }

    return NVF_OK;
}

nvf_err nvf_root_to_str(nvf_root *root, char **out, uintptr_t *out_len,
                        str_fmt_fn fmt_fn) {
    IF_RET(root == NULL || out == NULL || out_len == NULL, NVF_BAD_ARG);
    // Iterate through the structure and append it to the string.
    // Use the allocator to allocate the string.

    IF_RET(root->map_num == 0, NVF_OK);
    // Allocate one byte for the NULL terminator.
    *out = root->realloc_inst(NULL, 1);
    IF_RET(*out == NULL, NVF_BAD_ALLOC);
    *out[0] = '\0';
    *out_len = 1;

    return nvf_map_arr_to_str(root, out, out_len, 0, fmt_fn, NVF_PARSE_MAP, 0);
}

nvf_err nvf_default_root_to_str(nvf_root *root, char **out,
                                uintptr_t *out_len) {
    return nvf_root_to_str(root, out, out_len, snprintf);
}
