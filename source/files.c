

#include "files.h"

#include "structs.h"
#include "logger.h"

#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>



extern bool DEBUG;
extern bool TEST;



bool write_file(const char* filename, const string output) {
    FILE *fp = fopen(filename, "w");  // open for writing (creates or truncates)
    if (fp == NULL) {
        perror("fopen");
        return false;
    }

    // 1) fprintf: returns a negative value on error
    if (fprintf(fp, "%.*s", (int) output.size, output.data) < 0) {
        perror("fprintf");
        fclose(fp);
        return false;
    }


    // 5) fflush: ensure all data is pushed out (optional)
    if (fflush(fp) != 0) {
        perror("fflush");
        fclose(fp);
        return false;
    }

    // 6) fclose: returns zero on success
    if (fclose(fp) != 0) {
        perror("fclose");
        return false;
    }

    return true;
}



string read_file(const char* filename) {

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        FATAL_ERROR("Failed to open file: %s (%s)", strerror(errno), filename);
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    const size_t size = (size_t) ftell(file);
    if (size > 1024 * 1024 * 10) {
        FATAL_ERROR("File is way too large, size %zumb (%s)", size / (1024 * 1024), filename);
    }
    fseek(file, 0, SEEK_SET);

    if (ferror(file)) {
        fclose(file);
        FATAL_ERROR("Failed to read file: %s (%s)", strerror(errno), filename);
    }

    // Allocate buffer
    char* buffer = calloc(size + 1, sizeof(char));
    if (buffer == NULL) {
        fclose(file);
        FATAL_ERROR("Failed to allocate memory for file: %s (%s)", strerror(errno), filename);
    }

    // Read entire file
    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);
    
    return (string){buffer,  size};
}

void read_test_file(char* full_path, size_t num_files, bool* expect_fail_ids, string* filename_storage, string* file_storage) {

    const string expect_fail_str = (string){"[[expect_fail]]", 15};

    const size_t size = strlen(full_path) + 1;
    char* file_name   = calloc(size, sizeof(char)); 
    snprintf(file_name, size, "%s", full_path);

    const string filename_str = (string){file_name, size - 1};
    memcpy( (void*) &filename_storage[num_files], (const void*) &filename_str, sizeof(string));



    const string raw_file_output_str = read_file(filename_str.data);

    size_t offset = 0;

    if (raw_file_output_str.size >= expect_fail_str.size && strncmp(raw_file_output_str.data, expect_fail_str.data, expect_fail_str.size) == 0) {
        expect_fail_ids[num_files] = true; 
        offset = expect_fail_str.size;
    }

    char* new_file_output = calloc(raw_file_output_str.size - offset, sizeof(char)); 
    memcpy( (void*) new_file_output, (const void*) (raw_file_output_str.data + offset), raw_file_output_str.size - offset);
    free((void*) raw_file_output_str.data);

    const string file_output_str = {new_file_output, raw_file_output_str.size - offset};

    memcpy( (void*) &file_storage[num_files], (const void*) &file_output_str, sizeof(string));
}

#define GET_TESTS_ERR(fmt, ...)                     \
    do {                                            \
    FATAL_ERROR(fmt __VA_OPT__(,) __VA_ARGS__);     \
    return (folder_storage){.is_error=true};        \
    } while (0)

folder_storage get_tests(const char* path, const size_t capacity, bool* expect_fail_ids, string* filename_storage, string* file_storage) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024];
    char inner_path[1024];
    
    if (!dir) {
        GET_TESTS_ERR("Failed to open folder (%s): %s", strerror(errno), path); }

    size_t num_files = 0;
    
    while ( (entry = readdir(dir)) != NULL ) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; }
            
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_stat) != 0) {
            continue; }

        if (S_ISDIR(file_stat.st_mode)) { // Inner directory

            DIR *inner_dir = opendir(full_path);

            while ( (entry = readdir(inner_dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue; }
                    
                snprintf(inner_path, sizeof(inner_path), "%s/%s", full_path, entry->d_name);

                if (stat(inner_path, &file_stat) != 0) {
                    continue; }

                if (S_ISDIR(file_stat.st_mode)) { // Inner directory
                    GET_TESTS_ERR("Test folder (%s) contained 2nd nested directory (%s)\n", path, entry->d_name);
                } else {
                    if (num_files == capacity) {
                        GET_TESTS_ERR("More files than folder storage capacity (%zu)", capacity); }

                    read_test_file(inner_path, num_files, expect_fail_ids, filename_storage, file_storage);
                    num_files++;
                }
            }

            closedir(inner_dir);

        } else {
            if (num_files == capacity) {
                GET_TESTS_ERR("More files than folder storage capacity (%zu)", capacity); }

            read_test_file(full_path, num_files, expect_fail_ids, filename_storage, file_storage);
            num_files++;
        }
        
    }
    
    closedir(dir);

    return (folder_storage){
        .is_error        = false,
        .expect_fail_ids = expect_fail_ids,
        .capacity        = capacity,
        .num_files       = num_files,
        .filenames       = filename_storage,
        .file_outputs    = file_storage
    };
}


