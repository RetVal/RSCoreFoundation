//
//  Date.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Date__
#define __RSCoreFoundation__Date__

#include <RSFoundation/Object.h>

namespace RSFoundation {
    namespace Basic {
        
        class Date : public Object {
        public:
            struct GregorianDate {
                RSInt32 year;
                RSUInt8 month;
                RSUInt8 day;
                RSUInt8 hour;
                RSUInt8 minute;
                RSTimeInterval second;
                
                GregorianDate() = default;
                GregorianDate(const GregorianDate&) = default;
                
                GregorianDate(RSInt32 year, RSInt8 month, RSInt8 day, RSInt8 hour, RSInt8 minute, RSTimeInterval second) : year(year), month(month), hour(hour), minute(minute), second(second) {}
            };
            
            typedef RSTimeInterval AbsoluteTime;
            static AbsoluteTime GetCurrentTime();
            Date();
            Date(AbsoluteTime time);
            Date(const Date &);
            Date(Date&&);
            Date& operator=(const Date&);
            Date& operator=(Date&&);
            static bool IsEqualTo(const Date& a, const Date& b);
            static ComparisonResult Compare(const Date& a, const Date& b);
            bool operator==(const Date& value) const;
            bool operator!=(const Date& value) const;
            bool operator>(const Date& value) const;
            bool operator>=(const Date& value) const;
            bool operator<(const Date& value) const;
            bool operator<=(const Date& value) const;
            operator bool() const;
            
        public:
            
            AbsoluteTime GetTime() const { return time; }
            GregorianDate GetGregorianDate() const;
            AbsoluteTime GetTimeSince(const Date&value) const;
            RSUInt32 GetDayOfWeek() const;
            RSUInt32 GetDayOfYear() const;
            RSUInt32 GetWeekOfYear() const;
            
        public:
            constexpr static const AbsoluteTime Since1970 = 978307200.0L;
            constexpr static const AbsoluteTime Since1904 = 3061152000.0L;
            
        private:
            AbsoluteTime time;
        };
    }
}

#endif /* defined(__RSCoreFoundation__Date__) */
