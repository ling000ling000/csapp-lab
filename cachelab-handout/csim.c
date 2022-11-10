#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> // 文件操作
#include <getopt.h> // 解析命令行选项参数

#include "cachelab.h"

void usage(void) { exit(1); }

int verbose = 0, s, E, b, S, B; // opt的变量，定义了cache的大小

int T = 0; // 全局时刻表，第一个访存指令的时刻为1，之后每次数据访存都累加1

typedef struct cacheLine
{
    int valid; // 时刻，是LRU的判断标志
    __uint64_t tag; // 标记位
    // 本来应该还有个block，这个lab不需要写高速缓存块
}* cacheSet; // 组就是行的数组，也就是struct cacheLine*

cacheSet *Cache; // 组的数组就是缓存
enum Category { HIT, MISS, EVICTION };
unsigned int result[3]; // 缓存工作的结果
const char *categoryString[3] = {"hit", "miss", "eviction"};

// 初始化Cache
void init() 
{
    Cache = (cacheSet *)malloc(sizeof(cacheSet) * S);
    for (int i = 0; i < S; i ++ ) // 有S组
    {
        Cache[i] = (struct cacheLine *)malloc(sizeof(struct cacheLine) * E); // 每组有E行
        for (int j = 0 ; j < E; j ++ ) Cache[i][j].valid = 0; // 全部都未访问过，时间为0
    }
}

// 清空Cache
void freeCache() 
{
    for (int i = 0; i < S; i ++ ) free(Cache[i]);
    free(Cache);
}

void setResult(cacheSet set, enum Category category, int tag, int pos, char *resultV) // pos是行位置
{
    result[category] ++;
    set[pos].tag = tag;
    set[pos].valid = T;
    if (verbose) strcat(resultV, categoryString[category]); // 往resultV写入
}

// 缓存工作
void findCache(__uint64_t tag, int setPos, char *resultV) // tag是w地址的t位
{
    cacheSet set = Cache[setPos];
    int minPos = 0, emptyLine = -1; // ；标记空行
    for (int i = 0; i < E; i ++ ) // 遍历该set的每一行
    {
        struct cacheLine line = set[i]; // 当前set的第i行
        if (!line.valid) // valid = 0，说明该line没有用过，标记一下
        {
            emptyLine = i; // 有空行
        }
        else
        {
            if (line.tag == tag) // 行匹配
            {
                setResult(set, HIT, tag, i, resultV); // 设置HIT
                return;
            }
            if (set[minPos].valid > line.valid)
            {
                minPos = i; // 取最小时刻值
            }
        }
    }
    setResult(set, MISS, tag, emptyLine, resultV);
    if (emptyLine == -1) setResult(set, EVICTION, tag, minPos, resultV); // 要读或者写, 但是没有一个空行, 说明发生eviction
}

// 读取并处理命令行选项
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


int main(int argc, char* argv[])
{
    FILE *fd = opt(argc, argv);
    init();

    char op[2]; // 存放指令
    __uint64_t address; // w的地址
    int size; // w的字节数
    while (fscanf(fd, "%s %lx,%d\n", op, &address, &size) == 3) // fscanf()返回参数列表中被成功赋值的参数个数
    {
        if (op[0] == 'I') continue;
        int setPos = (address >> b) & ~(~0u << s); // 取出w地址中的组索引
        __uint64_t wTag = address >> (b + s); // 取出w地址中的标记

        char resultV[20]; // -v选项需要的“显示跟踪信息的可选详细标志”
        memset(resultV, 0, sizeof resultV);
        T ++;
        findCache(wTag, setPos, resultV);

        if (op[0] == 'M') findCache(wTag, setPos, resultV);

        if (verbose) fprintf(stdout, "%s %lx,%d %s\n", op, address, size, resultV);
    }

    printSummary(result[0], result[1], result[2]);

    return 0;
}
