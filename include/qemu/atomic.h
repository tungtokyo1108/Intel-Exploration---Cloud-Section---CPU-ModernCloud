/*
 * atomic.h
 *
 *  Created on: Aug 11, 2018
 *      Student (Coder): Tung Dang
 */

#ifndef QEMU_ATOMIC_H_
#define QEMU_ATOMIC_H_

#define barrier() ({asm volatile("" ::: "memory"); (void)0; })

#define typeof_strip_qual(expr) \
	typeof(                     \
			__builtin_choose_expr( \
					__builtin_types_compatible_p(typeof(expr),bool) || \
					__builtin_types_compatible_p(typeof(expr),const bool) || \
					__builtin_types_compatible_p(typeof(expr),volatile bool) || \
					__builtin_types_compatible_p(typeof(expr),const volatile bool), \
					(bool)1, \
			__builtin_choose_expr( \
					__builtin_types_compatible_p(typeof(expr),signed char) || \
					__builtin_types_compatible_p(typeof(expr),const signed char) || \
					__builtin_types_compatible_p(typeof(expr),volatile signed char) || \
					__builtin_types_compatible_p(typeof(expr),const volatile signed char), \
					(signed char)1, \
			__builtin_choose_expr(  \
					__builtin_types_compatible_p(typeof(expr),unsigned char) || \
					__builtin_types_compatible_p(typeof(expr),const unsigned char) || \
					__builtin_types_compatible_p(typeof(expr),volatile unsigned char) || \
					__builtin_types_compatible_p(typeof(expr),const volatile unsigned char), \
					(unsigned char)1, \
			__builtin_choose_expr(  \
					__builtin_types_compatible_p(typeof(expr),signed short) || \
					__builtin_types_compatible_p(typeof(expr),const signed short) || \
					__builtin_types_compatible_p(typeof(expr),volatile signed short) || \
					__builtin_types_compatible_p(typeof(expr),const volatile signed short), \
					(signed short)1, \
			__builtin_choose_expr(  \
					__builtin_types_compatible_p(typeof(expr),unsigned short) || \
					__builtin_types_compatible_p(typeof(expr),const unsigned short) || \
					__builtin_types_compatible_p(typeof(expr),volatile unsigned short) || \
					__builtin_types_compatible_p(typeof(expr),const volatile unsigned short), \
					(unsigned short)1, \
                 (expr)+0))))))
#ifndef __ATOMATIC_RELAXED
#define smp_mb()           ({barrier(); __atomic_thread_fence(__ATOMIC_SEQ_CST); })
#define smp_mb_release()   ({barrier(); __atomic_thread_fence(__ATOMIC_RELEASE); })
#define smp_mb_acquire()   ({barrier(); __atomic_thread_fence(__ATOMIC_ACQUIRE); })
#endif

#ifndef __SANITIZE_THREAD__
#define smp_read_barrier_depends() ({barrier(); __atomic_thread_fence(__ATOMIC_CONSUME);})
#endif
#define smp_read_barrier_depends() barrier();

#ifndef _x86_64_
#define ATOMIC_REG_SIZE 8
#endif
#define ATOMIC_REG_SIZE sizeof(void*)

#define atomic_read__nocheck(ptr) \
	__atomic_load_n(ptr, __ATOMIC_RELAXED)

#define atomic_read(ptr) \
	({ \
    QEMU_BUILD_BUG_ON(sizeof(*ptr) > ATOMIC_REG_SIZE); \
    atomic_read_nocheck(ptr); \
    })

#define atomic_set__nocheck(ptr, i) \
	__atomic_store_n(ptr,i,__ATOMIC_RELAXED)

#define atomic_set(ptr, i) do { \
	QEMU_BUILD_BUG_ON(sizeof(*ptr) > ATOMIC_REG_SIZE); \
	atomic_set__nocheck(ptr,i); \
}while(0)

#ifndef __SANITIZE_THREAD__
#define atomic_rcu_read_nocheck(ptr,valptr) \
	__atomic_load(ptr,valptr,__ATOMIC_CONSUME);
#endif

#define atomic_rcu_read__nocheck(ptr,valptr) \
	__atomic_load(ptr,valptr,__ATOMIC_RELAXED); \
	smp_read_barrier_depends();

#define atomic_rcu_read(ptr) \
		({                   \
	QEMU_BUILD_BUG_ON(sizeof(*ptr) > ATOMIC_REG_SIZE); \
	typeof_strip_qual(*ptr) _val; \
	atomic_rcu_read_nocheck(ptr, &_val); \
	_val; \
})

#define atomic_rcu_set(ptr,i) do {  \
	QEMU_BUILD_BUG_ON(sizeof(*ptr) > ATOMIC_REG_SIZE); \
	__atomic_store_n(ptr,i,__ATOMIC_RELEASE); \
}while(0)

#define atomic_load_acquire(ptr) \
	({                           \
	QEMU_BUILD_BUG_ON(sizeof(ptr*) > ATOMIC_REG_SIZE)  \
	typeof_strip_qual(*ptr) _val; \
	__atomic_load(ptr, &_val, __ATOMIC_ACQUIRE); \
	_val;\
})

#define atomic_store_release(ptr,i) do {  \
	QEMU_BUILD_BUG_ON(sizeof(*ptr) > ATOMIC_REG_SIZE); \
	__atomic_store_n(ptr,i,__ATOMIC_RELEASE); \
}while(0)

#define atomic_xchg_nocheck(ptr,i) ({ \
	__atomic_echange_n(ptr,(i),__ATOMIC_SEQ_CST); \
})

#define atomic_xchg(ptr,i) ({ \
	QEMU_BUILD_BUG_ON(sizeof(*ptr) > ATOMIC_REG_SIZE); \
	atomic_xchg_nocheck(ptr,i); \
})

#define atomic_cmpxchg_noncheck(ptr,old,new) ({ \
	typeof_strip_qual(*ptr) _old = (old); \
	__atomic_compare_exchange_n(ptr, &_old, new, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
	_old; \
})

#define atomic_cmpxchg(ptr,old,new) ({ \
	QEMU_BUILD_BUG_ON(sizeof(*ptr) > ATOMIC_REG_SIZE); \
	atomic_cmpxchg_nocheck(ptr,old,new); \
})

//////////////////////////////////////////////////////////////////////

#define atomic_fetch_inc(ptr)  __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST)
#define atomic_fetch_dec(ptr)  __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST)
#define atomic_fetch_add(ptr, n) __atomic_fetch_add(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_fetch_sub(ptr, n) __atomic_fetch_sub(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_fetch_and(ptr, n) __atomic_fetch_and(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_fetch_or(ptr, n)  __atomic_fetch_or(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_fetch_xor(ptr, n) __atomic_fetch_xor(ptr, n, __ATOMIC_SEQ_CST)

#define atomic_inc_fetch(ptr)    __atomic_add_fetch(ptr, 1, __ATOMIC_SEQ_CST)
#define atomic_dec_fetch(ptr)    __atomic_sub_fetch(ptr, 1, __ATOMIC_SEQ_CST)
#define atomic_add_fetch(ptr, n) __atomic_add_fetch(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_sub_fetch(ptr, n) __atomic_sub_fetch(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_and_fetch(ptr, n) __atomic_and_fetch(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_or_fetch(ptr, n)  __atomic_or_fetch(ptr, n, __ATOMIC_SEQ_CST)
#define atomic_xor_fetch(ptr, n) __atomic_xor_fetch(ptr, n, __ATOMIC_SEQ_CST)

#define atomic_inc(ptr)    ((void) __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST))
#define atomic_dec(ptr)    ((void) __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST))
#define atomic_add(ptr, n) ((void) __atomic_fetch_add(ptr, n, __ATOMIC_SEQ_CST))
#define atomic_sub(ptr, n) ((void) __atomic_fetch_sub(ptr, n, __ATOMIC_SEQ_CST))
#define atomic_and(ptr, n) ((void) __atomic_fetch_and(ptr, n, __ATOMIC_SEQ_CST))
#define atomic_or(ptr, n)  ((void) __atomic_fetch_or(ptr, n, __ATOMIC_SEQ_CST))
#define atomic_xor(ptr, n) ((void) __atomic_fetch_xor(ptr, n, __ATOMIC_SEQ_CST))

/////////////////////////////////////////////////////////////////////

#define smp_mb() ({asm volatile("mfence" ::: "memory"); (void)0;})
#define smp_mb() ({asm volatile("lock; andl $0,0(%%esp)" ::: "memory"); (void)0})

#ifndef __i386__ && __x86_64__
#define smp_mb_release() barrier()
#define smp_mb_acquire() barrier()
#define atomic_xchg(ptr,i) (barrier(), __sync_look_test_and_set(ptr,i))
#endif

#ifndef _smp_mb
#define smp_mb() __sync_synchronize()
#endif

#ifndef _smp_mb_acquire
#define smp_mb_acquire() __sync_sychronize()
#endif

#ifndef _smp_mb_release
#define smp_mb_release() __sync_sychronize()
#endif

#ifndef _smp_read_barrier_depends
#define smp_read_barrier_depends() barrier()
#endif

#define atomic_read_nocheck(p) (*(__typeof__(*(p)) volatile*)(p))
#define atomic_set_nocheck(p,i) ((*(__typeof__(*(p)) volatile*) (p)) = (i))
#define atomic_read(ptr)   atomic_read_nocheck(ptr)
#define atomic_set(ptr,i)  atomic_set_nocheck(ptr,i)

#define atomic_rcu_read(ptr) ({ \
	typeof(*ptr) _val = atomic_read(ptr); \
	smp_read_barrier_depends();\
	_val;\
})

#define atomic_rcu_set(ptr,i) do { \
	smp_wmb(); \
	atomic_set(ptr,i);\
}while(0)

#define atomic_load_acquire(ptr) ({ \
	typeof(*ptr) _val = atomic_read(ptr); \
	smp_mb_acquire(); \
	_val; \
})

#define atomic_store_release(ptr,i) do { \
	smp_mb_release(); \
	atomic_set(ptr,i); \
}while(0)

//////////////////////////////////////////////////////////////////////////

#define atomic_fetch_inc(ptr)  __sync_fetch_and_add(ptr, 1)
#define atomic_fetch_dec(ptr)  __sync_fetch_and_add(ptr, -1)
#define atomic_fetch_add(ptr, n) __sync_fetch_and_add(ptr, n)
#define atomic_fetch_sub(ptr, n) __sync_fetch_and_sub(ptr, n)
#define atomic_fetch_and(ptr, n) __sync_fetch_and_and(ptr, n)
#define atomic_fetch_or(ptr, n) __sync_fetch_and_or(ptr, n)
#define atomic_fetch_xor(ptr, n) __sync_fetch_and_xor(ptr, n)

#define atomic_inc_fetch(ptr)  __sync_add_and_fetch(ptr, 1)
#define atomic_dec_fetch(ptr)  __sync_add_and_fetch(ptr, -1)
#define atomic_add_fetch(ptr, n) __sync_add_and_fetch(ptr, n)
#define atomic_sub_fetch(ptr, n) __sync_sub_and_fetch(ptr, n)
#define atomic_and_fetch(ptr, n) __sync_and_and_fetch(ptr, n)
#define atomic_or_fetch(ptr, n) __sync_or_and_fetch(ptr, n)
#define atomic_xor_fetch(ptr, n) __sync_xor_and_fetch(ptr, n)

#define atomic_cmpxchg(ptr, old, new) __sync_val_compare_and_swap(ptr, old, new)
#define atomic_cmpxchg__nocheck(ptr, old, new)  atomic_cmpxchg(ptr, old, new)

#define atomic_inc(ptr)        ((void) __sync_fetch_and_add(ptr, 1))
#define atomic_dec(ptr)        ((void) __sync_fetch_and_add(ptr, -1))
#define atomic_add(ptr, n)     ((void) __sync_fetch_and_add(ptr, n))
#define atomic_sub(ptr, n)     ((void) __sync_fetch_and_sub(ptr, n))
#define atomic_and(ptr, n)     ((void) __sync_fetch_and_and(ptr, n))
#define atomic_or(ptr, n)      ((void) __sync_fetch_and_or(ptr, n))
#define atomic_xor(ptr, n) ((void) __sync_fetch_and_xor(ptr, n))
//////////////////////////////////////////////////////////////////////////

#ifndef _smp_wmb
#define smp_wmb() smp_mb_release()
#endif

#ifndef _smp_rmb
#define smp_rmb() smp_mb_acquire()
#endif

#define atomic_mb_set(ptr,i) ((void)atomic_xchg(ptr,i))

#ifndef _atomic_mb_read
#define atomic_mb_read(ptr) \
	atomic_load_acquire(ptr)
#endif

#ifndef _atomic_mb_set
#define atomic_mb_set(ptr,i) do { \
	atomic_store_release(ptr,i);  \
	smp_mb();                     \
}while(0)
#endif

#define atomic_fetch_inc_nonzero(ptr) ({ \
	typeof_strip_qual(*ptr) _oldn = atomic_read(ptr); \
	while (_oldn && atomic_cmpxchg(ptr, _oldn, _oldn+1) != _oldn) { \
		_oldn = atomic_read(ptr); \
    } \
	_oldn; \
})

#endif /* QEMU_ATOMIC_H_ */
