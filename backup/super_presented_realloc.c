void *mm_realloc(void *bp, size_t size)
{
    size_t asize = (size <= DSIZE) ? QSIZE : ALIGN(size + DSIZE);
    size_t old_size = GET_SIZE(HDRP(bp));
    if (asize <= old_size - DSIZE)
        return bp;

    void *next_ptr = NEXT_BLKP(bp);
    size_t next_size = GET_SIZE(HDRP(next_ptr));
    size_t combined_size = old_size + next_size;
    if (!GET_ALLOC(HDRP(next_ptr)) && combined_size >= asize)
    {
        // Case #2: next block이 free block인 경우,
        // 현재 block의 size + next block의 size가 요청 size보다 큰 경우
        remove_block(next_ptr, calc_idx(next_size));
        PUT(HDRP(bp), PACK(combined_size, 1));
        PUT(FTRP(bp), PACK(combined_size, 1));
        return bp;
    }

    void *prev_ptr = NEXT_BLKP(bp);
    size_t prev_size = GET_SIZE(HDRP(prev_ptr));
    combined_size = old_size + prev_size;
    if (!GET_ALLOC(HDRP(prev_ptr)) && combined_size >= asize)
    {
        // Case #2-2: prev block이 free block인 경우,
        // 현재 block의 size + next block의 size가 요청 size보다 큰 경우
        remove_block(prev_ptr, calc_idx(prev_size));
        bp = prev_ptr;
        memmove(bp, NEXT_BLKP(bp), asize);
        PUT(HDRP(bp), PACK(combined_size, 1));
        PUT(FTRP(bp), PACK(combined_size, 1));
        return bp;
    }

    // Case #3: 새로운 주소를 반환
    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL)
        return NULL;
    // 기존 데이터 복사
    size_t copy_size = old_size - DSIZE;
    memcpy(new_ptr, bp, copy_size);
    mm_free(bp);
    return new_ptr;
}