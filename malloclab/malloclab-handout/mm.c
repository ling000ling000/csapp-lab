/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ASoul",
    /* First member's full name */
    "Diana",
    /* First member's email address */
    "ASoul@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 头部脚部的大小
#define WSIZE 4

// 双字
#define DSIZE 8

// 拓展堆时的默认大小
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) (x > y ? x : y)

// 设置头部和脚部的值, 块大小+分配位
#define PACK(size, alloc) ((size) | (alloc))

// 读写指针p的位置
#define GET(p)      (*(unsigned int *)(p)) 
#define PUT(p, val) ((*(unsigned int *)(p)) = (val))

// 从指针p读取大小和已分配的字段
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// 给定有效载荷指针, 找到头部和脚部
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 给定有效载荷指针, 找到前一块或下一块
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// 给定序号，找到链表头节点位置
#define GET_HEAD(num) ((unsigned int *)(long)(GET(heap_list + WSIZE * num)))

// 给定bp，找到前驱和后驱
#define GET_PRE(bp) ((unsigned int *)(long)(GET(bp)))
#define GET_SUC(bp) ((unsigned int *)(long)(GET((unsigned int *)bp + 1)))

// 理论上来说，设置的大小类越多，时间性能要越好，因而设置 20 个大小类
#define CLASS_SIZE 20

// 总是指向序言块的第二块
static char *heap_list;

static void *extend_heap(size_t words);     //扩展堆
static void *coalesce(void *bp);            //合并空闲块
static void *find_fit(size_t asize);        //首次适配
static void place(void *bp, size_t asize);  //分割空闲块
static void delete(void *bp);               //从相应链表中删除块
static void insert(void *bp);               //在对应链表中插入块
static int search(size_t size);             //根据块大小, 找到头节点位置

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 申请四个字节空间
    if ((heap_list = mem_sbrk((4 + CLASS_SIZE) * WSIZE)) == (void *) - 1)
        return -1;

    // 初始化20个大小类头指针
    for (int i = 0; i < CLASS_SIZE; i ++ )
        PUT(heap_list + i * WSIZE, NULL);

    PUT(heap_list + CLASS_SIZE * WSIZE, 0); // 对齐填充
    PUT(heap_list + ((1 + CLASS_SIZE) * WSIZE), PACK(DSIZE, 1)); // 序言块头部
    PUT(heap_list + ((2 + CLASS_SIZE) * WSIZE), PACK(DSIZE, 1)); // 序言块脚步
    PUT(heap_list + ((3 + CLASS_SIZE) * WSIZE), PACK(0, 1)); // 结尾块

    // 在空堆外部放置一个 CHUNKSIZE 字节的空闲块
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

// 扩展heap, 传入的是字节数
static void *extend_heap(size_t words)
{
    // bp总是指向有效载荷
    char *bp;
    size_t size;

   // 分配偶数个字，以保持对齐
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // 根据传入字节数奇偶, 考虑对齐
    
    // 分配
    if ((long)(bp = mem_sbrk(size)) == -1) 
        return NULL;

    // 初始化空闲块的页眉/页脚和结尾页眉
    PUT(HDRP(bp), PACK(size, 0)); // 空闲块头
    PUT(FTRP(bp), PACK(size, 0)); // 空闲块脚
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 片的新结尾块

    // 判断相邻块是否是空闲块, 进行合并
    return coalesce(bp);
}

// 找到块大小对应的等价类的序号
int search(size_t size)
{
    int i;
    for (i = 4; i <= 22; i ++ )
        if (size <= (1 << i))
            return i - 4;

    return i - 4;
}

// 插入块，将块插到表头，涉及双向链表的插入删除
void insert(void *bp)
{
    // 块的大小
    size_t size = GET_SIZE(HDRP(bp));
    // 根据块大小找到头节点位置
    int num = search(size);
    
    // 如果是空的直接插入
    if (GET_HEAD(num) == NULL)
    {
        PUT(heap_list + WSIZE * num, bp);
        PUT(bp, NULL); // 前驱
        PUT((unsigned int *)bp + 1, NULL); // 后继
    } 
    else
    {
        PUT((unsigned int *)bp +1, GET_HEAD(num)); // bp后继放第一个节点
        PUT(GET_HEAD(num), bp); // 第一个节点的前驱放bo
        PUT(bp, NULL); // bp的前驱为空
        PUT(heap_list + WSIZE * num, bp); // 头节点放bp
    }
}

// 删除块，删除指针
void delete(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    int num = search(size);

    // 当前节点为唯一节点（前驱和后继都为null），就把头节点设为null
    if (GET_PRE(bp) == NULL && GET_SUC(bp) == NULL)
        PUT(heap_list + WSIZE * num, NULL);
    // 当前节点为最后一个节点，就把当前节点的前驱和后继都设为null
    else if (GET_PRE(bp) != NULL && GET_SUC(bp) == NULL)
        PUT(GET_PRE(bp) + 1, NULL);
    // 当前节点为第一个节点，就把头节点设为bp的后继
    else if (GET_SUC(bp) != NULL && GET_PRE(bp) == NULL)
    {
        PUT(heap_list + WSIZE * num, GET_SUC(bp));
        PUT(GET_SUC(bp), NULL);
    }
    // 处在中间的节点，当前节点的前驱 的后继 设为当前节点的后继，当前节点的后继 的前驱 设为当前节点的前驱
    else if (GET_PRE(bp) != NULL && GET_SUC(bp) != NULL)
    {
        // GET_PRE(bp) + 1是bp指向的块 的前驱 的后继 的位置
        // GET_PRE(bp + 1)是bp指向的块 的后继
        PUT(GET_PRE(bp) + 1, GET_SUC(bp)); 
        PUT(GET_SUC(bp), GET_PRE(bp));
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
    size_t asize; // 调整块大小
    size_t extendsize; // 如果没有合适的，可以扩展堆的数量
    char *bp;

    // 忽略超然的请求
    if (size == 0)
        return NULL;
     
    // 调整块的大小以包括开销和对齐要求
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    // 搜索适合的空闲块
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    //没有找到合适的空闲块, 那就拓展内存
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    if (bp == 0)
        return;

    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

// 合并空闲块
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 前一块大小
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 后一块大小
    size_t size = GET_SIZE(HDRP(bp)); // 当前块大小

    // 四种情况：前后都不空, 前不空后空, 前空后不空, 前后都空
    // 前后都不空
    if (prev_alloc && next_alloc)
    {
        insert(bp);
        return bp;
    }
    // 前不空后空
    else if (prev_alloc && !next_alloc)
    {
        delete(NEXT_BLKP(bp)); // 将后面的块从链表中删除
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 增加当前块大小
        PUT(HDRP(bp), PACK(size, 0)); // 修改头部
        PUT(FTRP(bp), PACK(size, 0)); // 根据头部大小定位尾部
    }
    // 前空后不空
    else if (!prev_alloc && next_alloc)
    {
        delete(PREV_BLKP(bp)); // 将其前面的快从链表中删除
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); // 增加当前块大小
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp); // 注意指针要变
    }
    // 都空
    else
    {
        delete(NEXT_BLKP(bp)); // 将前后两个块都从其链表中删除
        delete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 增加当前块大小
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert(bp); // 空闲块准备好后,将其插入合适位置

    return bp;
} 

// 首次适配
static void *find_fit(size_t asize)
{
    int num = search(asize);
    unsigned int *bp;

    // 如果找不到合适的块，那就搜索更大的大小类
    while (num < CLASS_SIZE)
    {
        bp = GET_HEAD(num);
        // 遍历当前的整个链表
        while (bp)
        {
            if (GET_SIZE(HDRP(bp)) >= asize)
                return (void *)bp;
            bp = GET_SUC(bp); // 用后继找下一块
        }
        num ++; // 找不到则进入下一个大小类
    }
    return NULL;
}

// 分离空闲块
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    // 判断是否能够分离空闲块
    delete(bp);
    if (csize - asize >= 2 * DSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp); // bp指向空闲块
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        insert(bp); // 加入分离出来的空闲块
    }
    else // 设置为填充
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    // void *oldptr = ptr;
    // void *newptr;
    // size_t copySize;
    
    // newptr = mm_malloc(size);
    // if (newptr == NULL)
    //   return NULL;
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    // if (size < copySize)
    //   copySize = size;
    // memcpy(newptr, oldptr, copySize);
    // mm_free(oldptr);
    // return newptr;

    // mm_realloc没有保留旧块的数据
    void *newptr;
    size_t copysize;
    
    newptr = mm_malloc(size);
    if(newptr == NULL)
        return 0;

    copysize = GET_SIZE(HDRP(ptr));

    if(size < copysize)
        copysize = size;

    memcpy(newptr, ptr, copysize);
    mm_free(ptr);

    return newptr;
}














