#pragma once
#include <stddef.h>
#include <stdint.h>

/**
    \file
    The main NVF header.
*/
/**
    \mainpage NVF (Name Value Format)

    \section ovr Overview

    This project is a simple configuration format that tries to solve a
    first world problem. While JSON's wide adoption has been pretty helpful for
    the industry, allowing wide parser and language interoperability through
    plaintext, I think JSON is more complicated that it needs to be. This format
    probably doesn't have JSON's feature set (I don't even know what the JSON
    feature set is), but if all you need is a way to associate a name with a
    value in a configuration file, this format will do.

    Here's what the format looks like:
    \code{.uparsed}
            # This is a line comment.
            #[ This is a longer,
               multi-line comment. ]#
            int 32343
            hex_int 0x32343
            octal_int 032343
            float 2.0
            hex_float 0x2.0
            string "string"
            multiline_string "multiline\n"
                             " string"
            BLOB bx010203040506070809
            array [32 2.0 "string" bx0708 [67]]
            map {
                    int 72333
                    float 0.8
                    string "other string"
                    BLOB bx05060708090a0b0c0d
                    array [128 0.4 "a string" bx090a0b]
            }
    \endcode

    See the source of \ref nvf_example.c for API usage examples.

    \section build Building
    The full build requires make, a C compiler (gcc is preferred), clang-format,
    and doxygen. After installing those, run make in the same folder as the make
    file.
    \code{.unparsed}
    $ make
    \endcode
    That will build the docs, library, example and test executable.
    It runs the example and the tests too.

    Run the tests by building this target.
    \code{.unparsed}
    $ make test
    \endcode

    Run the examples by building this target.
    \code{.unparsed}
    $ make example
    \endcode

    Build the docs by building this target.
    \code{.unparsed}
    $ make doc
    \endcode

    Build this target to format the C code.
    \code{.unparsed}
    $ make fmt
    \endcode
 */

/// The NVF error/return codes.
typedef enum {
    NVF_OK = 0,          ///< Success
    NVF_ERROR,           ///< Generic error
    NVF_BAD_ALLOC,       ///< Allocation failed
    NVF_BUF_OVF,         ///< Buffer overflow
    NVF_BAD_VALUE_FMT,   ///< Bad value format
    NVF_BAD_ARG,         ///< Bad argument
    NVF_BAD_DATA,        ///< Bad data
    NVF_BAD_VALUE_TYPE,  ///< Unexpected value type
    NVF_NOT_INIT,        ///< Struct wasn't initialized
    NVF_NOT_FOUND,       ///< Name not found
    NVF_DUP_NAME,        ///< Name already exists
    NVF_UNMATCHED_BRACE, ///< Found brace or bracket without a match
    NVF_NUM_OVF,         ///< Number is too big to be represented
    NVF_ERR_END,         ///< An end sentinel
} nvf_err;

/// The type of parsing to do when parsing the NVF format.
typedef enum {
    NVF_PARSE_ARRAY = 0, ///< Parse an array
    NVF_PARSE_MAP,       ///< Parse a map
} nvf_parse_type;

/// The type of data in an element
typedef enum {
    NVF_NONE = 0, ///< No value
    NVF_FLOAT,    ///< Floating point number
    NVF_INT,      ///< Integer number
    NVF_BLOB,     ///< Big-endian, binary data
    NVF_STRING,   ///< A C string
    NVF_MAP,      ///< A map (names associated with values)
    NVF_ARRAY,    ///< An array (values without names)
    NVF_TYPE_END, ///< An end sentinel
} nvf_data_type;

/// This keeps track of how many elements are in something
typedef uint32_t nvf_num;

/// Holds large amounts of binary data
typedef struct nvf_blob {
    nvf_num len;    ///< The data length in bytes
    uint8_t data[]; ///< The data itself
} nvf_blob;

/// The possible values used in NVF
typedef union nvf_value {
    nvf_num map_i;    ///< Index into the maps array
    nvf_num array_i;  ///< Index into the arrays array
    int64_t v_int;    ///< Integer
    double v_float;   ///< Floating point number
    char *v_string;   ///< C string
    nvf_blob *v_blob; ///< Binary data
} nvf_value;

/// Holds values without names
typedef struct nvf_array {
    uint8_t *types;    ///< The data type of each element
    nvf_value *values; ///< The actual value of each element
    nvf_num num,       ///< The number of elements in the array.
        cap;           ///< The array's capacity
} nvf_array;

/// Holds values associated with names
typedef struct nvf_map {
    char **names;  ///< The names for each value
    nvf_array arr; ///< where the values are stored
} nvf_map;

/// The function signature for realloc()
typedef void *(*realloc_fn)(void *, size_t);

/// The function signature for free()
typedef void (*free_fn)(void *);

/// The function signature for snprintf()
typedef int (*str_fmt_fn)(char *s, size_t n, const char *format, ...);

/// A magic init value to see if the NVF root is setup before use.
#define NVF_INIT_VAL (0x72)

/// Return \a rv if \a expr is true
#define IF_RET(expr, rv)                                                       \
    do {                                                                       \
        if ((expr)) {                                                          \
            return rv;                                                         \
        }                                                                      \
    } while (0)

/// Set \a rv.err to e and return \a rv if \a expr is true
#define IF_RET_DATA(expr, rv, e)                                               \
    do {                                                                       \
        if ((expr)) {                                                          \
            rv.err = e;                                                        \
            return rv;                                                         \
        }                                                                      \
    } while (0)

// These are in one struct because the allocator and the memory used in the
// base map are linked. You neeed to keep the allocators and the map together
// because you'll need to manipulate the memory allocated with the same
// allocator. NOTE: I tried to make this structure flat, using arrays to hold
// differently typed values, but I ran into a problem. I couldn't know how deep
// an item was in a structure. To search for a named value, I could look
// through the array of all the names, but that array didn't have the nesting
// level of the name. I could add a nesting level, but that could make search
// times bad for an unsuccessful linear search. I may go back to having a root
// map, and storing items in tagged unions. I'll probably have to use void* to
// nest maps in maps. Making maps self referential is difficult otherwise. I
// had an epiphany. Only the names need to be nested. The values don't. I can
// store all the blobs and types and values in one array, with indicies
// referencing it if necessary. I'm not sure how efficient that will be though.
// I need to think about this further. There's only an extra storage cost if I
// add an extra pointer to something. BLOBS don't cost extra because I need to
// store data behind a pointer anyway. A reference to a map might cost extra
// because that could be an index into an array of maps. Same for arrays. There
// will be pointers to those, but they could be indexes into an array of
// arrays.

/// Holds all the maps and arrays from parsing an NVF file
typedef struct nvf_root {
    realloc_fn
        realloc_inst;  ///< The function that allocates memory for this root
    free_fn free_inst; ///< A function that frees memory for this root

    nvf_num array_num, ///< The number of arrays
        array_cap;     ///< The array capacity
    nvf_array *arrays; ///< Array storage

    nvf_num map_num, ///< The number of maps stored
        map_cap;     ///< Map storage capacity
    nvf_map *maps;   ///< Map storage
    uint8_t
        init_val; ///< Set to \ref NVF_INIT_VAL when this struct is initialized.
} nvf_root;

// A return type used to figure out where the called function stopped while
// parsing.
// TODO: Maybe make data_i a u32. That should match the alignment and size of
// nvf_err better.
// TODO: typedef nvf_err to a u32 and make the error codes into a different
// enum (like nvf_err_e).

/// The return value from parsing NVF data.
typedef struct {
    uintptr_t data_i; ///< Last index touched before an error.
    nvf_err err;      ///< A return code
} nvf_err_data_i;

typedef struct {
    const nvf_value val;
    const nvf_data_type type;
} nvf_tag_value;

nvf_root nvf_root_init(realloc_fn realloc_inst, free_fn free_inst);

nvf_root nvf_root_default_init(void);

nvf_err_data_i nvf_parse_buf(const char *data, uintptr_t data_len,
                             nvf_root *out_root);

nvf_err nvf_get_int(nvf_root *root, const char **names, nvf_num name_depth,
                    int64_t *out);

nvf_err nvf_get_map(nvf_root *root, const char **m_names, nvf_num name_depth,
                    nvf_map *map_out);

nvf_err nvf_deinit(nvf_root *n_r);

nvf_err nvf_get_float(nvf_root *root, const char **names, nvf_num name_depth,
                      double *out);

nvf_err nvf_get_str(nvf_root *root, const char **names, nvf_num name_depth,
                    char *str_out, uintptr_t *str_out_len);

nvf_err nvf_get_blob(nvf_root *root, const char **names, nvf_num name_depth,
                     uint8_t *bin_out, uintptr_t *bin_out_len);

nvf_err nvf_get_array(nvf_root *root, const char **names, nvf_num name_depth,
                      nvf_array *out);

nvf_tag_value nvf_array_get_item(const nvf_array *arr, nvf_num arr_i);

nvf_err nvf_get_blob_alloc(nvf_root *root, const char **names,
                           nvf_num name_depth, uint8_t **out,
                           uintptr_t *out_len);

nvf_err nvf_get_str_alloc(nvf_root *root, const char **names,
                          nvf_num name_depth, char **out, uintptr_t *out_len);

nvf_err nvf_get_array_from_i(nvf_root *root, nvf_num arr_i, nvf_array *out);

nvf_err nvf_root_to_str(nvf_root *root, char **out, uintptr_t *out_len,
                        str_fmt_fn fmt_fn);

nvf_err nvf_default_root_to_str(nvf_root *root, char **out, uintptr_t *out_len);

const char *nvf_err_str(nvf_err e);

char nvf_bin_to_char(uint8_t byte);

const char *nvf_type_str(nvf_data_type dt);
