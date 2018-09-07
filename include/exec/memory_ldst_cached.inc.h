/*
 * memory_ldst_cached.inc.h
 *
 *  Created on: Sep 7, 2018
 *      Student (coder): Tung Dang
 */

#ifndef EXEC_MEMORY_LDST_CACHED_INC_H_
#define EXEC_MEMORY_LDST_CACHED_INC_H_

#define ADDRESS_SPACE_LD_CACHED(size) \
	glue(glue(address_space_ld,size),glue(ENDIANNESS, _cached))
#define ADDRESS_SPACE_LD_CACHED_SLOW(size) \
	glue(glue(address_space_ld,size),glue(ENDIANNESS, _cached))
#define LD_P(size) \
	glue(glue(ld,size),glue(ENDIANNESS, _p))

static inline uint32_t ADDRESS_SPACE_LD_CACHED(l)(MemoryRegionCache *cache,
		      hwaddr addr, MemTxAttrs attrs, MemTxResult *result) {
	assert(addr < cache->len && 4 <= cache->len - addr);
	if (likely(cache->ptr))
	{
		return LD_P(l)(cache->ptr + addr);
	} else {
		return ADDRESS_SPACE_LD_CACHED_SLOW(l)(cache, addr, attrs, result);
	}
}

static inline uint64_t ADDRESS_SPACE_LD_CACHED(q)(MemoryRegionCache *cache,
		      hwaddr addr, MemTxAttrs attrs, MemTxResult *result) {
	assert(addr < cache->len && 8 <= cache->len - addr);
	if (likely(cache->ptr))
	{
		return LD_P(q)(cache->ptr + addr);
	} else {
		return ADDRESS_SPACE_LD_CACHED_SLOW(q)(cache, addr, attrs, result);
	}
}

static inline uint32_t ADDRESS_SPACE_LD_CACHED(uw)(MemoryRegionCache *cache,
		      hwaddr addr, MemTxAttrs attrs, MemTxResult *result) {
	assert(addr < cache->len && 2 <= cache->len - addr);
	if (likely(cache->ptr))
	{
		return LD_P(uw)(cache->ptr + addr);
	} else {
		return ADDRESS_SPACE_LD_CACHED_SLOW(uw)(cache,addr,attrs,result);
	}
}


#undef ADDRESS_SPACE_LD_CACHED
#undef ADDRESS_SPACE_LD_CACHED_SLOW
#undef LD_P

#define ADDRESS_SPACE_ST_CACHED(size) \
    glue(glue(address_space_st, size), glue(ENDIANNESS, _cached))
#define ADDRESS_SPACE_ST_CACHED_SLOW(size) \
    glue(glue(address_space_st, size), glue(ENDIANNESS, _cached_slow))
#define ST_P(size) \
    glue(glue(st, size), glue(ENDIANNESS, _p))

static inline void ADDRESS_SPACE_ST_CACHED(l)(MemoryRegionCache *cache,
    hwaddr addr, uint32_t val, MemTxAttrs attrs, MemTxResult *result)
{
    assert(addr < cache->len && 4 <= cache->len - addr);
    if (likely(cache->ptr)) {
        ST_P(l)(cache->ptr + addr, val);
    } else {
        ADDRESS_SPACE_ST_CACHED(l)(cache, addr, val, attrs, result);
    }
}

static inline void ADDRESS_SPACE_ST_CACHED(w)(MemoryRegionCache *cache,
    hwaddr addr, uint32_t val, MemTxAttrs attrs, MemTxResult *result)
{
    assert(addr < cache->len && 2 <= cache->len - addr);
    if (likely(cache->ptr)) {
        ST_P(w)(cache->ptr + addr, val);
    } else {
        ADDRESS_SPACE_ST_CACHED(w)(cache, addr, val, attrs, result);
    }
}

static inline void ADDRESS_SPACE_ST_CACHED(q)(MemoryRegionCache *cache,
    hwaddr addr, uint64_t val, MemTxAttrs attrs, MemTxResult *result)
{
    assert(addr < cache->len && 8 <= cache->len - addr);
    if (likely(cache->ptr)) {
        ST_P(q)(cache->ptr + addr, val);
    } else {
        ADDRESS_SPACE_ST_CACHED(q)(cache, addr, val, attrs, result);
    }
}

#undef ADDRESS_SPACE_ST_CACHED
#undef ADDRESS_SPACE_ST_CACHED_SLOW
#undef ST_P

#endif /* EXEC_MEMORY_LDST_CACHED_INC_H_ */
