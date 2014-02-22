/* qsort() */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef int T;          /* type of item to be sorted */

#define MAXSTACK (sizeof(size_t) * CHAR_BIT)

#define exchange(a, b, size) \
{ \
    size_t s; \
    int *ai, *bi; \
    char *ac, *bc; \
    ai = (int *)a; \
    bi = (int *)b; \
    for (s = sizeof(int); s <= size; s += sizeof(int)) { \
        int t = *ai; \
        *ai++ = *bi; \
        *bi++ = t; \
    } \
    ac = (char *)ai; \
    bc = (char *)bi; \
    for (s = s - sizeof(int) + 1; s <= size; s++) { \
        char t = *ac; \
        *ac++ = *bc; \
        *bc++ = t; \
    } \
}

//seli:add a new pointer as first param
void vvqsort(
		void* obj,
		void *base, size_t nmemb, size_t size,
        int (*compar)(void *, const void *, const void *)) {
    void *lbStack[MAXSTACK], *ubStack[MAXSTACK];
    int sp;
    unsigned int offset;

    /********************
     *  ANSI-C qsort()  *
     ********************/

    lbStack[0] = (char *)base;
    ubStack[0] = (char *)base + (nmemb-1)*size;
    for (sp = 0; sp >= 0; sp--) {
        char *lb, *ub, *m;
        char *P, *i, *j;

        lb = (char *)lbStack[sp];
        ub = (char *)ubStack[sp];

        while (lb < ub) {

            /* select pivot and exchange with 1st element */
            offset = (ub - lb) >> 1;
            P = lb + offset - offset % size;
            exchange (lb, P, size);

            /* partition into two segments */
            i = lb + size;
            j = ub;
            while (1) {
                while (i < j && compar(obj, lb, i) > 0) i += size;
                while (j >= i && compar(obj, j, lb) > 0) j -= size;
                if (i >= j) break;
                exchange (i, j, size);
                j -= size;
                i += size;
            }

            /* pivot belongs in A[j] */
            exchange (lb, j, size);
            m = j;

            /* keep processing smallest segment, and stack largest */
            if (m - lb <= ub - m) {
                if (m + size < ub) {
                    lbStack[sp] = m + size;
                    ubStack[sp++] = ub;
                }
                ub = m - size;
            } else {
                if (m - size > lb) {
                    lbStack[sp] = lb; 
                    ubStack[sp++] = m - size;
                }
                lb = m + size;
            }
        }
    }
}
//
//void fill(T *lb, T *ub) {
//    T *i;
//    srand(1);
//    for (i = lb; i <= ub; i++) *i = rand();
//}
//
//int Comp(const void *a, const void *b) { return *(T *)a - *(T *)b; }
//
//int main(int argc, char *argv[]) {
//    int maxnum;
//    int *a, *lb, *ub;
//
//    /* command-line:
//     *
//     *   qsort maxnum
//     *
//     *   qsort 2000
//     *       sorts 2000 records
//     *
//     */
//
//    maxnum = atoi(argv[1]);
//    if ((a = malloc(maxnum * sizeof(T))) == 0) {
//        fprintf (stderr, "insufficient memory (a)\n");
//        exit(1);
//    }
//    lb = a;  ub = a + maxnum - 1;
//
//    fill(lb, ub);
//    qsort(a, maxnum, sizeof(T), Comp);
//    return 0;
//}
