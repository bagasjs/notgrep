#ifndef BTK_STRUTIL_H_
#define BTK_STRUTIL_H_

typedef unsigned int btksu_size_t;

typedef struct btk_stringview {
    const char *data;
    btksu_size_t count;
} btk_stringview_t;
#define BTK_SV(cstr_literal) (btk_stringview_t){ .data = (cstr_literal), .count = sizeof(cstr_literal) - 1, }
#define BTK_SV_NULL (btk_stringview_t){0}
#define BTK_SV_FMT "%.*s"
#define BTK_SV_ARGV(sv) (int)(sv).count, (sv).data

btk_stringview_t btk_sv_from_cstr(const char *);

#endif // BTK_STRUTIL_H_

#ifdef BTK_STRUTIL_IMPLEMENTATION

btk_stringview_t btk_sv_from_cstr(const char *cstr)
{
    btk_stringview_t result;
    result.count = 0;
    for(; cstr[result.count] != 0; ++result.count);
    result.data = cstr;
    return result;
}

#endif
