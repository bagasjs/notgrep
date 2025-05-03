/*
 
    parseopt.h - A simple getopt like command-line arguments parser
 
    Example
    ```c
    #define PARSEOPT_IMPLEMENTATION
    #include "parseopt.h"
    #include <stdio.h>
    #include <stdbool.h>

    int main(int argc, char **argv)
    {
        Opt opts[] = {
            (Opt){ .name = "name", .has_arg = 1, },
            (Opt){ .name = "age",  .has_arg = 1, },
            (Opt){ .name = "help", .has_arg = 0, }
        };

        OptParser parser = {0};
        parser.opts = opts;
        parser.count_opts = sizeof(opts)/sizeof(*opts);

        char *input = NULL;
        char *name = NULL;
        char *age = NULL;
        bool help = false;

        int res = 0;
        while((res = parseopt(&parser, argc, argv))) {
            if(res == PO_NO_ARGUMENT) {
                printf("ERROR: \"%s\" requires an argument\n", po_optname(&parser));
                return -1;
            }
            if(res == PO_INVALID_OPT) {
                printf("ERROR: invalid opt %s\n", po_optname(&parser));
                return -1;
            }

            if(res == PO_OPT) {
                switch(parser.optopt) {
                    // This correspond to name
                    case 0:
                        {
                            name = parser.optarg;
                        } break;
                    // This correspond to age
                    case 1:
                        {
                            age = parser.optarg;
                        } break;
                    // This correspond to help
                    case 2:
                        {
                            help = true;
                        } break;
                }
            }

            if(res == PO_ARG) {
                if(!parser.arg) {
                    parser.arg = parser.arg;
                }

            }
        }

        if(help) {
            printf("USAGE: %s [OPTIONS]\n", argv[0]);
            printf("   --help        Get this information\n");
            printf("   --name <name> Enter your name\n");
            printf("   --age  <age>  Enter your age\n");
        } else {
            printf("name = %s, age = %s\n", name, age);
        }
    }
    ```
*/

#ifndef PARSEOPT_H_
#define PARSEOPT_H_

#include <stddef.h>

enum {
    PO_STOP = 0,
    PO_OPT,
    PO_ARG,
    PO_NO_ARGUMENT,
    PO_INVALID_OPT,
};

typedef struct Opt {
    const char *name;
    int has_arg;
} Opt;

typedef struct OptParser {
    Opt *opts;
    int count_opts;

    char *arg;
    int optind;
    int optopt;
    char *optarg;
} OptParser;

int parseopt(OptParser *parser, int argc, char **argv);
const char *po_optname(OptParser *parser);

#endif // PARSEOPT_H_

#ifdef PARSEOPT_IMPLEMENTATION

static int internal_strcmpbn(const char *a, const char *b, int bn)
{
    if(a == NULL || b == NULL) return -1;
    for(int i = 0; i < bn; ++i) {
        if(a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

static int internal_strcmp(const char *a, const char *b)
{
    if(a == NULL || b == NULL) return -1;
    for (; *a == *b && *a; a++, b++);
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static const char *internal_strchr(const char *str, int ch)
{
    if(str == NULL) return 0;
    while(*str != ch) {
        if(*str == 0) return 0;
        str += 1;
    }
    return str;
}

const char *po_optname(OptParser *parser)
{
    return parser->opts[parser->optopt].name;
}

int parseopt(OptParser *parser, int argc, char **argv)
{
    parser->optopt = -1;
    parser->optarg = 0;
    if(parser->optind >= argc) return PO_STOP;

    char *arg = argv[parser->optind];
    if(arg[0] != '-') {
        parser->optind += 1;
        return PO_ARG;
    }

    if(arg[1] == 0) return PO_OPT;
    if(arg[1] == '-') {
        if(arg[2] == 0) return PO_OPT;
        arg = &arg[2];
    } else {
        arg = &arg[1];
    }

    char *eq = internal_strchr(arg, '=');

    for(int i = 0; i < parser->count_opts; ++i) {
        Opt opt = parser->opts[i];
        int this_opt = 0;
        if(eq && internal_strcmpbn(opt.name, arg, eq - arg) == 0)
            this_opt = 1;
        if(!this_opt && internal_strcmp(opt.name, arg) == 0)
            this_opt = 1;

        if(this_opt) {
            parser->optopt = i;
            if(opt.has_arg) {
                if(eq) {
                    parser->optarg = &eq[1];
                } else {
                    if(parser->optind + 1 >= argc) {
                        parser->optopt = i;
                        return PO_NO_ARGUMENT;
                    }
                    parser->optarg = argv[parser->optind + 1];
                    parser->optind += 1;
                }
            }
            parser->optind += 1;
            return PO_OPT;
        }
    }

    parser->optopt = -1;
    parser->optind += 1;
    return PO_INVALID_OPT;
}

#endif
