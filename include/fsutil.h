#ifndef FSUTIL_H_
#define FSUTIL_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
#define PATH_SEP_CHAR '\\'
#define PATH_SEP_CSTR "\\"
#else
#define PATH_SEP_CHAR '/'
#define PATH_SEP_CSTR "/"
#endif

/**
 * This function gives you the current working directory
 *
 * This function returns int which
 * path_getcwd(...) <  0 if it's an error
 * path_getcwd(...) >= 0 if it's success
 */
int path_getcwd(char *dstbuf, size_t dstbufsz);

/**
 * This function join paths based on the OS separator
 *
 * This function returns int which
 * path_path_join(...) <  1 if it's an error
 * path_path_join(...) == 0 if it's success
 * path_path_join(...) >  0 if given dstbuf's size is not enough
 */
int path_join(char *dstbuf, size_t dstbufsz, const char *path_a, const char *path_b);

/**
 * This function gives you the extension of a from a path
 * i.e. /home/user/script.sh -> .sh
 * 
 * This function returns int which
 * path_getext(...) <  1 if it's an error
 * path_getext(...) == 0 if given path doesn't have any extension
 * path_getext(...) >  0 if it's success or given dstbuf's size is not enough
 */
int path_getext(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function gives you the basename of a file
 * i.e. /home/user/script.sh -> script.sh
 *
 * path_getbasename(...) <  1 if it's an error
 * path_getbasename(...) >= 0 if it's success or given dstbuf's size is not enough
 */
int path_getbasename(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function gives you the absolute path of a path
 * i.e. src/main.c -> /home/user/workspace/project/src
 * 
 * path_getabspath(...) <  1 if it's an error
 * path_getabspath(...) >  1 if the dstbuf's size is not enough
 * path_getabspath(...) == 0 if it's successful
 */
int path_getabspath(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function check if a path is an absolute path
 * i.e. 
 * - src/main.c -> 0
 * - /home/user/workspace/project/src -> 1
 */
bool path_isabspath(const char *path);

// TODO
int path_getdirname(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function check if a given path is a directory
 */
bool path_isdir(const char *path);

/**
 * This function check if a given path is exists
 */
bool path_exists(const char *path);

/**
 * This function gets you the size of a file
 */
size_t fs_get_file_size(const char *filepath);


#endif // FSUTIL_H_

#ifdef FSUTIL_IMPLEMENTATION
#ifdef _WIN32
#include <string.h>
#include <windows.h>
#include <Shlwapi.h>

enum fs_error_codes {
    FS_ERROR_NONE = 0,
    FS_ERROR_UNKNOWN = -1,
    FS_ERROR_INVALID_ARGUMENTS = -2,
    FS_ERROR_PATH_NOT_EXISTS = -3,
    FS_ERROR_PATH_ALREADY_EXISTS = -4,
    FS_ERROR_NO_MORE_PARENT_DIR = -5,
    FS_ERROR_NOT_A_FILE = -6,
    FS_ERROR_NOT_A_DIRECTORY = -7,
    FS_ERROR_COULDNT_OPEN_DIR = -8,
};

static const char *_error_code_explanation[] = {
    "None",
    "Unknown error",
    "Invalid arguments",
    "Path is not exists",
    "Path is already exists",
    "No more parent dir",
    "Not a file",
    "Not a directory",
    "Could not open directory",
};

const char *path_explain(int error_code)
{
    int index = error_code * -1;
    if(index < 0) return NULL;
    return _error_code_explanation[index];
}

int path_getcwd(char *dstbuf, size_t dstbufsz)
{
    if((dstbuf == NULL && dstbufsz > 0) || (dstbuf != NULL && dstbufsz == 0))
        return FS_ERROR_INVALID_ARGUMENTS;
    DWORD length = GetCurrentDirectory(dstbufsz, dstbuf);
    return length;
}

int path_join(char *dstbuf, size_t dstbufsz, const char *path_a, const char *path_b)
{
    if(path_a == NULL || path_b == NULL) return FS_ERROR_INVALID_ARGUMENTS;
    size_t len_a = strlen(path_a);
    size_t len_b = strlen(path_b);
    char need_separator = path_a[len_a - 1] != PATH_SEP_CHAR;
    size_t total_length = len_a + len_b + (need_separator ? 1 : 0) + 1;
    if(dstbuf == NULL || dstbufsz == 0) return (int)total_length + 1;

    // Allocating last 1 byte for null terminator
    dstbuf[dstbufsz - 1] = 0;
    dstbufsz -= 1;
    if(dstbufsz < total_length) return (int)total_length + 1;

    size_t i;
    for(i = 0; i < len_a; ++i) dstbuf[i] = path_a[i];
    if(need_separator) dstbuf[i++] = PATH_SEP_CHAR;
    for(size_t j = 0; j < len_b; ++j) dstbuf[i + j] = path_b[j];
    return 0;
}

int path_getext(char *dstbuf, size_t dstbufsz, const char *path)
{
    if(path == NULL) return FS_ERROR_INVALID_ARGUMENTS;
    size_t path_len = strlen(path);
    size_t extlen = 0;
    for (size_t i = path_len; i > 0; i--) {
        int c = path[i - 1];
        if(c == '.') {
            extlen = path_len - i + 1;
            break;
        } else if(c == PATH_SEP_CHAR) {
            return 0;
        }
    }
    if(dstbufsz < extlen) return (int)extlen+1;

    dstbuf[dstbufsz+1] = 0;
    for(size_t i = 0; i < extlen; ++i) {
        dstbuf[i] = path[path_len - extlen + i];
    }

    return extlen;
}

int path_getbasename(char *dstbuf, size_t dstbufsz, const char *path)
{
    if(path == NULL) return FS_ERROR_INVALID_ARGUMENTS;
    size_t path_len = strlen(path);
    size_t basename_len = 0;
    for (size_t i = path_len; i > 0; i--) {
        int c = path[i - 1];
        if(c == PATH_SEP_CHAR) {
            basename_len = path_len - i;
            break;
        }
    }
    if(basename_len == 0) basename_len = path_len;
    if(dstbufsz < basename_len) return (int)basename_len+1;

    dstbuf[dstbufsz - 1] = 0;
    for(size_t i = 0; i < basename_len; ++i) {
        dstbuf[i] = path[path_len - basename_len + i];
    }

    return basename_len;
}

int path_getabspath(char *dstbuf, size_t dstbufsz, const char *path)
{
    if(path == NULL) return FS_ERROR_INVALID_ARGUMENTS;
    DWORD result = GetFullPathName(path, (DWORD)dstbufsz, dstbuf, 0);
    if(result > dstbufsz) return (int)result;
    return 0;
}

bool path_exists(const char *path)
{
    DWORD attr = GetFileAttributes(path);
    if(attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return true;
}

bool path_isdir(const char *path)
{
    // TODO(bagasjs): Assertion for invalid argument path
    DWORD attr = GetFileAttributes(path);
    if(attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool path_isabspath(const char *path)
{
    if(path == NULL) return false;
    return PathIsRelative(path) ? false : true;
}

size_t fs_get_file_size(const char *filepath)
{
    // TODO(bagasjs): Assertion for invalid argument filepath
    HANDLE file_handle = CreateFileA(
        filepath,               // File name
        GENERIC_READ,          // Access mode
        FILE_SHARE_READ,      // Share mode
        NULL,                  // Security attributes
        OPEN_EXISTING,         // Creation disposition
        FILE_ATTRIBUTE_NORMAL,  // File attributes
        NULL                   // Template file
    );
    if(file_handle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    DWORD file_size = GetFileSize(file_handle, NULL);
    CloseHandle(file_handle);
    return file_size;
}

#include <stdio.h>
int path_readdir(char *dstbuf, size_t dstbufsz, const char *dirpath)
{
    if(!dirpath) return FS_ERROR_INVALID_ARGUMENTS;
    WIN32_FIND_DATA find_file_data;
    HANDLE h_find;
    h_find = FindFirstFile(dirpath, &find_file_data);
    if(h_find == INVALID_HANDLE_VALUE) return FS_ERROR_COULDNT_OPEN_DIR;

    size_t total_length = 0;
    do {
        printf("%s(%s)\n", dirpath, find_file_data.cFileName);
        if(strncmp(find_file_data.cFileName, ".", sizeof(find_file_data.cFileName)) == 0 
            && strncmp(find_file_data.cFileName, "..", sizeof(find_file_data.cFileName)) == 0) {
            continue;
        }
        total_length += strnlen(find_file_data.cFileName, sizeof(find_file_data.cFileName)) + 1;
    } while(FindNextFile(h_find, &find_file_data) != 0);

    if(GetLastError() != ERROR_NO_MORE_FILES) {
        // TODO(bagasjs): Find out what would resulting this error
        return FS_ERROR_UNKNOWN;
    }
    FindClose(h_find);
    if(dstbuf == NULL || dstbufsz == 0) {
        return total_length+1;
    }
    
    // The end of the buffer would be 2 zeros
    dstbuf[dstbufsz - 1] = 0;

    h_find = FindFirstFile(dirpath, &find_file_data);
    if(h_find == INVALID_HANDLE_VALUE) {
        return FS_ERROR_COULDNT_OPEN_DIR;
    }

    size_t offset = 0;
    do {
        if(strncmp(find_file_data.cFileName, ".", 260) == 0 
            && strncmp(find_file_data.cFileName, "..", 260) == 0) {
            continue;
        }
        size_t name_length = strnlen(find_file_data.cFileName, sizeof(find_file_data.cFileName));
        strncpy(dstbuf + offset, find_file_data.cFileName, name_length);
        offset += name_length;
        dstbuf[offset++] = 0;
    } while(FindNextFile(h_find, &find_file_data) != 0);
    FindClose(h_find);

    return 0;
}

char *fs_next_in_directory(char *direntry)
{
    size_t i;
    for(i = 0; direntry[i] != 0; ++i);
    return direntry + i + 1;
}
#endif
#endif
