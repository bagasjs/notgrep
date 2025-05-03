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
