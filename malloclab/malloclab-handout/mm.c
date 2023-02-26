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
    "bovik@cs.cmu.edu",
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

// 在地址 p 读写一个字
#define GET(P)      (*(unsigned int *)(p)) 
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// 从地址 p 读取大小和已分配的字段
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define FET_ALLOC(p) (GET(p) & 0x1)

// 给定有效载荷指针, 找到头部和脚部
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 给定有效载荷指针, 找到前一块或下一块
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// 总是指向序言块的第二块
static char *heap_list;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 申请四个字节空间
    if ((heap_list = men_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_list, 0);                            // 对齐填充
    PUT(heap_list + (1 * WSIZE), PACK(DSIZE, 1)); // 序言块头部
    PUT(heap_list + (2 * WSIZE), PACK(DSIZE, 1)); // 序言块脚部
    PUT(heap_list + (3 * WSIZE), PACK(0, 1));     // 结尾块
    heap_list += (2 * WSIZE);     

    // 在空堆外部放置一个 CHUNKSIZE 字节的空闲块
    if (extern_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;                
    
    return 0;
}

// 扩展heap, 传入的是字节数
static void *extern_heap(size_t words)
{
    // bp总是指向有效载荷
    char *bp;
    size_t size;

   // 分配偶数个字，以保持对齐
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // 根据传入字节数奇偶, 考虑对齐
    if ((long)(bp = men_sbrk(size)) == -1) // 分配
        return NULL;

    // 
    PUT(HDRP(bp), PACK(size, 0)); // 空闲块头
    PUT(FTRP(bp), PACK(size, 0)); // 空闲块脚
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 片的新结尾块

    // 判断相邻块是否是空闲块, 进行合并
    return coalesce(bp);
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
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extern_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE((HDRP(bp)));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp), PACK(size, 0)));
        PUT(FTRP(NEXT_BLKP(bp), PACK(size, 0)));
        bp = PREV_BLKP(bp);
    }

    return bp;
} 

static void *find_fit(size_t asize)
{

}

static void place(void *bp, size_t asize)
{
    
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














