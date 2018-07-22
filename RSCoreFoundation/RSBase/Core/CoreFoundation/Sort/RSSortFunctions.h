//
//  RSSortFunctions.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/7/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSSortFunctions_h
#define RSCoreFoundation_RSSortFunctions_h
#if !RS_BLOCKS_AVAILABLE
RSInline BOOL ___RSZSortComparisonOver0ReturnYes(RSComparisonResult result)
{
    return result > 0 ? YES : NO;
}

RSInline BOOL ___RSZSortComparisonBelow0ReturnYes(RSComparisonResult result)
{
    return result < 0 ? YES : NO;
}
#endif

enum RSSortOptionFlags {
    RSSortConcurrent = 2,
    RSSortStable = 4
};

RSExport void RSQSortArray(void **list, RSIndex count, RSIndex elementSize, RSComparisonOrder order, RSComparatorFunction comparator, void *context);
RSExport void RSSortArray(void *list, RSIndex count, RSIndex elementSize, RSComparisonOrder order, RSComparatorFunction comparator, void *context);
#if RS_BLOCKS_AVAILABLE
RSExport void RSSortIndexes(RSIndex *indexBuffer, RSIndex count, RSOptionFlags opts, RSComparisonResult (^cmp)(RSIndex idx1, RSIndex idx2), RSComparisonOrder order);
#else 
typedef RSComparisonResult (*COMPARATOR)(RSIndex, RSIndex);
RSExport void RSSortIndexes(RSIndex *indexBuffer, RSIndex count, RSOptionFlags opts, COMPARATOR comparator, RSComparisonOrder order);
#endif
#endif
