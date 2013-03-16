
#ifndef __HEAP_H
#define __HEAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct heap heap;

struct heap *hpCreate(int (*compare)(void *lhs, void *rhs));
void hpFree(struct heap *hp);
void hpAdd(struct heap *hp, void *data);
void *hpPeek(struct heap *hp);
void *hpTop(struct heap *hp);
unsigned hpSize(struct heap *hp);

#ifdef __cplusplus
}
#endif

#endif
