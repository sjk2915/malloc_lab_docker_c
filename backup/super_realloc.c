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

    // 커지면 재할당 후 memmove
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
            size_t total_size = cursize + GET_SIZE(HDRP(NEXT_BLKP(bp)));
            if (total_size >= asize)
            {
                remove_block(NEXT_BLKP(bp));
                cursize = total_size;
                PUT(HDRP(bp), PACK(cursize, 1));
                PUT(FTRP(bp), PACK(cursize, 1));

                // 합병했더니 너무 크면 쪼개기
                if (cursize-asize >= QSIZE)
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
        }

        // case 3
        else if (!prev_alloc && next_alloc)
        {
            size_t total_size = cursize + GET_SIZE(HDRP(PREV_BLKP(bp)));
            if (total_size >= asize)
            {
                remove_block(PREV_BLKP(bp));
                cursize = total_size;
                PUT(HDRP(PREV_BLKP(bp)), PACK(cursize, 1));
                PUT(FTRP(bp), PACK(cursize, 1));
                bp = PREV_BLKP(bp);

                memmove(bp, oldbp, oldsize - DSIZE);
                // 합병했더니 너무 크면 쪼개기
                if (cursize-asize >= QSIZE)
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
        }

        // case 4
        else if (!prev_alloc && !next_alloc)
        {
            size_t total_size = cursize + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
            if (total_size >= asize)
            {
                remove_block(PREV_BLKP(bp));
                remove_block(NEXT_BLKP(bp));
                cursize = total_size;
                PUT(HDRP(PREV_BLKP(bp)), PACK(cursize, 1));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(cursize, 1));
                bp = PREV_BLKP(bp);

                memmove(bp, oldbp, oldsize - DSIZE);
                // 합병했더니 너무 크면 쪼개기
                if (cursize-asize >= QSIZE)
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