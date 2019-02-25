#ifndef __AILUA_ATOMIC_H__
#define __AILUA_ATOMIC_H__

#define ATOM_CAS(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define ATOM_FAND(ptr,n) __sync_fetch_and_and(ptr,n)
#define ATOM_FOR(ptr,n) __sync_fetch_and_or(ptr,n)
#define ATOM_FXOR(ptr,n) __sync_fetch_and_xor(ptr,n)

#define ATOM_INC(ptr) __sync_add_and_fetch(ptr, 1)
#define ATOM_FINC(ptr) __sync_fetch_and_add(ptr, 1)
#define ATOM_DEC(ptr) __sync_sub_and_fetch(ptr, 1)
#define ATOM_FDEC(ptr) __sync_fetch_and_sub(ptr, 1)
#endif
