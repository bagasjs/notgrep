/*

   `btk_btkfstil.h` - A single header multiplatform library (Linux and Windows) filesystem utilities with no memory allocation


    Guide:
    - Link with -lShlwapi

*/
#ifndef BTK_FSUTIL_H_
#define BTK_FSUTIL_H_

#include <stddef.h>

#ifdef _WIN32
#define BTKFS_PATHSEP '\\'
#define BTKFS_PATHSEP_CSTR "\\"
#else
#define BTKFS_PATHSEP '/'
#define BTKFS_PATHSEP_CSTR "/"
#endif

#define BTKFS_FALSE 0
#define BTKFS_TRUE 1
typedef char btkfs_bool;

/**
 * This function gives you the current working directory
 *
 * This function returns int which
 * btkfs_getcwd(...) <  0 if it's an error
 * btkfs_getcwd(...) >= 0 if it's success
 */
int btkfs_getcwd(char *dstbuf, size_t dstbufsz);

/**
 * This function join paths based on the OS separator
 *
 * This function returns int which
 * btkfs_path_join(...) <  1 if it's an error
 * btkfs_path_join(...) == 0 if it's success
 * btkfs_path_join(...) >  0 if given dstbuf's size is not enough
 */
int btkfs_path_join(char *dstbuf, size_t dstbufsz, const char *path_a, const char *path_b);

/**
 * This function gives you the extension of a from a path
 * i.e. /home/user/script.sh -> .sh
 * 
 * This function returns int which
 * btkfs_getext(...) <  1 if it's an error
 * btkfs_getext(...) == 0 if given path doesn't have any extension
 * btkfs_getext(...) >  0 if it's success or given dstbuf's size is not enough
 */
int btkfs_getext(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function gives you the basename of a file
 * i.e. /home/user/script.sh -> script.sh
 *
 * btkfs_getbasename(...) <  1 if it's an error
 * btkfs_getbasename(...) >= 0 if it's success or given dstbuf's size is not enough
 */
int btkfs_getbasename(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function gives you the absolute path of a path
 * i.e. src/main.c -> /home/user/workspace/project/src
 * 
 * btkfs_getabspath(...) <  1 if it's an error
 * btkfs_getabspath(...) >  1 if the dstbuf's size is not enough
 * btkfs_getabspath(...) == 0 if it's successful
 */
int btkfs_getabspath(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function check if a path is an absolute path
 * i.e. 
 * - src/main.c -> 0
 * - /home/user/workspace/project/src -> 1
 */
btkfs_bool btkfs_isabspath(const char *path);

// TODO
int btkfs_getdirname(char *dstbuf, size_t dstbufsz, const char *path);

/**
 * This function check if a given path is a directory
 */
btkfs_bool btkfs_isdir(const char *path);

/**
 * This function check if a given path is exists
 */
btkfs_bool btkfs_exists(const char *path);

/**
 * This function gets you the size of a file
 */
size_t btkfs_get_file_size(const char *filepath);


/**
 * Read the entire entries of a directory and put it into a big 
 * chunk of char arrays containing the name of files/dirs in that
 * directory separated by a single byte with value 0.
 *
 * This function returns int which
 * btkfs_readdir(...) <  1 if it's an error
 * btkfs_readdir(...) == 0 if it's success
 * btkfs_readdir(...) >  0 if given dstbuf's size is not enough
 */
int btkfs_readdir(char *dstbuf, size_t dstbufsz, const char *dirpath);

// TODO
int btkfs_load_file_text(const char *filepath, char *data, size_t datasz, size_t offset);
int btkfs_load_file_data(const char *filepath, void *data, size_t datasz, size_t offset);
int btkfs_save_file_text(const char *filepath, const char *data, size_t datasz);
int btkfs_save_file_data(const char *filepath, const void *data, size_t datasz);
int btkfs_remove_file(const char *filepath);

// TODO
int btkfs_create_dir(const char *path, unsigned int permission);
int btkfs_remove_dir(const char *path);

const char *btkfs_explain(int error_code);

#endif // BTK_FSUTIL_H_
