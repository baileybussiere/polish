#ifndef _DARRAY_H
#define _DARRAY_H
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

struct darr {
	size_t cap;
	size_t len;
	char data[];
};


#define darr_struct_t(T) struct { size_t cap; size_t len; T data[]; }

#define darr_offset(T) (offsetof(darr_struct_t(T), data))

#define darr_alloc(T, l) ({ \
	darr_struct_t(T) *d = malloc(sizeof(darr_struct_t(T)) + l*sizeof(T)); \
	d->cap = l*sizeof(T); \
	d->len = 0; \
	d->data; \
})

#define __to_darr(d) ({ \
	darr_struct_t(typeof(d[0]))* darr = (darr_struct_t(typeof(d[0]))*) ( (d) - darr_offset(typeof(d[0])) ); \
	darr; \
})

#define darr_free(d) free(__to_darr((d)))

#define __copy_darr(din, newlen) ({ \
	typeof(din) dout = malloc(sizeof(typeof(din)) + len*sizeof(*(din->data))); \
	dout->cap = newlen*sizeof(typeof(*(din->data))); \
	dout->len = din->len; \
	memcpy(dout->data, din->data, din->len*sizeof(typeof(*(din->data)))); \
	dout; \
})

#define copy_darr(din, newlen) __copy_darr(__to_darr(din), newlen)->data

#define darr_len(d) (__to_darr((d)))->len
#define darr_cap(d) (__to_darr((d)))->cap

//#define __darr_unsafe_append(d, i) {
//
//#define darr_append(darr_p, i) { \
//	struct darr *d = (struct darr*) (*(darr_p) - 2*sizeof(size_t)); \
//	size_t isize = sizeof(i); \
//	if (d->len + isize >= d->cap) { struct darr *darrnew = __copy_darr(d, 2*d->cap); free(d); d = newdata; } \
//	((typeof(i)*) d->data)[d->len] = i; *(size_t*) (*(darr_p) - sizeof(size_t)) += isize; \
//}

#endif //_DARRAY_H
