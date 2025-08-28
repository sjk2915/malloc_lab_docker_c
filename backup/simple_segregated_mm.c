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
    "CSAPP",
    /* First member's full name */
    "Rivad",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

#define WSIZE 8
#define DSIZE 16
#define CHUNKSIZE (1<<12)
#define SEGREGATED_MAX_SIZE 20
#define SEGREGATED_SIZE(idx) (0x08 << (idx))

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned long *)(p))
#define PUT(p, val) (*(unsigned long *)(p) = (val))

#define NEXTP(bp) (*(char **)(bp))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (WSIZE - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static void *segregated_listp[SEGREGATED_MAX_SIZE];

static void *extend_heap(size_t words)
{
    void *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    return bp;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    for (int i=0; i<SEGREGATED_MAX_SIZE; i++)
        segregated_listp[i] = NULL;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0) return NULL;

    void *bp;
    size_t asize = MAX(DSIZE, size + WSIZE);

    int idx = 0;
    while (idx < SEGREGATED_MAX_SIZE - 1 && asize > SEGREGATED_SIZE(idx))
        idx++;

    // 가용 리스트에서 블록 찾기
    if ((bp = segregated_listp[idx]) != NULL)
    {
        segregated_listp[idx] = NEXTP(bp);
        PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
        return bp;
    }

    // 가용 블록이 없으면 새로운 청크를 할당하고 분할
    else
    {
        size_t block_size = SEGREGATED_SIZE(idx);
        // 큰 블록은 개별 할당
        if (block_size >= CHUNKSIZE)
        {
            if ((bp = extend_heap(block_size / WSIZE)) == NULL)
                return NULL;
            
            PUT(HDRP(bp), PACK(block_size, 1));
            return bp;
        }
        
        // 작은 블록들은 청크에서 분할
        else
        {
            if ((bp = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
                return NULL;
            
            char *current = bp;
            char *chunk_end = current + CHUNKSIZE;
            
            // 첫 번째 블록 (반환할 블록)
            char *first_block = current;
            PUT(HDRP(first_block), PACK(block_size, 1));
            current += block_size;
            
            // 나머지 블록들을 가용 리스트에 추가
            while (current + block_size <= chunk_end)
            {
                PUT(HDRP(current), PACK(block_size, 0));
                NEXTP(current) = segregated_listp[idx];
                segregated_listp[idx] = current;
                current += block_size;
            }

            return first_block;
        }
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    int idx = 0;
    while (idx < SEGREGATED_MAX_SIZE - 1 && size > SEGREGATED_SIZE(idx))
        idx++;
    // 리스트에 추가
    NEXTP(bp) = segregated_listp[idx];
    segregated_listp[idx] = bp;
}

/*
 * mm_calloc
 */
void *mm_calloc(size_t num, size_t size)
{
    // 쓸모없는 요청 무시
    if (num == 0 || size == 0 || num > 0xffffffffffffffff / size)
        return NULL;
        
    void *bp = mm_malloc(num * size);
    if (bp != NULL)
        memset(bp, 0, num * size);
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    if (bp == NULL) 
        return mm_malloc(size);
    if (size == 0)
    {
        mm_free(bp);
        return NULL; 
    }

    void *oldbp = bp;
    void *newbp = mm_malloc(size);
    if (newbp == NULL)
        return NULL;
    size_t copySize = GET_SIZE(HDRP(oldbp)) - WSIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newbp, oldbp, copySize);
    mm_free(oldbp);
    return newbp;
}