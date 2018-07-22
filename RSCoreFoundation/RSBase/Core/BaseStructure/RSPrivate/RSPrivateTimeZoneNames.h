//
//  RSPrivateTimeZoneNames.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/6/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPrivateTimeZoneNames_h
#define RSCoreFoundation_RSPrivateTimeZoneNames_h
struct __RSTimeZoneInformation {
    RSCBuffer timeZoneName;
    RSInteger secondsFromGMT;
};
extern const RSRange __RSTimeZoneNameRangeAfrica;
extern const RSRange __RSTimeZoneNameRangeAmerica;
extern const RSRange __RSTimeZoneNameRangeAntarctica;
extern const RSRange __RSTimeZoneNameRangeArctic;
extern const RSRange __RSTimeZoneNameRangeAsia;
extern const RSRange __RSTimeZoneNameRangeAtlantic;
extern const RSRange __RSTimeZoneNameRangeAustralia;
extern const RSRange __RSTimeZoneNameRangeEurope;
extern const RSRange __RSTimeZoneNameRangeGMT;
extern const RSRange __RSTimeZoneNameRangeIndian;
extern const RSRange __RSTimeZoneNameRangePacific;

extern const struct __RSTimeZoneInformation __RSPrivateTimeZoneInfo[];
extern const unsigned int __RSPrivateTimeZoneNameCount;

#endif
