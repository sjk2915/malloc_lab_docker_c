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
#define QSIZE 32
#define CHUNKSIZE (1<<12)
#define SEGREGATED_MAX_SIZE 15
#define SEGREGATED_SIZE(idx) (0x10 << (idx))

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned long *)(p))
#define PUT(p, val) (*(unsigned long *)(p) = (val))

#define NEXTP(bp) (*(char **)(bp))
#define PREVP(bp) (*(char **)((char *)(bp) + WSIZE))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (WSIZE - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static size_t bucket_size[SEGREGATED_MAX_SIZE-1] = 
{
    16,
    32,
    48,
    64,
    128,
    192,
    256,
    384,
    456,
    512,
    768,
    1024,
    1536,
    2048,
};

static void *segregated_listp[SEGREGATED_MAX_SIZE];

static void insert_block_LIFO(void *bp, int idx)
{
    NEXTP(bp) = segregated_listp[idx];
    PREVP(bp) = NULL;
    if (segregated_listp[idx] != NULL)
        PREVP(segregated_listp[idx]) = bp;
    segregated_listp[idx] = bp;
}

static void remove_block(void *bp, int idx)
{
    if (PREVP(bp) == NULL)
        segregated_listp[idx] = NEXTP(bp);
    else
        NEXTP(PREVP(bp)) = NEXTP(bp);
    if (NEXTP(bp) != NULL)
        PREVP(NEXTP(bp)) = PREVP(bp);
}

static int calc_idx(size_t asize)
{
    int idx = 0;
    for (int i=0; i<SEGREGATED_MAX_SIZE-1; i++)
        if (asize > bucket_size[i])
            idx++;
    return idx;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case 1
    if (prev_alloc && next_alloc)
    {
        // 아무것도 안하기
    }

    // case 2
    else if (prev_alloc && !next_alloc)
    {
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_block(NEXT_BLKP(bp), calc_idx(next_size));
        size += next_size;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // case 3
    else if (!prev_alloc && next_alloc)
    {
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        remove_block(PREV_BLKP(bp), calc_idx(prev_size));
        size += prev_size;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // case 4
    else if (!prev_alloc && !next_alloc)
    {
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_block(PREV_BLKP(bp), calc_idx(prev_size));
        remove_block(NEXT_BLKP(bp), calc_idx(next_size));
        size += prev_size + next_size;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_block_LIFO(bp, calc_idx(size));
    return bp;
}

static void *extend_heap(size_t words)
{
    void *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    char *heap_listp;
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0UL);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));

    for (int i=0; i<SEGREGATED_MAX_SIZE; i++)
        segregated_listp[i] = NULL;

    return 0;
}

static void *first_fit(size_t asize, int idx)
{
    // 힙리스트 idx부터 순회
    for (int i=idx; i<SEGREGATED_MAX_SIZE; i++)
    {
        void *bp = segregated_listp[i];
        while (bp != NULL)
        {
            // 할당 안되었고 크기도 요구하는거보다 작거나 같으면 해당 블럭포인터 return
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
                return bp;
            bp = NEXTP(bp);
        }
    }

    // 맞는게 없음
    return NULL;
}

static void *best_fit(size_t asize, int idx)
{
    void *best_bp = NULL;
    for (int i=idx; i<SEGREGATED_MAX_SIZE; i++)
    {
        void *bp = segregated_listp[i];
        while (bp != NULL)
        {
            // 할당 안되었고 크기도 요구하는거보다 작거나 같으면
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
                // best_bp가 NULL 이거나 best_bp 보다 작으면
                if (best_bp == NULL || GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(best_bp)))
                    best_bp = bp;
            bp = NEXTP(bp);
        }
    }
    return best_bp;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    remove_block(bp, calc_idx(csize));

    // 너무 크면 쪼개기
    if ((csize - asize) >= (QSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        coalesce(bp);
    }

    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // 쓸모없는 요청 무시
    if (size == 0) return NULL;

    size_t asize;
    size_t extendsize;
    void *bp;

    asize = (size <= DSIZE) ? QSIZE : ALIGN(size + DSIZE);

    if ((bp = best_fit(asize, calc_idx(asize))) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    else
    {
        extendsize = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
            return NULL;
        place(bp, asize);
        return bp;
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
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

    size_t asize = (size <= DSIZE) ? QSIZE : ALIGN(size + DSIZE);
    size_t cursize = GET_SIZE(HDRP(bp));
    // 작아지기
    if (asize <= cursize)
    {
        // 작아지고 남는 부분이 너무 크면 쪼개기
        if ((cursize - asize) >= QSIZE)
        {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            void *next_bp = NEXT_BLKP(bp);
            PUT(HDRP(next_bp), PACK(cursize-asize, 0));
            PUT(FTRP(next_bp), PACK(cursize-asize, 0));
            coalesce(next_bp);
        }
        return bp;
    }

    // 커지기
    else
    {
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t oldsize = cursize;
        void *oldbp = bp;

        // case 1
        if (prev_alloc && next_alloc)
        {
            // 즉시 추가 할당
            bp = mm_malloc(size);
            if (bp == NULL)
                return NULL;
            memcpy(bp, oldbp, oldsize - DSIZE);
            mm_free(oldbp);
            return bp;
        }

        // case 2
        else if (prev_alloc && !next_alloc)
        {
            size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
            size_t total_size = cursize + next_size;
            if (total_size >= asize)
            {
                remove_block(NEXT_BLKP(bp), calc_idx(next_size));
                PUT(HDRP(bp), PACK(total_size, 1));
                PUT(FTRP(bp), PACK(total_size, 1));

                // 합병했더니 너무 크면 쪼개기
                if (total_size-asize >= QSIZE)
                {
                    PUT(HDRP(bp), PACK(asize, 1));
                    PUT(FTRP(bp), PACK(asize, 1));
                    void *next_bp = NEXT_BLKP(bp);
                    PUT(HDRP(next_bp), PACK(total_size-asize, 0));
                    PUT(FTRP(next_bp), PACK(total_size-asize, 0));
                    coalesce(next_bp);
                }
                return bp;
            }
        }

        // case 3
        else if (!prev_alloc && next_alloc)
        {
            size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
            size_t total_size = cursize + prev_size;
            if (total_size >= asize)
            {
                remove_block(PREV_BLKP(bp), calc_idx(prev_size));
                PUT(HDRP(PREV_BLKP(bp)), PACK(total_size, 1));
                PUT(FTRP(bp), PACK(total_size, 1));
                bp = PREV_BLKP(bp);

                memmove(bp, oldbp, oldsize - DSIZE);
                // 합병했더니 너무 크면 쪼개기
                if (total_size-asize >= QSIZE)
                {
                    PUT(HDRP(bp), PACK(asize, 1));
                    PUT(FTRP(bp), PACK(asize, 1));
                    void *next_bp = NEXT_BLKP(bp);
                    PUT(HDRP(next_bp), PACK(total_size-asize, 0));
                    PUT(FTRP(next_bp), PACK(total_size-asize, 0));
                    coalesce(next_bp);
                }
                return bp;
            }
        }

        // case 4
        else if (!prev_alloc && !next_alloc)
        {
            size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
            size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
            size_t total_size = cursize + prev_size + next_size;
            if (total_size >= asize)
            {
                remove_block(PREV_BLKP(bp), calc_idx(prev_size));
                remove_block(NEXT_BLKP(bp), calc_idx(next_size));
                PUT(HDRP(PREV_BLKP(bp)), PACK(total_size, 1));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(total_size, 1));
                bp = PREV_BLKP(bp);

                memmove(bp, oldbp, oldsize - DSIZE);
                // 합병했더니 너무 크면 쪼개기
                if (total_size-asize >= QSIZE)
                {
                    PUT(HDRP(bp), PACK(asize, 1));
                    PUT(FTRP(bp), PACK(asize, 1));
                    void *next_bp = NEXT_BLKP(bp);
                    PUT(HDRP(next_bp), PACK(total_size-asize, 0));
                    PUT(FTRP(next_bp), PACK(total_size-asize, 0));
                    coalesce(next_bp);
                }
                return bp;
            }
        }

        // 힙의 끝영역이면
        if (GET_SIZE(HDRP(NEXT_BLKP(bp))) == 0)
        {
            // 필요한 추가 크기 계산 및 확장
            size_t extendsize = asize - cursize;
            if ((long)(mem_sbrk(extendsize)) == -1)
                return NULL;
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
            return bp;
        }

        // 확장 실패 추가 할당
        bp = mm_malloc(size);
        if (bp == NULL)
            return NULL;
        memcpy(bp, oldbp, oldsize - DSIZE);
        mm_free(oldbp);
        return bp;
    }
}