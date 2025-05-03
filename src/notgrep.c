#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define FSUTIL_IMPLEMENTATION
#include "fsutil.h"

#ifdef _WIN32
#include "windows_dirent.h"
#else
#include "dirent.h"
#endif

///////////////////////////////////////////
///
/// Utilities
///

#define TRACE(msg) printf("%s:%d:%s(): %s\n", __FILE__, __LINE__, __func__, msg)

typedef struct StringView {
    const char *data;
    size_t count;
} StringView;
#define SV(cstr_literal) (StringView){ .data = (cstr_literal), .count = sizeof(cstr_literal) - 1, }
#define SV_NULL (StringView){0}
#define SV_FMT "%.*s"
#define SV_ARGV(sv) (int)(sv).count, (sv).data

StringView sv_from_cstr(const char *cstr)
{
    StringView result;
    result.count = 0;
    for(; cstr[result.count] != 0; ++result.count);
    result.data = cstr;
    return result;
}

typedef struct Args {
    int count;
    const char **items;
} Args;

void usage(const char *program)
{
    fprintf(stderr, "USAGE: %s <PATTERN> <DIR?>\n", program);
    fprintf(stderr, "# %s\n", program);
    fprintf(stderr, "   %s is a grep like tools for searching text in a directory.\n", program);
    fprintf(stderr, "   %s by default will search in the directory recursively.\n", program);
    fprintf(stderr, "   %s doesn't ignore anything.\n", program);
    fprintf(stderr, "## Positional Argument\n");
    fprintf(stderr, "   <PATTERN> Pattern to be searched\n");
    fprintf(stderr, "   <DIR?> A directory which files will be searched. This could be empty which means, %s will look in current dir\n", program);
}

StringView shift_args(Args *args, const char *on_error_message)
{
    assert(args && "Invalid args pointer");
    assert(args && "Invalid on_error_message pointer");
    if(args->count == 0) {
        fprintf(stderr, "ERROR: %s\n", on_error_message);
        usage("grepper");
        exit(EXIT_FAILURE);
    }
    const char *result = args->items[0];
    args->items += 1;
    args->count -= 1;
    return sv_from_cstr(result);
}

int find_with_glob(StringView pattern, StringView text, size_t encounter_index)
{
    size_t j = 0;
    size_t start_index = 0;
    for(size_t i = 0; i < text.count; ++i) {
        switch(pattern.data[j]) {
            case '?':
                {
                    if(j == 0) start_index = i;
                    j += 1;
                } break;
            case '*':
                {

                } break;
            case '[':
                {
                } break;
            case '(':
                {
                } break;
            default:
                {
                    if(j == 0) start_index = i;
                    j = pattern.data[j] == text.data[i] ? j + 1 : 0;
                } break;
        }
        if(j == pattern.count) {
            if(encounter_index == 0) {
                return start_index;
            }
            encounter_index -= 1;
        }
    }
    return -1;
}

///////////////////////////////////////////
///
/// Grep Logics
///

typedef struct SearchResult {
    StringView filepath;
    int row;
    int col;
    StringView preview;
} SearchResult;

// TODO(bagasjs): We need another way of storing result since it would be so slow if we have to copy the data when appending new result
typedef struct SearchContext {
    StringView pattern;
    Arena in_life;
    Arena in_file;
    Arena in_dir;
    uint32_t find_count;

    char *readbuf;
    uint32_t readbufsz;
    struct {
        SearchResult *items;
        size_t count;
        size_t capacity;
    } results;
} SearchContext;

void sc_init(SearchContext *sc, StringView pattern)
{
    assert(sc && "Invalid sc pointer");
    sc->in_file = (Arena){0};
    sc->in_dir = (Arena){0};
    sc->in_life = (Arena){0};
    sc->results.count = 0;
    sc->results.capacity = 0;
    sc->pattern = pattern;
    sc->find_count = 0;
}

void sc_destroy(SearchContext *sc)
{
    assert(sc && "Invalid sc pointer");
    arena_free(&sc->in_life);
    arena_free(&sc->in_dir);
    arena_free(&sc->in_file);
}

void sc_append(SearchContext *sc, SearchResult res)
{
    assert(sc && "Invalid sc pointer");
    if(sc->results.count >= sc->results.capacity) {
        sc->results.capacity = sc->results.capacity == 0 ? 1024 : sc->results.capacity*2;
        SearchResult *new_items = arena_alloc(&sc->in_life, sc->results.capacity*sizeof(SearchResult));
        memcpy(new_items, sc->results.items, sc->results.count*sizeof(SearchResult));
        sc->results.items = new_items;
    }
    sc->results.items[sc->results.count++] = res;
}

// TODO(bagasjs): Regex searching
void search_in_line(SearchContext *sc, StringView line, StringView filepath, size_t row)
{
    assert(sc && "Invalid sc pointer");
    StringView pattern = sc->pattern;
    size_t found = 0;
    for(size_t i = 0; i < line.count; ++i) {
        if(line.data[i] == pattern.data[found]) {
            ++found;
            if(pattern.count == found) {
                sc->find_count += 1;
                sc_append(sc, (SearchResult){
                    .row = row,
                    .col = 0, // TODO(bagasjs): Get the column number
                    .filepath = filepath,
                    .preview = (StringView){ 
                        .count = line.count, .data = arena_bufdup(&sc->in_life, line.data, line.count) 
                    },
                });
            }
        } else {
            if(found > 0) found = 0;
        }
    }
}

// Search in file 1st version
// Allocate a buffer that resizable then read line by line then search in that line.
// This won't work if the file is something like Javascript Bundled source
void search_in_file1(SearchContext *sc, StringView filepath)
{
    assert(sc && "Invalid sc pointer");

    const char *filepath_cstr = arena_bufdup(&sc->in_file, filepath.data, filepath.count);
    FILE *fp = fopen(filepath_cstr, "r");
    assert(fp != NULL);
    size_t fsz = 0;
    fseek(fp, 0L, SEEK_END);
    fsz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    int ch;
    uint32_t row = 0;
    uint32_t cur = 0;
    while((ch = fgetc(fp)) != EOF) {
        if(cur == sc->readbufsz) {
            arena_reset(&sc->in_file);
            return;
        }
        if(ch == '\n') {
            sc->readbuf[cur] = 0;
            search_in_line(sc, sv_from_cstr(sc->readbuf), filepath, row);
            row += 1;
            cur = 0;
        } else {
            // TODO (bagasjs): Handle if the line is bigger than the buf
            sc->readbuf[cur] = ch;
            cur++;
        }
    }

    arena_reset(&sc->in_file);
}

// Search in file 2nd version
// Allocate an exact sized buffer then read the file data with that buffer size. But we need somehow a way
// to continue the check if we reached end of the file but we still don't checked all the way to the pattern
// Maybe it would be cool if we can search for byte files like maybe an input for a hex bytes like grep 0xFFFFFF
void search_in_file2(SearchContext *sc, StringView filepath)
{
    assert(sc && "Invalid sc pointer");
    assert(0 && "Unimplemented");
}

const char *arena_path_join(Arena *a, const char *path_a, const char *path_b)
{
    int res = path_join(NULL, 0, path_a, path_b);
    assert(res > 0 && "Failed to join path");
    char *joined_path = arena_alloc(a, sizeof(char)*res);
    assert(path_join(joined_path, res, path_a, path_b) == 0);
    return joined_path;
}

// TODO(bagasjs): Evaluating .gitignore content if it exists in the directory
void inner_search_in_dir(SearchContext *sc, StringView dirpath, int depth)
{
    assert(sc && "Invalid sc pointer");
    struct dirent *ep = NULL;

    DIR *dp = opendir(dirpath.data);
    while((ep=readdir(dp)) != NULL) {
        bool is_cwd_or_parent = strncmp(ep->d_name, ".", sizeof(ep->d_name)) == 0 
            || strncmp(ep->d_name, "..", sizeof(ep->d_name)) == 0;
        if(is_cwd_or_parent) continue;
        const char *target = arena_path_join(&sc->in_dir, dirpath.data, ep->d_name);
        if(path_isdir(target)) {
            inner_search_in_dir(sc, sv_from_cstr(target), depth + 1);
        } else {
            search_in_file1(sc, sv_from_cstr(target));
        }
    }
    closedir(dp);
    if(depth == 0) arena_reset(&sc->in_dir);
}

void search_in_dir(SearchContext *sc, StringView dirpath)
{
    assert(sc && "Invalid sc pointer");
    return inner_search_in_dir(sc, dirpath, 0);
}

void show_result(SearchResult res)
{
    printf(SV_FMT":%u:%u:"SV_FMT"\n", SV_ARGV(res.filepath), res.row, res.col, SV_ARGV(res.preview));
}

int main(int argc, const char **argv)
{
    SearchContext sc;
    char buf[1024];
    StringView pattern = SV_NULL;
    StringView dir = SV_NULL;

    Args args;
    args.count = argc;
    args.items = argv;
    shift_args(&args, "Unreachable");

    pattern = shift_args(&args, "Provide the pattern that should be searched");


    sc_init(&sc, pattern);
    sc.readbuf = buf;
    sc.readbufsz = 1024;

    if(args.count > 0) {
        dir = shift_args(&args, "Unreachable");
    } else {
        int res = path_getcwd(NULL, 0);
        assert(res >= 0);
        char *dir1 = arena_alloc(&sc.in_life, sizeof(char)*res);
        assert(path_getcwd(dir1, res) >= 0);

        // This will safe since we allocate it for the life time of search context
        dir = sv_from_cstr(dir1);
    }

    search_in_dir(&sc, dir);

    if(sc.results.count == 0) {
        printf("Nothing found!\n");
        return 0;
    }

    for(size_t i = 0; i < sc.results.count; ++i) {
        show_result(sc.results.items[i]);
    }
    sc_destroy(&sc);
    return 0;
}
