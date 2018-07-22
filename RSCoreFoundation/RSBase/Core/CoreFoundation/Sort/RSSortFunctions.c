//
//  RSSortFunctions.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/7/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include "RSSortFunctions.h"
#include "../../BaseStructure/RSPrivate/RSPrivateOperatingSystem/RSPrivateOSCpu.h"

//void RSQSortIndex(RSIndex numbers[], RSIndex left, RSIndex right, RSComparisonOrder order)
//{
//    RSIndex pivot, l_hold, r_hold;
//    
//    l_hold = left;
//    r_hold = right;
//    pivot = numbers[left];
//
//    while (left < right)
//    {
//        if (order == RSOrderedAscending)
//        {
//            while ((numbers[right] > pivot) && (left < right))
//                right--;
//            if (left != right)
//            {
//                numbers[left] = numbers[right];
//                left++;
//            }
//            while ((numbers[left] < pivot) && (left < right))
//                left++;
//            if (left != right)
//            {
//                numbers[right] = numbers[left];
//                right--;
//            }
//        }
//        else
//        {
//            while ((numbers[right] < pivot) && (left < right))
//                right--;
//            if (left != right)
//            {
//                numbers[left] = numbers[right];
//                left++;
//            }
//            while ((numbers[left] > pivot) && (left < right))
//                left++;
//            if (left != right)
//            {
//                numbers[right] = numbers[left];
//                right--;
//            }
//        }
//        
//    }
//    numbers[left] = pivot;
//    pivot = left;
//    left = l_hold;
//    right = r_hold;
//    if (left < pivot)
//        RSQSortIndex(numbers, left, pivot-1, order);
//    if (right > pivot)
//        RSQSortIndex(numbers, pivot+1, right, order);
//}

/***********************************************************************************************************************/
static RSIndex __RSQSortCore(void** list, RSIndex left, RSIndex right, RSIndex elementSize, RSComparisonOrder order, RSComparatorFunction comparator, void *context)
{
    void* pivot = nil;
    pivot = list[left];
//    pivot = list[(left + right) / 2];
//    list[(left + right) / 2] = list[left];
    RSComparisonResult compares[2] = {RSCompareGreaterThan};
    compares[0] = RSCompareGreaterThan;
    compares[1] = RSCompareGreaterThan;
    compares[order == RSOrderedAscending ? 1 : 0] = RSCompareLessThan;
    while (left < right)
    {
        while ((compares[0] == comparator(list[right], pivot, context)) && (left < right))
            right--;
        if (left != right)
        {
            list[left] = list[right];
            left++;
        }
        while ((compares[1] == comparator(list[left], pivot, context)) && (left < right))
            left++;
        if (left != right)
        {
            list[right] = list[left];
            right--;
        }
    }
    list[right] = pivot;
    return left;
}
static void __RSQSortArray(void** list, RSIndex left, RSIndex right, RSIndex elementSize, RSComparisonOrder order, RSComparatorFunction comparator, void *context)
{
    if (left >= right) return;
    RSIndex newOffset = 0;
    newOffset = __RSQSortCore(list, left, right, elementSize, order, comparator, context);
    __RSQSortArray(list, left, newOffset - 1, elementSize, order, comparator, context);
    __RSQSortArray(list, newOffset + 1, right, elementSize, order, comparator, context);
}

RSExport void RSQSortArray(void **list, RSIndex count, RSIndex elementSize, RSComparisonOrder order, RSComparatorFunction comparator, void *context)
{
    if (list == nil || count <= 1 || elementSize < 1 || comparator == nil) return;
    return __RSQSortArray(list, 0, count - 1, elementSize, order, comparator, context);
}

/***********************************************************************************************************************/
#if RS_BLOCKS_AVAILABLE
typedef RSComparisonResult (^COMPARATOR)(RSIndex, RSIndex);
#else
typedef RSComparisonResult (*COMPARATOR)(RSIndex, RSIndex);
#endif
static void __RSSimpleMerge(RSIndex listp[], RSIndex cnt1, RSIndex cnt2, RSIndex tmp[], COMPARATOR cmp) {
    if (cnt1 <= 0 || cnt2 <= 0) return;
    // if the last element of listp1 <= the first of listp2, lists are already ordered
    if (16 < cnt1 + cnt2 && cmp(listp[cnt1 - 1], listp[cnt1]) <= 0) return;
    
    RSIndex idx = 0, idx1 = 0, idx2 = cnt1;
    for (;;) {
        if (cnt1 <= idx1) {
            while (idx--) {
                listp[idx] = tmp[idx];
            }
            return;
        }
        if (cnt1 + cnt2 <= idx2) {
            for (RSIndex t = cnt1 + cnt2 - 1; idx <= t; t--) {
                listp[t] = listp[t - cnt2];
            }
            while (idx--) {
                listp[idx] = tmp[idx];
            }
            return;
        }
        RSIndex v1 = listp[idx1], v2 = listp[idx2];
        if (cmp(v1, v2) <= 0) {
            tmp[idx] = v1;
            idx1++;
        } else {
            tmp[idx] = v2;
            idx2++;
        }
        idx++;
    }
}

static void __RSSimpleMergeSort(RSIndex listp[], RSIndex cnt, RSIndex tmp[], COMPARATOR cmp) {
    if (cnt < 2) {
        /* do nothing */
    } else if (2 == cnt) {
        RSIndex v0 = listp[0], v1 = listp[1];
        if (0 < cmp(v0, v1)) {
            listp[0] = v1;
            listp[1] = v0;
        }
    } else if (3 == cnt) {
        RSIndex v0 = listp[0], v1 = listp[1], v2 = listp[2], vt;
        if (0 < cmp(v0, v1)) {
            vt = v0;
            v0 = v1;
            v1 = vt;
        }
        if (0 < cmp(v1, v2)) {
            vt = v1;
            v1 = v2;
            v2 = vt;
            if (0 < cmp(v0, v1)) {
                vt = v0;
                v0 = v1;
                v1 = vt;
            }
        }
        listp[0] = v0;
        listp[1] = v1;
        listp[2] = v2;
    } else {
        RSIndex half_cnt = cnt / 2;
        __RSSimpleMergeSort(listp, half_cnt, tmp, cmp);
        __RSSimpleMergeSort(listp + half_cnt, cnt - half_cnt, tmp, cmp);
        __RSSimpleMerge(listp, half_cnt, cnt - half_cnt, tmp, cmp);
    }
}

static void __RSSimpleMergeR(RSIndex listp[], RSIndex cnt1, RSIndex cnt2, RSIndex tmp[], COMPARATOR cmp) {
    if (cnt1 <= 0 || cnt2 <= 0) return;
    // if the last element of listp1 <= the first of listp2, lists are already ordered
    if (16 < cnt1 + cnt2 && cmp(listp[cnt1 - 1], listp[cnt1]) <= 0) return;
    
    RSIndex idx = 0, idx1 = 0, idx2 = cnt1;
    for (;;) {
        if (cnt1 <= idx1) {
            while (idx--) {
                listp[idx] = tmp[idx];
            }
            return;
        }
        if (cnt1 + cnt2 <= idx2) {
            for (RSIndex t = cnt1 + cnt2 - 1; idx <= t; t--) {
                listp[t] = listp[t - cnt2];
            }
            while (idx--) {
                listp[idx] = tmp[idx];
            }
            return;
        }
        RSIndex v1 = listp[idx1], v2 = listp[idx2];
        if (cmp(v1, v2) > 0) {
            tmp[idx] = v1;
            idx1++;
        } else {
            tmp[idx] = v2;
            idx2++;
        }
        idx++;
    }
}

static void __RSSimpleMergeSortR(RSIndex listp[], RSIndex cnt, RSIndex tmp[], COMPARATOR cmp) {
    if (cnt < 2) {
        /* do nothing */
    } else if (2 == cnt) {
        RSIndex v0 = listp[0], v1 = listp[1];
        if (0 > cmp(v0, v1)) {
            listp[0] = v1;
            listp[1] = v0;
        }
    } else if (3 == cnt) {
        RSIndex v0 = listp[0], v1 = listp[1], v2 = listp[2], vt;
        if (0 > cmp(v0, v1)) {
            vt = v0;
            v0 = v1;
            v1 = vt;
        }
        if (0 > cmp(v1, v2)) {
            vt = v1;
            v1 = v2;
            v2 = vt;
            if (0 > cmp(v0, v1)) {
                vt = v0;
                v0 = v1;
                v1 = vt;
            }
        }
        listp[0] = v0;
        listp[1] = v1;
        listp[2] = v2;
    } else {
        RSIndex half_cnt = cnt / 2;
        __RSSimpleMergeSortR(listp, half_cnt, tmp, cmp);
        __RSSimpleMergeSortR(listp + half_cnt, cnt - half_cnt, tmp, cmp);
        __RSSimpleMergeR(listp, half_cnt, cnt - half_cnt, tmp, cmp);
    }
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS
// Excluded from linux for dispatch dependency

// if !right, put the cnt1 smallest values in tmp, else put the cnt2 largest values in tmp
static void __RSSortIndexesNMerge(RSIndex listp1[], RSIndex cnt1, RSIndex listp2[], RSIndex cnt2, RSIndex tmp[], size_t right, COMPARATOR cmp) {
    // if the last element of listp1 <= the first of listp2, lists are already ordered
    if (16 < cnt1 + cnt2 && cmp(listp1[cnt1 - 1], listp2[0]) <= 0) {
        memmove(tmp, (right ? listp2 : listp1), (right ? cnt2 : cnt1) * sizeof(RSIndex));
        return;
    }
    
    if (right) {
        RSIndex *listp1_end = listp1;
        RSIndex *listp2_end = listp2;
        RSIndex *tmp_end = tmp;
        listp1 += cnt1 - 1;
        listp2 += cnt2 - 1;
        tmp += cnt2;
        while (tmp_end < tmp) {
            tmp--;
            if (listp2 < listp2_end) {
                listp1--;
                *tmp = *listp1;
            } else if (listp1 < listp1_end) {
                listp2--;
                *tmp = *listp2;
            } else {
                RSIndex v1 = *listp1, v2 = *listp2;
                RSComparisonResult res = cmp(v1, v2);
                if (res <= 0) {
                    *tmp = v2;
                    listp2--;
                } else {
                    *tmp = v1;
                    listp1--;
                }
            }
        }
    } else {
        RSIndex *listp1_end = listp1 + cnt1;
        RSIndex *listp2_end = listp2 + cnt2;
        RSIndex *tmp_end = tmp + cnt1;
        while (tmp < tmp_end) {
            if (listp2_end <= listp2) {
                *tmp = *listp1;
                listp1++;
            } else if (listp1_end <= listp1) {
                *tmp = *listp2;
                listp2++;
            } else {
                RSIndex v1 = *listp1, v2 = *listp2;
                RSComparisonResult res = cmp(v1, v2);
                if (res <= 0) {
                    *tmp = v1;
                    listp1++;
                } else {
                    *tmp = v2;
                    listp2++;
                }
            }
            tmp++;
        }
    }
}

static void __RSSortIndexesNMergeR(RSIndex listp1[], RSIndex cnt1, RSIndex listp2[], RSIndex cnt2, RSIndex tmp[], size_t right, COMPARATOR cmp) {
    // if the last element of listp1 <= the first of listp2, lists are already ordered
    if (16 < cnt1 + cnt2 && cmp(listp1[cnt1 - 1], listp2[0]) >= 0) {
        memmove(tmp, (right ? listp2 : listp1), (right ? cnt2 : cnt1) * sizeof(RSIndex));
        return;
    }
    
    if (right) {
        RSIndex *listp1_end = listp1;
        RSIndex *listp2_end = listp2;
        RSIndex *tmp_end = tmp;
        listp1 += cnt1 - 1;
        listp2 += cnt2 - 1;
        tmp += cnt2;
        while (tmp_end < tmp) {
            tmp--;
            if (listp2 < listp2_end) {
                listp1--;
                *tmp = *listp1;
            } else if (listp1 < listp1_end) {
                listp2--;
                *tmp = *listp2;
            } else {
                RSIndex v1 = *listp1, v2 = *listp2;
                RSComparisonResult res = cmp(v1, v2);
                if (res > 0) {
                    *tmp = v2;
                    listp2--;
                } else {
                    *tmp = v1;
                    listp1--;
                }
            }
        }
    } else {
        RSIndex *listp1_end = listp1 + cnt1;
        RSIndex *listp2_end = listp2 + cnt2;
        RSIndex *tmp_end = tmp + cnt1;
        while (tmp < tmp_end) {
            if (listp2_end <= listp2) {
                *tmp = *listp1;
                listp1++;
            } else if (listp1_end <= listp1) {
                *tmp = *listp2;
                listp2++;
            } else {
                RSIndex v1 = *listp1, v2 = *listp2;
                RSComparisonResult res = cmp(v1, v2);
                if (res > 0) {
                    *tmp = v1;
                    listp1++;
                } else {
                    *tmp = v2;
                    listp2++;
                }
            }
            tmp++;
        }
    }
}

/* Merging algorithm based on
 "A New Parallel Sorting Algorithm based on Odd-Even Mergesort", Ezequiel Herruzo, et al
 */
static void __RSSortIndexesN(RSIndex listp[], RSIndex count, int32_t ncores, COMPARATOR cmp) {
    /* Divide the array up into up to ncores, multiple-of-16-sized, chunks */
    RSIndex sz = ((((count + ncores - 1) / ncores) + 15) / 16) * 16;
    RSIndex num_sect = (count + sz - 1) / sz;
    RSIndex last_sect_len = count + sz - sz * num_sect;
    
    STACK_BUFFER_DECL(RSIndex *, stack_tmps, num_sect);
    for (RSIndex idx = 0; idx < num_sect; idx++) {
        stack_tmps[idx] = (RSIndex *)malloc(sz * sizeof(RSIndex));
    }
    RSIndex **tmps = stack_tmps;
    
    RSPerformBlockRepeatWithFlags(num_sect, RSPerformBlockOverCommit, ^(RSIndex sect) {
        RSIndex sect_len = (sect < num_sect - 1) ? sz : last_sect_len;
        __RSSimpleMergeSort(listp + sect * sz, sect_len, tmps[sect], cmp); // naturally stable
    });
    
    RSIndex even_phase_cnt = ((num_sect / 2) * 2);
    RSIndex odd_phase_cnt = (((num_sect - 1) / 2) * 2);
    for (RSIndex idx = 0; idx < (num_sect + 1) / 2; idx++) {
        RSPerformBlockRepeatWithFlags(even_phase_cnt, RSPerformBlockOverCommit, ^(RSIndex sect) {
            size_t right = sect & (size_t)0x1;
            RSIndex *left_base = listp + sect * sz - (right ? sz : 0);
            RSIndex *right_base = listp + sect * sz + (right ? 0 : sz);
            RSIndex sect2_len = (sect + 1 + (right ? 0 : 1) == num_sect) ? last_sect_len : sz;
            __RSSortIndexesNMerge(left_base, sz, right_base, sect2_len, tmps[sect], right, cmp);
        });
        if (num_sect & 0x1) {
            memmove(tmps[num_sect - 1], listp + (num_sect - 1) * sz, last_sect_len * sizeof(RSIndex));
        }
        RSPerformBlockRepeatWithFlags(odd_phase_cnt, RSPerformBlockOverCommit, ^(RSIndex sect) {
            size_t right = sect & (size_t)0x1;
            RSIndex *left_base = tmps[sect + (right ? 0 : 1)];
            RSIndex *right_base = tmps[sect + (right ? 1 : 2)];
            RSIndex sect2_len = (sect + 1 + (right ? 1 : 2) == num_sect) ? last_sect_len : sz;
            __RSSortIndexesNMerge(left_base, sz, right_base, sect2_len, listp + sect * sz + sz, right, cmp);
        });
        memmove(listp + 0 * sz, tmps[0], sz * sizeof(RSIndex));
        if (!(num_sect & 0x1)) {
            memmove(listp + (num_sect - 1) * sz, tmps[num_sect - 1], last_sect_len * sizeof(RSIndex));
        }
    }
    
    for (RSIndex idx = 0; idx < num_sect; idx++) {
        free(stack_tmps[idx]);
    }
}

static void __RSSortIndexesNR(RSIndex listp[], RSIndex count, int32_t ncores, COMPARATOR cmp) {
    /* Divide the array up into up to ncores, multiple-of-16-sized, chunks */
    RSIndex sz = ((((count + ncores - 1) / ncores) + 15) / 16) * 16;
    RSIndex num_sect = (count + sz - 1) / sz;
    RSIndex last_sect_len = count + sz - sz * num_sect;
    
    STACK_BUFFER_DECL(RSIndex *, stack_tmps, num_sect);
    for (RSIndex idx = 0; idx < num_sect; idx++) {
        stack_tmps[idx] = (RSIndex *)malloc(sz * sizeof(RSIndex));
    }
    RSIndex **tmps = stack_tmps;
    
    RSPerformBlockRepeatWithFlags(num_sect, RSPerformBlockOverCommit, ^(RSIndex sect) {
        RSIndex sect_len = (sect < num_sect - 1) ? sz : last_sect_len;
        __RSSimpleMergeSortR(listp + sect * sz, sect_len, tmps[sect], cmp); // naturally stable
    });
    
    RSIndex even_phase_cnt = ((num_sect / 2) * 2);
    RSIndex odd_phase_cnt = (((num_sect - 1) / 2) * 2);
    for (RSIndex idx = 0; idx < (num_sect + 1) / 2; idx++) {
        RSPerformBlockRepeatWithFlags(even_phase_cnt, RSPerformBlockOverCommit, ^(RSIndex sect) {
            size_t right = sect & (size_t)0x1;
            RSIndex *left_base = listp + sect * sz - (right ? sz : 0);
            RSIndex *right_base = listp + sect * sz + (right ? 0 : sz);
            RSIndex sect2_len = (sect + 1 + (right ? 0 : 1) == num_sect) ? last_sect_len : sz;
            __RSSortIndexesNMergeR(left_base, sz, right_base, sect2_len, tmps[sect], right, cmp);
        });
        if (num_sect & 0x1) {
            memmove(tmps[num_sect - 1], listp + (num_sect - 1) * sz, last_sect_len * sizeof(RSIndex));
        }
        RSPerformBlockRepeatWithFlags(odd_phase_cnt, RSPerformBlockOverCommit, ^(RSIndex sect) {
            size_t right = sect & (size_t)0x1;
            RSIndex *left_base = tmps[sect + (right ? 0 : 1)];
            RSIndex *right_base = tmps[sect + (right ? 1 : 2)];
            RSIndex sect2_len = (sect + 1 + (right ? 1 : 2) == num_sect) ? last_sect_len : sz;
            __RSSortIndexesNMergeR(left_base, sz, right_base, sect2_len, listp + sect * sz + sz, right, cmp);
        });
        memmove(listp + 0 * sz, tmps[0], sz * sizeof(RSIndex));
        if (!(num_sect & 0x1)) {
            memmove(listp + (num_sect - 1) * sz, tmps[num_sect - 1], last_sect_len * sizeof(RSIndex));
        }
    }
    
    for (RSIndex idx = 0; idx < num_sect; idx++) {
        free(stack_tmps[idx]);
    }
}
#endif


RSExport void RSSortIndexes(RSIndex *indexBuffer, RSIndex count, RSOptionFlags opts, COMPARATOR cmp, RSComparisonOrder order)
{
    if (nil == indexBuffer || count < 2 || cmp == nil) return;
    if (INTPTR_MAX / sizeof(RSIndex) < count) return;
    int ncores = 0;
    if (opts & RSSortConcurrent)
    {
        ncores = (int)__RSActiveProcessorCount();
        if (count < 160 || ncores < 2) {
            opts = (opts & ~RSSortConcurrent);
        } else if (count < 640 && 2 < ncores) {
            ncores = 2;
        } else if (count < 3200 && 4 < ncores) {
            ncores = 4;
        } else if (count < 16000 && 8 < ncores) {
            ncores = 8;
        }
        if (16 < ncores) {
            ncores = 16;
        }
    }
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS
    if (count <= 65536) {
        for (RSIndex idx = 0; idx < count; idx++) indexBuffer[idx] = idx;
    }
    else
    {
        // init indexBuffer with idx concurrently
        /* Specifically hard-coded to 8; the count has to be very large before more chunks and/or cores is worthwhile. */
        RSIndex sz = ((((size_t)count + 15) / 16) * 16) / 8;
        RSPerformBlockRepeatWithFlags(8, RSPerformBlockOverCommit, ^(RSIndex n) {
            RSIndex idx = n * sz, lim = __RSMin(idx + sz, count);
            for (; idx < lim; idx++) {
                indexBuffer[idx] = idx;
            }
        });
    }
#else
    for (RSIndex idx = 0; idx < count; idx++) indexBuffer[idx] = idx;
#endif
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS
    if (opts & RSSortConcurrent) {
        if (order == RSOrderedAscending)
            __RSSortIndexesN(indexBuffer, count, ncores, cmp); // naturally stable
        else
            __RSSortIndexesNR(indexBuffer, count, ncores, cmp);
        return;
    }
#endif
    STACK_BUFFER_DECL(RSIndex, local, count <= 4096 ? count : 1);
    RSIndex *tmp = (count <= 4096) ? local : (RSIndex *)malloc(count * sizeof(RSIndex));
    if (order == RSOrderedAscending) __RSSimpleMergeSort(indexBuffer, count, tmp, cmp); // naturally stable
    else __RSSimpleMergeSortR(indexBuffer, count, tmp, cmp);
    if (local != tmp) free(tmp);
}

RSExport void RSSortArray(void *list, RSIndex count, RSIndex elementSize, RSComparisonOrder order, RSComparatorFunction comparator, void *context)
{
    if (count < 2 || elementSize < 1) return;
    STACK_BUFFER_DECL(RSIndex, locali, count <= 4096 ? count : 1);
    RSIndex *indexes = (count <= 4096) ? locali : (RSIndex *)RSAllocatorAllocate(RSAllocatorSystemDefault, count * sizeof(RSIndex));
    RSSortIndexes(indexes,
                  count,
                  RSSortConcurrent,
                  ^RSComparisonResult (RSIndex a, RSIndex b) {
                    return comparator((char *)list + a * elementSize, (char *)list + b * elementSize, context);
                  },
                  order);
    STACK_BUFFER_DECL(uint8_t, locals, count <= (16 * 1024 / elementSize) ? count * elementSize : 1);
    void *store = (count <= (16 * 1024 / elementSize)) ? locals : RSAllocatorAllocate(RSAllocatorSystemDefault, count * elementSize);
    for (RSIndex idx = 0; idx < count; idx++)
    {
        if (sizeof(uintptr_t) == elementSize)
        {
            uintptr_t *a = (uintptr_t *)list + indexes[idx];
            uintptr_t *b = (uintptr_t *)store + idx;
            *b = *a;
        }
        else
        {
            memmove((char *)store + idx * elementSize, (char *)list + indexes[idx] * elementSize, elementSize);
        }
    }
    // no swapping or modification of the original list has occurred until this point
    memmove(list, store, count * elementSize);
    if (locals != store) RSAllocatorDeallocate(RSAllocatorSystemDefault, store);
    if (locali != indexes) RSAllocatorDeallocate(RSAllocatorSystemDefault, indexes);
}
/***********************************************************************************************************************/
RSInline void __RSSwapIndex(RSIndex* a, RSIndex* b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

void __RSMinHeapFixup(RSIndex a[], RSIndex i)
{
    for (RSIndex j = (i - 1) / 2; j >= 0 && a[i] > a[j]; i = j, j = (i - 1) / 2)
        __RSSwapIndex(&a[i], &a[j]);
}
void __RSMinHeapInsert(RSIndex a[], RSIndex n, RSIndex nNum)
{
    a[n] = nNum;
    __RSMinHeapFixup(a, n);
}

void __RSMinHeapFixdown(RSIndex **a, RSIndex i, RSIndex n)
{
    RSIndex j, temp;
    
    temp = (*a)[i];
    j = 2 * i + 1;  // j = left child
    while (j < n)
    {
        if (j + 1 < n && (*a)[j + 1] < (*a)[j])   // compare the left and right child and get the min one.
            j++;
        
        if ((*a)[j] >= temp)                   // if the child gather than father, it is right.
            break;
        
        (*a)[i] = (*a)[j];                        // swap the parent node with the min child node.
        i = j;                              // now i is the parent node.
        j = 2 * i + 1;                      // get the current child node.
    }
    (*a)[i] = temp;                            // restore the a[i] from temp.
}
//在最小堆中删除数
void __RSMinHeapDeleteNumber(RSIndex **a, RSIndex n)
{
    __RSSwapIndex(&(*a)[0], &(*a)[n - 1]);
    __RSMinHeapFixdown(a, 0, n - 1);
}


void __RSMakeMinHeap(RSIndex **a, RSIndex n)
{
    for (RSIndex i = n / 2 - 1; i >= 0; i--)
        __RSMinHeapFixdown(a, i, n);
}

void __RSMinHeapSortToDescendArray(RSIndex a[], RSIndex n)  //heap sort to descend array
{
    __RSMakeMinHeap(&a, n);
    for (RSIndex i = n - 1; i >= 1; i--)
    {
        __RSSwapIndex(&a[i], &a[0]);
        __RSMinHeapFixdown(&a, 0, i);
    }
}

