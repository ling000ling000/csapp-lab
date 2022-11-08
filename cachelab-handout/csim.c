#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> // 文件操作
#include <getopt.h> // 解析命令行选项参数

#include "cachelab.h"

int verbose = 0, s, E, b, S, B;

void usage(void)
{
    exit(1);
}

FILE *opt(int argc, char* argv[])
{
    FILE *tracefile;

    for (int i = 1; i < argc; i ++ )
    {
        int op = getopt(argc, argv, "hvsEbt");
        switch(op)
        {
            case 's': // 组数的位
                s = atoi(argv[optind]);
                if (s <= 0) usage();
                S = 1 << s;
            case 'E': // 每一组的行数
                E = atoi(argv[optind]);
                if (E <= 0) usage();
                break;
            case 'b':
                b = atoi(argv[optind]);
                if (b <= 0) usage();
                B = 1 << b;
                break;
            case 't': // 文件
                tracefile = fopen(argv[optind], "r");
                if (tracefile == NULL) usage();
                break;
        }
    }
    return tracefile;
}

void init()
{

}

int main(int argc, char* argv[])
{
    FILE *fd = opt(argc, argv);
    init();
    
}
