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
        return bp;

    // 커지기
    else
    {
        size_t div_size = 2048;
        size_t oldsize = cursize;
        void *oldbp = bp;

        // case 2
        if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))))
        {
            size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
            size_t total_size = cursize + next_size;
            if (total_size >= asize)
            {
                remove_block(NEXT_BLKP(bp), calc_idx(next_size));
                PUT(HDRP(bp), PACK(total_size, 1));
                PUT(FTRP(bp), PACK(total_size, 1));
                // 합병했더니 너무 크면 쪼개기
                if (total_size-asize >= div_size)
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
        // 힙의 끝영역이면 (2번의 특수케이스)
        else if (GET_SIZE(HDRP(NEXT_BLKP(bp))) == 0)
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

        // case 3
        else if (!GET_ALLOC(FTRP(PREV_BLKP(bp))))
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
                if (total_size-asize >= div_size)
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

        // 확장 실패 추가 할당
        bp = mm_malloc(size);
        if (bp == NULL)
            return NULL;
        memcpy(bp, oldbp, oldsize - DSIZE);
        mm_free(oldbp);
        return bp;
    }
}