#include "btk_fsutil.h"
#include <string.h>
#include <windows.h>
#include <Shlwapi.h>

enum btkfs_error_codes {
    BTKFS_ERROR_NONE = 0,
    BTKFS_ERROR_UNKNOWN = -1,
    BTKFS_ERROR_INVALID_ARGUMENTS = -2,
    BTKFS_ERROR_PATH_NOT_EXISTS = -3,
    BTKFS_ERROR_PATH_ALREADY_EXISTS = -4,
    BTKFS_ERROR_NO_MORE_PARENT_DIR = -5,
    BTKFS_ERROR_NOT_A_FILE = -6,
    BTKFS_ERROR_NOT_A_DIRECTORY = -7,
    BTKFS_ERROR_COULDNT_OPEN_DIR = -8,
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

const char *btkfs_explain(int error_code)
{
    int index = error_code * -1;
    if(index < 0) return NULL;
    return _error_code_explanation[index];
}

int btkfs_getcwd(char *dstbuf, size_t dstbufsz)
{
    if((dstbuf == NULL && dstbufsz > 0) || (dstbuf != NULL && dstbufsz == 0))
        return BTKFS_ERROR_INVALID_ARGUMENTS;
    DWORD length = GetCurrentDirectory(dstbufsz, dstbuf);
    return length;
}

int btkfs_path_join(char *dstbuf, size_t dstbufsz, const char *path_a, const char *path_b)
{
    if(path_a == NULL || path_b == NULL) return BTKFS_ERROR_INVALID_ARGUMENTS;
    size_t len_a = strlen(path_a);
    size_t len_b = strlen(path_b);
    char need_separator = path_a[len_a - 1] != BTKFS_PATHSEP;
    size_t total_length = len_a + len_b + (need_separator ? 1 : 0) + 1;
    if(dstbuf == NULL || dstbufsz == 0) return (int)total_length + 1;

    // Allocating last 1 byte for null terminator
    dstbuf[dstbufsz - 1] = 0;
    dstbufsz -= 1;
    if(dstbufsz < total_length) return (int)total_length + 1;

    size_t i;
    for(i = 0; i < len_a; ++i) dstbuf[i] = path_a[i];
    if(need_separator) dstbuf[i++] = BTKFS_PATHSEP;
    for(size_t j = 0; j < len_b; ++j) dstbuf[i + j] = path_b[j];
    return 0;
}

int btkfs_getext(char *dstbuf, size_t dstbufsz, const char *path)
{
    if(path == NULL) return BTKFS_ERROR_INVALID_ARGUMENTS;
    size_t path_len = strlen(path);
    size_t extlen = 0;
    for (size_t i = path_len; i > 0; i--) {
        int c = path[i - 1];
        if(c == '.') {
            extlen = path_len - i + 1;
            break;
        } else if(c == BTKFS_PATHSEP) {
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

int btkfs_getbasename(char *dstbuf, size_t dstbufsz, const char *path)
{
    if(path == NULL) return BTKFS_ERROR_INVALID_ARGUMENTS;
    size_t path_len = strlen(path);
    size_t basename_len = 0;
    for (size_t i = path_len; i > 0; i--) {
        int c = path[i - 1];
        if(c == BTKFS_PATHSEP) {
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

int btkfs_getabspath(char *dstbuf, size_t dstbufsz, const char *path)
{
    if(path == NULL) return BTKFS_ERROR_INVALID_ARGUMENTS;
    DWORD result = GetFullPathName(path, (DWORD)dstbufsz, dstbuf, 0);
    if(result > dstbufsz) return (int)result;
    return 0;
}

btkfs_bool btkfs_exists(const char *path)
{
    DWORD attr = GetFileAttributes(path);
    if(attr == INVALID_FILE_ATTRIBUTES) {
        return BTKFS_FALSE;
    }
    return BTKFS_TRUE;
}

btkfs_bool btkfs_isdir(const char *path)
{
    // TODO(bagasjs): Assertion for invalid argument path
    DWORD attr = GetFileAttributes(path);
    if(attr == INVALID_FILE_ATTRIBUTES) {
        return BTKFS_FALSE;
    }
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

btkfs_bool btkfs_isabspath(const char *path)
{
    if(path == NULL) return BTKFS_FALSE;
    return PathIsRelative(path) ? BTKFS_FALSE : BTKFS_TRUE;
}

size_t btkfs_get_file_size(const char *filepath)
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
int btkfs_readdir(char *dstbuf, size_t dstbufsz, const char *dirpath)
{
    if(!dirpath) return BTKFS_ERROR_INVALID_ARGUMENTS;
    WIN32_FIND_DATA find_file_data;
    HANDLE h_find;
    h_find = FindFirstFile(dirpath, &find_file_data);
    if(h_find == INVALID_HANDLE_VALUE) return BTKFS_ERROR_COULDNT_OPEN_DIR;

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
        return BTKFS_ERROR_UNKNOWN;
    }
    FindClose(h_find);
    if(dstbuf == NULL || dstbufsz == 0) {
        return total_length+1;
    }
    
    // The end of the buffer would be 2 zeros
    dstbuf[dstbufsz - 1] = 0;

    h_find = FindFirstFile(dirpath, &find_file_data);
    if(h_find == INVALID_HANDLE_VALUE) {
        return BTKFS_ERROR_COULDNT_OPEN_DIR;
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

char *btkfs_next_in_direntry(char *direntry)
{
    size_t i;
    for(i = 0; direntry[i] != 0; ++i);
    return direntry + i + 1;
}
