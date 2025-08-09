#pragma once

#include "structs.h"



typedef struct {
    const bool   is_error;
    const bool*  expect_fail_ids;
    const size_t capacity; 
    size_t       num_files;
    string*      filenames;
    string*      file_outputs;
} folder_storage;



[[nodiscard]] bool           write_file(const string filename, const string output);
[[nodiscard]] string         read_file (const char* filename);
[[nodiscard]] folder_storage get_tests (const char* path, const size_t capacity, bool* expect_fail_ids, string* filename_storage, string* file_storage);
[[nodiscard]] int            compile_with_gcc(const string filename);



