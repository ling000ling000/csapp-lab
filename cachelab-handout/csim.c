#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> // 文件操作
#include <getopt.h> // 解析命令行选项参数

#include "cachelab.h"

void usage(void) { exit(1); }

int verbose = 0, s, E, b, S, B;

int T = 0;

typedef struct cacheLine
{
    int t; // 时刻，即是valid也是LRU的判断标志
    __uint64_t tag; // 标记位
    // 本来应该还有个block，这个lab不需要写高速缓存块
}* cacheSet; // 组就是行的数组，也就是struct cacheLine*

void init() // 初始化Cache
{
    Cache = (cacheSet *)malloc(sizeof(cacheSet) * S);
    for (int i = 0; i < S; i ++ ) // 有S组
    {
        Cache[i] = (struct cacheLine *)malloc(sizeof(struct cacheLine) * E); // 每组有E行
        for (int j = 0 ; j < E; j ++ ) Cache[i][j].t = 0; // 全部都未访问过，时间为0
    }
}

void freeCache() // 清空Cache
{
    for (int i = 0; i < S; i ++ ) free(Cache[i]);
    free(Cache);
}

FILE *opt(int argc, char* argv[]) // 读取并处理命令行选项
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

cacheSet *Cache; // 组的数组就是缓存

int main(int argc, char* argv[])
{
    FILE *fd = opt(argc, argv);
    init();
    
}
