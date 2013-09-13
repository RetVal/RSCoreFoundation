//
//  RSLocale.c
//  RSCoreFoundation
//
//  Created by RetVal on 8/4/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSLocale.h>
#include <RSCoreFoundation/RSCalendar.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSString.h>

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
#include "unicode/uloc.h"           // ICU locales
#include "unicode/ulocdata.h"       // ICU locale data
#include "unicode/ucal.h"
#include "unicode/ucurr.h"          // ICU currency functions
#include "unicode/uset.h"           // ICU Unicode sets
#include "unicode/putil.h"          // ICU low-level utilities
#include "unicode/umsg.h"           // ICU message formatting
#include "unicode/ucol.h"
#endif

#include <stdlib.h>
#include <string.h>

#if DEPLOYMENT_TARGET_EMBEDDED_MINI
// Some compatability definitions
#define ULOC_FULLNAME_CAPACITY 157
#define ULOC_KEYWORD_AND_VALUES_CAPACITY 100

//typedef long UErrorCode;
//#define U_BUFFER_OVERFLOW_ERROR 15
//#define U_ZERO_ERROR 0
//
//typedef uint16_t UChar;
#endif

RS_PUBLIC_CONST_STRING_DECL(RSLocaleCurrentLocaleDidChangeNotification, "RSLocaleCurrentLocaleDidChangeNotification")

static const char *kCalendarKeyword = "calendar";
static const char *kCollationKeyword = "collation";
#define kMaxICUNameSize 1024

typedef struct __RSLocale *RSMutableLocaleRef;

RS_PUBLIC_CONST_STRING_DECL(__RSLocaleCollatorID, "locale:collator id");


enum
{
    __RSLocaleKeyTableCount = 21
};
    
struct key_table {
    RSStringRef key;
    bool (*get)(RSLocaleRef, bool user, RSTypeRef *, RSStringRef context);  // returns an immutable copy & reference
    bool (*set)(RSMutableLocaleRef, RSTypeRef, RSStringRef context);
    bool (*name)(const char *, const char *, RSStringRef *);
    RSStringRef context;
};

// Must forward decl. these functions:
static bool __RSLocaleCopyLocaleID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleSetNOP(RSMutableLocaleRef locale, RSTypeRef cf, RSStringRef context);
static bool __RSLocaleFullName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleCopyCodes(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCountryName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleScriptName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleLanguageName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleCurrencyShortName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleCopyExemplarCharSet(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleVariantName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleNoName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleCopyCalendarID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCalendarName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleCollationName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleCopyUsesMetric(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCopyCalendar(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCopyCollationID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCopyMeasurementSystem(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCopyNumberFormat(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCopyNumberFormat2(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCurrencyFullName(const char *locale, const char *value, RSStringRef *out);
static bool __RSLocaleCopyCollatorID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);
static bool __RSLocaleCopyDelimiter(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context);

RS_PUBLIC_CONST_STRING_DECL(RSLocaleAlternateQuotationBeginDelimiterKey, "RSLocaleAlternateQuotationBeginDelimiterKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleAlternateQuotationEndDelimiterKey, "RSLocaleAlternateQuotationEndDelimiterKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleQuotationBeginDelimiterKey, "RSLocaleQuotationBeginDelimiterKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleQuotationEndDelimiterKey, "RSLocaleQuotationEndDelimiterKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleCalendarIdentifierKey, "calendar"); // ***
RS_PUBLIC_CONST_STRING_DECL(RSLocaleCalendarKey, "RSLocaleCalendarKey");

RS_PUBLIC_CONST_STRING_DECL(RSLocaleCollationIdentifierKey, "collation"); // ***
RS_PUBLIC_CONST_STRING_DECL(RSLocaleCollatorIdentifierKey, "RSLocaleCollatorIdentifierKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleCountryCodeKey, "RSLocaleCountryCodeKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleCurrencyCodeKey, "currency"); // ***
RS_PUBLIC_CONST_STRING_DECL(RSLocaleCurrencySymbolKey, "RSLocaleCurrencySymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleDecimalSeparatorKey, "RSLocaleDecimalSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleExemplarCharacterSetKey, "RSLocaleExemplarCharacterSetKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleGroupingSeparatorKey, "RSLocaleGroupingSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleIdentifierKey, "RSLocaleIdentifierKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleLanguageCodeKey, "RSLocaleLanguageCodeKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleMeasurementSystemKey, "RSLocaleMeasurementSystemKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleScriptCodeKey, "RSLocaleScriptCodeKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleUsesMetricSystemKey, "RSLocaleUsesMetricSystemKey");
RS_PUBLIC_CONST_STRING_DECL(RSLocaleVariantCodeKey, "RSLocaleVariantCodeKey");

RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterAMSymbolKey, "RSDateFormatterAMSymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterCalendarKey, "RSDateFormatterCalendarKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterCalendarIdentifierKey, "RSDateFormatterCalendarIdentifierKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterDefaultDateKey, "RSDateFormatterDefaultDateKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterDefaultFormatKey, "RSDateFormatterDefaultFormatKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterDoesRelativeDateFormattingKey, "RSDateFormatterDoesRelativeDateFormattingKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterEraSymbolsKey, "RSDateFormatterEraSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterGregorianStartDateKey, "RSDateFormatterGregorianStartDateKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterIsLenientKey, "RSDateFormatterIsLenientKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterLongEraSymbolsKey, "RSDateFormatterLongEraSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterMonthSymbolsKey, "RSDateFormatterMonthSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterPMSymbolKey, "RSDateFormatterPMSymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterAmbiguousYearStrategyKey, "RSDateFormatterAmbiguousYearStrategyKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterQuarterSymbolsKey, "RSDateFormatterQuarterSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterShortMonthSymbolsKey, "RSDateFormatterShortMonthSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterShortQuarterSymbolsKey, "RSDateFormatterShortQuarterSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterShortStandaloneMonthSymbolsKey, "RSDateFormatterShortStandaloneMonthSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterShortStandaloneQuarterSymbolsKey, "RSDateFormatterShortStandaloneQuarterSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterShortStandaloneWeekdaySymbolsKey, "RSDateFormatterShortStandaloneWeekdaySymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterShortWeekdaySymbolsKey, "RSDateFormatterShortWeekdaySymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterStandaloneMonthSymbolsKey, "RSDateFormatterStandaloneMonthSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterStandaloneQuarterSymbolsKey, "RSDateFormatterStandaloneQuarterSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterStandaloneWeekdaySymbolsKey, "RSDateFormatterStandaloneWeekdaySymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterTimeZoneKey, "RSDateFormatterTimeZoneKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterTwoDigitStartDateKey, "RSDateFormatterTwoDigitStartDateKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterVeryShortMonthSymbolsKey, "RSDateFormatterVeryShortMonthSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterVeryShortStandaloneMonthSymbolsKey, "RSDateFormatterVeryShortStandaloneMonthSymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterVeryShortStandaloneWeekdaySymbolsKey, "RSDateFormatterVeryShortStandaloneWeekdaySymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterVeryShortWeekdaySymbolsKey, "RSDateFormatterVeryShortWeekdaySymbolsKey");
RS_PUBLIC_CONST_STRING_DECL(RSDateFormatterWeekdaySymbolsKey, "RSDateFormatterWeekdaySymbolsKey");

RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterAlwaysShowDecimalSeparatorKey, "RSNumberFormatterAlwaysShowDecimalSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterCurrencyCodeKey, "RSNumberFormatterCurrencyCodeKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterCurrencyDecimalSeparatorKey, "RSNumberFormatterCurrencyDecimalSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterCurrencyGroupingSeparatorKey, "RSNumberFormatterCurrencyGroupingSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterCurrencySymbolKey, "RSNumberFormatterCurrencySymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterDecimalSeparatorKey, "RSNumberFormatterDecimalSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterDefaultFormatKey, "RSNumberFormatterDefaultFormatKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterExponentSymbolKey, "RSNumberFormatterExponentSymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterFormatWidthKey, "RSNumberFormatterFormatWidthKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterGroupingSeparatorKey, "RSNumberFormatterGroupingSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterGroupingSizeKey, "RSNumberFormatterGroupingSizeKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterInfinitySymbolKey, "RSNumberFormatterInfinitySymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterInternationalCurrencySymbolKey, "RSNumberFormatterInternationalCurrencySymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterIsLenientKey, "RSNumberFormatterIsLenientKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMaxFractionDigitsKey, "RSNumberFormatterMaxFractionDigitsKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMaxIntegerDigitsKey, "RSNumberFormatterMaxIntegerDigitsKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMaxSignificantDigitsKey, "RSNumberFormatterMaxSignificantDigitsKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMinFractionDigitsKey, "RSNumberFormatterMinFractionDigitsKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMinIntegerDigitsKey, "RSNumberFormatterMinIntegerDigitsKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMinSignificantDigitsKey, "RSNumberFormatterMinSignificantDigitsKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMinusSignKey, "RSNumberFormatterMinusSignKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterMultiplierKey, "RSNumberFormatterMultiplierKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterNaNSymbolKey, "RSNumberFormatterNaNSymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterNegativePrefixKey, "RSNumberFormatterNegativePrefixKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterNegativeSuffixKey, "RSNumberFormatterNegativeSuffixKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterPaddingCharacterKey, "RSNumberFormatterPaddingCharacterKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterPaddingPositionKey, "RSNumberFormatterPaddingPositionKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterPerMillSymbolKey, "RSNumberFormatterPerMillSymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterPercentSymbolKey, "RSNumberFormatterPercentSymbolKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterPlusSignKey, "RSNumberFormatterPlusSignKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterPositivePrefixKey, "RSNumberFormatterPositivePrefixKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterPositiveSuffixKey, "RSNumberFormatterPositiveSuffixKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterRoundingIncrementKey, "RSNumberFormatterRoundingIncrementKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterRoundingModeKey, "RSNumberFormatterRoundingModeKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterSecondaryGroupingSizeKey, "RSNumberFormatterSecondaryGroupingSizeKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterUseGroupingSeparatorKey, "RSNumberFormatterUseGroupingSeparatorKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterUseSignificantDigitsKey, "RSNumberFormatterUseSignificantDigitsKey");
RS_PUBLIC_CONST_STRING_DECL(RSNumberFormatterZeroSymbolKey, "RSNumberFormatterZeroSymbolKey");

RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierGregorian, "gregorian");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierBuddhist, "buddhist");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierJapanese, "japanese");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierIslamic, "islamic");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierIslamicCivil, "islamic-civil");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierHebrew, "hebrew");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierChinese, "chinese");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierRepublicOfChina, "roc");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierPersian, "persian");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierIndian, "indian");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierISO8601, "");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierCoptic, "coptic");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierEthiopicAmeteMihret, "ethiopic");
RS_PUBLIC_CONST_STRING_DECL(RSCalendarIdentifierEthiopicAmeteAlem, "ethiopic-amete-alem");

// Aliases for Linux
#if DEPLOYMENT_TARGET_LINUX

RSExport RSStringRef const RSBuddhistCalendar __attribute__((alias ("RSCalendarIdentifierBuddhist")));
RSExport RSStringRef const RSChineseCalendar __attribute__((alias ("RSCalendarIdentifierChinese")));
RSExport RSStringRef const RSGregorianCalendar __attribute__((alias ("RSCalendarIdentifierGregorian")));
RSExport RSStringRef const RSHebrewCalendar __attribute__((alias ("RSCalendarIdentifierHebrew")));
RSExport RSStringRef const RSISO8601Calendar __attribute__((alias ("RSCalendarIdentifierISO8601")));
RSExport RSStringRef const RSIndianCalendar __attribute__((alias ("RSCalendarIdentifierIndian")));
RSExport RSStringRef const RSIslamicCalendar __attribute__((alias ("RSCalendarIdentifierIslamic")));
RSExport RSStringRef const RSIslamicCivilCalendar __attribute__((alias ("RSCalendarIdentifierIslamicCivil")));
RSExport RSStringRef const RSJapaneseCalendar __attribute__((alias ("RSCalendarIdentifierJapanese")));
RSExport RSStringRef const RSPersianCalendar __attribute__((alias ("RSCalendarIdentifierPersian")));
RSExport RSStringRef const RSRepublicOfChinaCalendar __attribute__((alias ("RSCalendarIdentifierRepublicOfChina")));


RSExport RSStringRef const RSLocaleCalendarIdentifier __attribute__((alias ("RSLocaleCalendarIdentifierKey")));
RSExport RSStringRef const RSLocaleCalendar __attribute__((alias ("RSLocaleCalendarKey")));
RSExport RSStringRef const RSLocaleCollationIdentifier __attribute__((alias ("RSLocaleCollationIdentifierKey")));
RSExport RSStringRef const RSLocaleCollatorIdentifier __attribute__((alias ("RSLocaleCollatorIdentifierKey")));
RSExport RSStringRef const RSLocaleCountryCode __attribute__((alias ("RSLocaleCountryCodeKey")));
RSExport RSStringRef const RSLocaleCurrencyCode __attribute__((alias ("RSLocaleCurrencyCodeKey")));
RSExport RSStringRef const RSLocaleCurrencySymbol __attribute__((alias ("RSLocaleCurrencySymbolKey")));
RSExport RSStringRef const RSLocaleDecimalSeparator __attribute__((alias ("RSLocaleDecimalSeparatorKey")));
RSExport RSStringRef const RSLocaleExemplarCharacterSet __attribute__((alias ("RSLocaleExemplarCharacterSetKey")));
RSExport RSStringRef const RSLocaleGroupingSeparator __attribute__((alias ("RSLocaleGroupingSeparatorKey")));
RSExport RSStringRef const RSLocaleIdentifier __attribute__((alias ("RSLocaleIdentifierKey")));
RSExport RSStringRef const RSLocaleLanguageCode __attribute__((alias ("RSLocaleLanguageCodeKey")));
RSExport RSStringRef const RSLocaleMeasurementSystem __attribute__((alias ("RSLocaleMeasurementSystemKey")));
RSExport RSStringRef const RSLocaleScriptCode __attribute__((alias ("RSLocaleScriptCodeKey")));
RSExport RSStringRef const RSLocaleUsesMetricSystem __attribute__((alias ("RSLocaleUsesMetricSystemKey")));
RSExport RSStringRef const RSLocaleVariantCode __attribute__((alias ("RSLocaleVariantCodeKey")));
RSExport RSStringRef const RSDateFormatterAMSymbol __attribute__((alias ("RSDateFormatterAMSymbolKey")));
RSExport RSStringRef const RSDateFormatterCalendar __attribute__((alias ("RSDateFormatterCalendarKey")));
RSExport RSStringRef const RSDateFormatterCalendarIdentifier __attribute__((alias ("RSDateFormatterCalendarIdentifierKey")));
RSExport RSStringRef const RSDateFormatterDefaultDate __attribute__((alias ("RSDateFormatterDefaultDateKey")));
RSExport RSStringRef const RSDateFormatterDefaultFormat __attribute__((alias ("RSDateFormatterDefaultFormatKey")));
RSExport RSStringRef const RSDateFormatterEraSymbols __attribute__((alias ("RSDateFormatterEraSymbolsKey")));
RSExport RSStringRef const RSDateFormatterGregorianStartDate __attribute__((alias ("RSDateFormatterGregorianStartDateKey")));
RSExport RSStringRef const RSDateFormatterIsLenient __attribute__((alias ("RSDateFormatterIsLenientKey")));
RSExport RSStringRef const RSDateFormatterLongEraSymbols __attribute__((alias ("RSDateFormatterLongEraSymbolsKey")));
RSExport RSStringRef const RSDateFormatterMonthSymbols __attribute__((alias ("RSDateFormatterMonthSymbolsKey")));
RSExport RSStringRef const RSDateFormatterPMSymbol __attribute__((alias ("RSDateFormatterPMSymbolKey")));
RSExport RSStringRef const RSDateFormatterQuarterSymbols __attribute__((alias ("RSDateFormatterQuarterSymbolsKey")));
RSExport RSStringRef const RSDateFormatterShortMonthSymbols __attribute__((alias ("RSDateFormatterShortMonthSymbolsKey")));
RSExport RSStringRef const RSDateFormatterShortQuarterSymbols __attribute__((alias ("RSDateFormatterShortQuarterSymbolsKey")));
RSExport RSStringRef const RSDateFormatterShortStandaloneMonthSymbols __attribute__((alias ("RSDateFormatterShortStandaloneMonthSymbolsKey")));
RSExport RSStringRef const RSDateFormatterShortStandaloneQuarterSymbols __attribute__((alias ("RSDateFormatterShortStandaloneQuarterSymbolsKey")));
RSExport RSStringRef const RSDateFormatterShortStandaloneWeekdaySymbols __attribute__((alias ("RSDateFormatterShortStandaloneWeekdaySymbolsKey")));
RSExport RSStringRef const RSDateFormatterShortWeekdaySymbols __attribute__((alias ("RSDateFormatterShortWeekdaySymbolsKey")));
RSExport RSStringRef const RSDateFormatterStandaloneMonthSymbols __attribute__((alias ("RSDateFormatterStandaloneMonthSymbolsKey")));
RSExport RSStringRef const RSDateFormatterStandaloneQuarterSymbols __attribute__((alias ("RSDateFormatterStandaloneQuarterSymbolsKey")));
RSExport RSStringRef const RSDateFormatterStandaloneWeekdaySymbols __attribute__((alias ("RSDateFormatterStandaloneWeekdaySymbolsKey")));
RSExport RSStringRef const RSDateFormatterTimeZone __attribute__((alias ("RSDateFormatterTimeZoneKey")));
RSExport RSStringRef const RSDateFormatterTwoDigitStartDate __attribute__((alias ("RSDateFormatterTwoDigitStartDateKey")));
RSExport RSStringRef const RSDateFormatterVeryShortMonthSymbols __attribute__((alias ("RSDateFormatterVeryShortMonthSymbolsKey")));
RSExport RSStringRef const RSDateFormatterVeryShortStandaloneMonthSymbols __attribute__((alias ("RSDateFormatterVeryShortStandaloneMonthSymbolsKey")));
RSExport RSStringRef const RSDateFormatterVeryShortStandaloneWeekdaySymbols __attribute__((alias ("RSDateFormatterVeryShortStandaloneWeekdaySymbolsKey")));
RSExport RSStringRef const RSDateFormatterVeryShortWeekdaySymbols __attribute__((alias ("RSDateFormatterVeryShortWeekdaySymbolsKey")));
RSExport RSStringRef const RSDateFormatterWeekdaySymbols __attribute__((alias ("RSDateFormatterWeekdaySymbolsKey")));
RSExport RSStringRef const RSNumberFormatterAlwaysShowDecimalSeparator __attribute__((alias ("RSNumberFormatterAlwaysShowDecimalSeparatorKey")));
RSExport RSStringRef const RSNumberFormatterCurrencyCode __attribute__((alias ("RSNumberFormatterCurrencyCodeKey")));
RSExport RSStringRef const RSNumberFormatterCurrencyDecimalSeparator __attribute__((alias ("RSNumberFormatterCurrencyDecimalSeparatorKey")));
RSExport RSStringRef const RSNumberFormatterCurrencyGroupingSeparator __attribute__((alias ("RSNumberFormatterCurrencyGroupingSeparatorKey")));
RSExport RSStringRef const RSNumberFormatterCurrencySymbol __attribute__((alias ("RSNumberFormatterCurrencySymbolKey")));
RSExport RSStringRef const RSNumberFormatterDecimalSeparator __attribute__((alias ("RSNumberFormatterDecimalSeparatorKey")));
RSExport RSStringRef const RSNumberFormatterDefaultFormat __attribute__((alias ("RSNumberFormatterDefaultFormatKey")));
RSExport RSStringRef const RSNumberFormatterExponentSymbol __attribute__((alias ("RSNumberFormatterExponentSymbolKey")));
RSExport RSStringRef const RSNumberFormatterFormatWidth __attribute__((alias ("RSNumberFormatterFormatWidthKey")));
RSExport RSStringRef const RSNumberFormatterGroupingSeparator __attribute__((alias ("RSNumberFormatterGroupingSeparatorKey")));
RSExport RSStringRef const RSNumberFormatterGroupingSize __attribute__((alias ("RSNumberFormatterGroupingSizeKey")));
RSExport RSStringRef const RSNumberFormatterInfinitySymbol __attribute__((alias ("RSNumberFormatterInfinitySymbolKey")));
RSExport RSStringRef const RSNumberFormatterInternationalCurrencySymbol __attribute__((alias ("RSNumberFormatterInternationalCurrencySymbolKey")));
RSExport RSStringRef const RSNumberFormatterIsLenient __attribute__((alias ("RSNumberFormatterIsLenientKey")));
RSExport RSStringRef const RSNumberFormatterMaxFractionDigits __attribute__((alias ("RSNumberFormatterMaxFractionDigitsKey")));
RSExport RSStringRef const RSNumberFormatterMaxIntegerDigits __attribute__((alias ("RSNumberFormatterMaxIntegerDigitsKey")));
RSExport RSStringRef const RSNumberFormatterMaxSignificantDigits __attribute__((alias ("RSNumberFormatterMaxSignificantDigitsKey")));
RSExport RSStringRef const RSNumberFormatterMinFractionDigits __attribute__((alias ("RSNumberFormatterMinFractionDigitsKey")));
RSExport RSStringRef const RSNumberFormatterMinIntegerDigits __attribute__((alias ("RSNumberFormatterMinIntegerDigitsKey")));
RSExport RSStringRef const RSNumberFormatterMinSignificantDigits __attribute__((alias ("RSNumberFormatterMinSignificantDigitsKey")));
RSExport RSStringRef const RSNumberFormatterMinusSign __attribute__((alias ("RSNumberFormatterMinusSignKey")));
RSExport RSStringRef const RSNumberFormatterMultiplier __attribute__((alias ("RSNumberFormatterMultiplierKey")));
RSExport RSStringRef const RSNumberFormatterNaNSymbol __attribute__((alias ("RSNumberFormatterNaNSymbolKey")));
RSExport RSStringRef const RSNumberFormatterNegativePrefix __attribute__((alias ("RSNumberFormatterNegativePrefixKey")));
RSExport RSStringRef const RSNumberFormatterNegativeSuffix __attribute__((alias ("RSNumberFormatterNegativeSuffixKey")));
RSExport RSStringRef const RSNumberFormatterPaddingCharacter __attribute__((alias ("RSNumberFormatterPaddingCharacterKey")));
RSExport RSStringRef const RSNumberFormatterPaddingPosition __attribute__((alias ("RSNumberFormatterPaddingPositionKey")));
RSExport RSStringRef const RSNumberFormatterPerMillSymbol __attribute__((alias ("RSNumberFormatterPerMillSymbolKey")));
RSExport RSStringRef const RSNumberFormatterPercentSymbol __attribute__((alias ("RSNumberFormatterPercentSymbolKey")));
RSExport RSStringRef const RSNumberFormatterPlusSign __attribute__((alias ("RSNumberFormatterPlusSignKey")));
RSExport RSStringRef const RSNumberFormatterPositivePrefix __attribute__((alias ("RSNumberFormatterPositivePrefixKey")));
RSExport RSStringRef const RSNumberFormatterPositiveSuffix __attribute__((alias ("RSNumberFormatterPositiveSuffixKey")));
RSExport RSStringRef const RSNumberFormatterRoundingIncrement __attribute__((alias ("RSNumberFormatterRoundingIncrementKey")));
RSExport RSStringRef const RSNumberFormatterRoundingMode __attribute__((alias ("RSNumberFormatterRoundingModeKey")));
RSExport RSStringRef const RSNumberFormatterSecondaryGroupingSize __attribute__((alias ("RSNumberFormatterSecondaryGroupingSizeKey")));
RSExport RSStringRef const RSNumberFormatterUseGroupingSeparator __attribute__((alias ("RSNumberFormatterUseGroupingSeparatorKey")));
RSExport RSStringRef const RSNumberFormatterUseSignificantDigits __attribute__((alias ("RSNumberFormatterUseSignificantDigitsKey")));
RSExport RSStringRef const RSNumberFormatterZeroSymbol __attribute__((alias ("RSNumberFormatterZeroSymbolKey")));
RSExport RSStringRef const RSDateFormatterCalendarName __attribute__((alias ("RSDateFormatterCalendarIdentifierKey")));

#endif


// Note string members start with an extra &, and are fixed up at init time
static struct key_table __RSLocaleKeyTable[__RSLocaleKeyTableCount] = {
    {(RSStringRef)&RSLocaleIdentifierKey, __RSLocaleCopyLocaleID, __RSLocaleSetNOP, __RSLocaleFullName, NULL},
    {(RSStringRef)&RSLocaleLanguageCodeKey, __RSLocaleCopyCodes, __RSLocaleSetNOP, __RSLocaleLanguageName, (RSStringRef)&RSLocaleLanguageCodeKey},
    {(RSStringRef)&RSLocaleCountryCodeKey, __RSLocaleCopyCodes, __RSLocaleSetNOP, __RSLocaleCountryName, (RSStringRef)&RSLocaleCountryCodeKey},
    {(RSStringRef)&RSLocaleScriptCodeKey, __RSLocaleCopyCodes, __RSLocaleSetNOP, __RSLocaleScriptName, (RSStringRef)&RSLocaleScriptCodeKey},
    {(RSStringRef)&RSLocaleVariantCodeKey, __RSLocaleCopyCodes, __RSLocaleSetNOP, __RSLocaleVariantName, (RSStringRef)&RSLocaleVariantCodeKey},
    {(RSStringRef)&RSLocaleExemplarCharacterSetKey, __RSLocaleCopyExemplarCharSet, __RSLocaleSetNOP, __RSLocaleNoName, NULL},
    {(RSStringRef)&RSLocaleCalendarIdentifierKey, __RSLocaleCopyCalendarID, __RSLocaleSetNOP, __RSLocaleCalendarName, NULL},
    {(RSStringRef)&RSLocaleCalendarKey, __RSLocaleCopyCalendar, __RSLocaleSetNOP, __RSLocaleNoName, NULL},
    {(RSStringRef)&RSLocaleCollationIdentifierKey, __RSLocaleCopyCollationID, __RSLocaleSetNOP, __RSLocaleCollationName, NULL},
    {(RSStringRef)&RSLocaleUsesMetricSystemKey, __RSLocaleCopyUsesMetric, __RSLocaleSetNOP, __RSLocaleNoName, NULL},
    {(RSStringRef)&RSLocaleMeasurementSystemKey, __RSLocaleCopyMeasurementSystem, __RSLocaleSetNOP, __RSLocaleNoName, NULL},
    {(RSStringRef)&RSLocaleDecimalSeparatorKey, __RSLocaleCopyNumberFormat, __RSLocaleSetNOP, __RSLocaleNoName, (RSStringRef)&RSNumberFormatterDecimalSeparatorKey},
    {(RSStringRef)&RSLocaleGroupingSeparatorKey, __RSLocaleCopyNumberFormat, __RSLocaleSetNOP, __RSLocaleNoName, (RSStringRef)&RSNumberFormatterGroupingSeparatorKey},
    {(RSStringRef)&RSLocaleCurrencySymbolKey, __RSLocaleCopyNumberFormat2, __RSLocaleSetNOP, __RSLocaleCurrencyShortName, (RSStringRef)&RSNumberFormatterCurrencySymbolKey},
    {(RSStringRef)&RSLocaleCurrencyCodeKey, __RSLocaleCopyNumberFormat2, __RSLocaleSetNOP, __RSLocaleCurrencyFullName, (RSStringRef)&RSNumberFormatterCurrencyCodeKey},
    {(RSStringRef)&RSLocaleCollatorIdentifierKey, __RSLocaleCopyCollatorID, __RSLocaleSetNOP, __RSLocaleNoName, NULL},
    {(RSStringRef)&__RSLocaleCollatorID, __RSLocaleCopyCollatorID, __RSLocaleSetNOP, __RSLocaleNoName, NULL},
    {(RSStringRef)&RSLocaleQuotationBeginDelimiterKey, __RSLocaleCopyDelimiter, __RSLocaleSetNOP, __RSLocaleNoName, (RSStringRef)&RSLocaleQuotationBeginDelimiterKey},
    {(RSStringRef)&RSLocaleQuotationEndDelimiterKey, __RSLocaleCopyDelimiter, __RSLocaleSetNOP, __RSLocaleNoName, (RSStringRef)&RSLocaleQuotationEndDelimiterKey},
    {(RSStringRef)&RSLocaleAlternateQuotationBeginDelimiterKey, __RSLocaleCopyDelimiter, __RSLocaleSetNOP, __RSLocaleNoName, (RSStringRef)&RSLocaleAlternateQuotationBeginDelimiterKey},
    {(RSStringRef)&RSLocaleAlternateQuotationEndDelimiterKey, __RSLocaleCopyDelimiter, __RSLocaleSetNOP, __RSLocaleNoName, (RSStringRef)&RSLocaleAlternateQuotationEndDelimiterKey},
};

enum {
    kLocaleIdentifierCStringMax = ULOC_FULLNAME_CAPACITY + ULOC_KEYWORD_AND_VALUES_CAPACITY	// currently 56 + 100
};

// KeyStringToResultString struct used in data tables for RSLocaleCreateCanonicalLocaleIdentifierFromString
struct KeyStringToResultString {
    const char *    key;
    const char *    result;
};
typedef struct KeyStringToResultString KeyStringToResultString;

// SpecialCaseUpdates struct used in data tables for RSLocaleCreateCanonicalLocaleIdentifierFromString
struct SpecialCaseUpdates {
    const char *    lang;
    const char *    reg1;
    const char *    update1;
    const char *    reg2;
    const char *    update2;
};
typedef struct SpecialCaseUpdates SpecialCaseUpdates;


static const char * const regionCodeToLocaleString[] = {
    // map RegionCode (array index) to canonical locale string
    //
    //  canon. string      region code;             language code;      [comment]       [   # __RSBundleLocaleAbbreviationsArray
    //  --------           ------------             ------------------  ------------        --------     string, if different ]
    "en_US",        //   0 verUS;                 0 langEnglish;
    "fr_FR",        //   1 verFrance;             1 langFrench;
    "en_GB",        //   2 verBritain;            0 langEnglish;
    "de_DE",        //   3 verGermany;            2 langGerman;
    "it_IT",        //   4 verItaly;              3 langItalian;
    "nl_NL",        //   5 verNetherlands;        4 langDutch;
    "nl_BE",        //   6 verFlemish;           34 langFlemish (redundant, =Dutch);
    "sv_SE",        //   7 verSweden;             5 langSwedish;
    "es_ES",        //   8 verSpain;              6 langSpanish;
    "da_DK",        //   9 verDenmark;            7 langDanish;
    "pt_PT",        //  10 verPortugal;           8 langPortuguese;
    "fr_CA",        //  11 verFrCanada;           1 langFrench;
    "nb_NO",        //  12 verNorway;             9 langNorwegian (Bokmal);             # "no_NO"
    "he_IL",        //  13 verIsrael;            10 langHebrew;
    "ja_JP",        //  14 verJapan;             11 langJapanese;
    "en_AU",        //  15 verAustralia;          0 langEnglish;
    "ar",           //  16 verArabic;            12 langArabic;
    "fi_FI",        //  17 verFinland;           13 langFinnish;
    "fr_CH",        //  18 verFrSwiss;            1 langFrench;
    "de_CH",        //  19 verGrSwiss;            2 langGerman;
    "el_GR",        //  20 verGreece;            14 langGreek (modern)-Grek-mono;
    "is_IS",        //  21 verIceland;           15 langIcelandic;
    "mt_MT",        //  22 verMalta;             16 langMaltese;
    "el_CY",        //  23 verCyprus;            14 langGreek?;     el or tr? guess el  # ""
    "tr_TR",        //  24 verTurkey;            17 langTurkish;
    "hr_HR",        //  25 verYugoCroatian;      18 langCroatian;   * one-way mapping -> verCroatia
    "nl_NL",        //  26 KCHR, Netherlands;     4 langDutch;      * one-way mapping
    "nl_BE",        //  27 KCHR, verFlemish;     34 langFlemish;    * one-way mapping
    "_CA",          //  28 KCHR, Canada-en/fr?;  -1 none;           * one-way mapping   # "en_CA"
    "_CA",          //  29 KCHR, Canada-en/fr?;  -1 none;           * one-way mapping   # "en_CA"
    "pt_PT",        //  30 KCHR, Portugal;        8 langPortuguese; * one-way mapping
    "nb_NO",        //  31 KCHR, Norway;          9 langNorwegian (Bokmal); * one-way mapping   # "no_NO"
    "da_DK",        //  32 KCHR, Denmark;         7 langDanish;     * one-way mapping
    "hi_IN",        //  33 verIndiaHindi;        21 langHindi;
    "ur_PK",        //  34 verPakistanUrdu;      20 langUrdu;
    "tr_TR",        //  35 verTurkishModified;   17 langTurkish;    * one-way mapping
    "it_CH",        //  36 verItalianSwiss;       3 langItalian;
    "en_001",       //  37 verInternational;      0 langEnglish; ASCII only             # "en"
    NULL,           //  38 *unassigned;          -1 none;           * one-way mapping   # ""
    "ro_RO",        //  39 verRomania;           37 langRomanian;
    "grc",          //  40 verGreekAncient;     148 langGreekAncient -Grek-poly;        # "el_GR"
    "lt_LT",        //  41 verLithuania;         24 langLithuanian;
    "pl_PL",        //  42 verPoland;            25 langPolish;
    "hu_HU",        //  43 verHungary;           26 langHungarian;
    "et_EE",        //  44 verEstonia;           27 langEstonian;
    "lv_LV",        //  45 verLatvia;            28 langLatvian;
    "se",           //  46 verSami;              29 langSami;
    "fo_FO",        //  47 verFaroeIsl;          30 langFaroese;
    "fa_IR",        //  48 verIran;              31 langFarsi/Persian;
    "ru_RU",        //  49 verRussia;            32 langRussian;
    "ga_IE",        //  50 verIreland;           35 langIrishGaelic (no dots);
    "ko_KR",        //  51 verKorea;             23 langKorean;
    "zh_CN",        //  52 verChina;             33 langSimpChinese;
    "zh_TW",        //  53 verTaiwan;            19 langTradChinese;
    "th_TH",        //  54 verThailand;          22 langThai;
    "und",          //  55 verScriptGeneric;     -1 none;                               # ""        // <1.9>
    "cs_CZ",        //  56 verCzech;             38 langCzech;
    "sk_SK",        //  57 verSlovak;            39 langSlovak;
    "und",          //  58 verEastAsiaGeneric;   -1 none;           * one-way mapping   # ""        // <1.9>
    "hu_HU",        //  59 verMagyar;            26 langHungarian;  * one-way mapping -> verHungary
    "bn",           //  60 verBengali;           67 langBengali;    _IN or _BD? guess generic
    "be_BY",        //  61 verBelarus;           46 langBelorussian;
    "uk_UA",        //  62 verUkraine;           45 langUkrainian;
    NULL,           //  63 *unused;              -1 none;           * one-way mapping   # ""
    "el_GR",        //  64 verGreeceAlt;         14 langGreek (modern)-Grek-mono;   * one-way mapping
    "sr_RS",        //  65 verSerbian;           42 langSerbian -Cyrl;								// <1.18>
    "sl_SI",        //  66 verSlovenian;         40 langSlovenian;
    "mk_MK",        //  67 verMacedonian;        43 langMacedonian;
    "hr_HR",        //  68 verCroatia;           18 langCroatian;
    NULL,           //  69 *unused;              -1 none;           * one-way mapping   # ""
    "de-1996",      //  70 verGermanReformed;     2 langGerman;     1996 orthogr.       # "de_DE"
    "pt_BR",        //  71 verBrazil;             8 langPortuguese;
    "bg_BG",        //  72 verBulgaria;          44 langBulgarian;
    "ca_ES",        //  73 verCatalonia;        130 langCatalan;
    "mul",          //  74 verMultilingual;      -1 none;                               # ""
    "gd",           //  75 verScottishGaelic;   144 langScottishGaelic;
    "gv",           //  76 verManxGaelic;       145 langManxGaelic;
    "br",           //  77 verBreton;           142 langBreton;
    "iu_CA",        //  78 verNunavut;          143 langInuktitut -Cans;
    "cy",           //  79 verWelsh;            128 langWelsh;
    "_CA",          //  80 KCHR, Canada-en/fr?;  -1 none;           * one-way mapping   # "en_CA"
    "ga-Latg_IE",   //  81 verIrishGaelicScrip; 146 langIrishGaelicScript -dots;        # "ga_IE"   // <xx>
    "en_CA",        //  82 verEngCanada;          0 langEnglish;
    "dz_BT",        //  83 verBhutan;           137 langDzongkha;
    "hy_AM",        //  84 verArmenian;          51 langArmenian;
    "ka_GE",        //  85 verGeorgian;          52 langGeorgian;
    "es_419",       //  86 verSpLatinAmerica;     6 langSpanish;                        # "es"
    "es_ES",        //  87 KCHR, Spain;           6 langSpanish;    * one-way mapping
    "to_TO",        //  88 verTonga;            147 langTongan;
    "pl_PL",        //  89 KCHR, Poland;         25 langPolish;     * one-way mapping
    "ca_ES",        //  90 KCHR, Catalonia;     130 langCatalan;    * one-way mapping
    "fr_001",       //  91 verFrenchUniversal;    1 langFrench;
    "de_AT",        //  92 verAustria;            2 langGerman;
    "es_419",       //  93 > verSpLatinAmerica;   6 langSpanish;    * one-way mapping   # "es"
    "gu_IN",        //  94 verGujarati;          69 langGujarati;
    "pa",           //  95 verPunjabi;           70 langPunjabi;    _IN or _PK? guess generic
    "ur_IN",        //  96 verIndiaUrdu;         20 langUrdu;
    "vi_VN",        //  97 verVietnam;           80 langVietnamese;
    "fr_BE",        //  98 verFrBelgium;          1 langFrench;
    "uz_UZ",        //  99 verUzbek;             47 langUzbek;
    "en_SG",        // 100 verSingapore;          0 langEnglish?; en, zh, or ms? guess en   # ""
    "nn_NO",        // 101 verNynorsk;          151 langNynorsk;                        # ""
    "af_ZA",        // 102 verAfrikaans;        141 langAfrikaans;
    "eo",           // 103 verEsperanto;         94 langEsperanto;
    "mr_IN",        // 104 verMarathi;           66 langMarathi;
    "bo",           // 105 verTibetan;           63 langTibetan;
    "ne_NP",        // 106 verNepal;             64 langNepali;
    "kl",           // 107 verGreenland;        149 langGreenlandic;
    "en_IE",        // 108 verIrelandEnglish;     0 langEnglish;                        # (no entry)
};
enum {
    kNumRegionCodeToLocaleString = sizeof(regionCodeToLocaleString)/sizeof(char *)
};

static const char * const langCodeToLocaleString[] = {
    // map LangCode (array index) to canonical locale string
    //
    //  canon. string   language code;                  [ comment]  [   # __RSBundleLanguageAbbreviationsArray
    //  --------        --------------                  ----------      --------    string, if different ]
    "en",       //   0 langEnglish;
    "fr",       //   1 langFrench;
    "de",       //   2 langGerman;
    "it",       //   3 langItalian;
    "nl",       //   4 langDutch;
    "sv",       //   5 langSwedish;
    "es",       //   6 langSpanish;
    "da",       //   7 langDanish;
    "pt",       //   8 langPortuguese;
    "nb",       //   9 langNorwegian (Bokmal);                      # "no"
    "he",       //  10 langHebrew -Hebr;
    "ja",       //  11 langJapanese -Jpan;
    "ar",       //  12 langArabic -Arab;
    "fi",       //  13 langFinnish;
    "el",       //  14 langGreek (modern)-Grek-mono;
    "is",       //  15 langIcelandic;
    "mt",       //  16 langMaltese -Latn;
    "tr",       //  17 langTurkish -Latn;
    "hr",       //  18 langCroatian;
    "zh-Hant",  //  19 langTradChinese;                             # "zh"
    "ur",       //  20 langUrdu -Arab;
    "hi",       //  21 langHindi -Deva;
    "th",       //  22 langThai -Thai;
    "ko",       //  23 langKorean -Hang;
    "lt",       //  24 langLithuanian;
    "pl",       //  25 langPolish;
    "hu",       //  26 langHungarian;
    "et",       //  27 langEstonian;
    "lv",       //  28 langLatvian;
    "se",       //  29 langSami;
    "fo",       //  30 langFaroese;
    "fa",       //  31 langFarsi/Persian -Arab;
    "ru",       //  32 langRussian -Cyrl;
    "zh-Hans",  //  33 langSimpChinese;                             # "zh"
    "nl-BE",    //  34 langFlemish (redundant, =Dutch);             # "nl"
    "ga",       //  35 langIrishGaelic (no dots);
    "sq",       //  36 langAlbanian;                no region codes
    "ro",       //  37 langRomanian;
    "cs",       //  38 langCzech;
    "sk",       //  39 langSlovak;
    "sl",       //  40 langSlovenian;
    "yi",       //  41 langYiddish -Hebr;           no region codes
    "sr",       //  42 langSerbian -Cyrl;
    "mk",       //  43 langMacedonian -Cyrl;
    "bg",       //  44 langBulgarian -Cyrl;
    "uk",       //  45 langUkrainian -Cyrl;
    "be",       //  46 langBelorussian -Cyrl;
    "uz-Cyrl",  //  47 langUzbek -Cyrl;             also -Latn, -Arab
    "kk",       //  48 langKazakh -Cyrl;            no region codes; also -Latn, -Arab
    "az-Cyrl",  //  49 langAzerbaijani -Cyrl;       no region codes # "az"
    "az-Arab",  //  50 langAzerbaijanAr -Arab;      no region codes # "az"
    "hy",       //  51 langArmenian -Armn;
    "ka",       //  52 langGeorgian -Geor;
    "mo",       //  53 langMoldavian -Cyrl;         no region codes
    "ky",       //  54 langKirghiz -Cyrl;           no region codes; also -Latn, -Arab
    "tg-Cyrl",  //  55 langTajiki -Cyrl;            no region codes; also -Latn, -Arab
    "tk-Cyrl",  //  56 langTurkmen -Cyrl;           no region codes; also -Latn, -Arab
    "mn-Mong",  //  57 langMongolian -Mong;         no region codes # "mn"
    "mn-Cyrl",  //  58 langMongolianCyr -Cyrl;      no region codes # "mn"
    "ps",       //  59 langPashto -Arab;            no region codes
    "ku",       //  60 langKurdish -Arab;           no region codes
    "ks",       //  61 langKashmiri -Arab;          no region codes
    "sd",       //  62 langSindhi -Arab;            no region codes
    "bo",       //  63 langTibetan -Tibt;
    "ne",       //  64 langNepali -Deva;
    "sa",       //  65 langSanskrit -Deva;          no region codes
    "mr",       //  66 langMarathi -Deva;
    "bn",       //  67 langBengali -Beng;
    "as",       //  68 langAssamese -Beng;          no region codes
    "gu",       //  69 langGujarati -Gujr;
    "pa",       //  70 langPunjabi -Guru;
    "or",       //  71 langOriya -Orya;             no region codes
    "ml",       //  72 langMalayalam -Mlym;         no region codes
    "kn",       //  73 langKannada -Knda;           no region codes
    "ta",       //  74 langTamil -Taml;             no region codes
    "te",       //  75 langTelugu -Telu;            no region codes
    "si",       //  76 langSinhalese -Sinh;         no region codes
    "my",       //  77 langBurmese -Mymr;           no region codes
    "km",       //  78 langKhmer -Khmr;             no region codes
    "lo",       //  79 langLao -Laoo;               no region codes
    "vi",       //  80 langVietnamese -Latn;
    "id",       //  81 langIndonesian -Latn;        no region codes
    "tl",       //  82 langTagalog -Latn;           no region codes
    "ms",       //  83 langMalayRoman -Latn;        no region codes # "ms"
    "ms-Arab",  //  84 langMalayArabic -Arab;       no region codes # "ms"
    "am",       //  85 langAmharic -Ethi;           no region codes
    "ti",       //  86 langTigrinya -Ethi;          no region codes
    "om",       //  87 langOromo -Ethi;             no region codes
    "so",       //  88 langSomali -Latn;            no region codes
    "sw",       //  89 langSwahili -Latn;           no region codes
    "rw",       //  90 langKinyarwanda -Latn;       no region codes
    "rn",       //  91 langRundi -Latn;             no region codes
    "ny",       //  92 langNyanja/Chewa -Latn;      no region codes # ""
    "mg",       //  93 langMalagasy -Latn;          no region codes
    "eo",       //  94 langEsperanto -Latn;
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, //  95 to 105 (gap)
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, // 106 to 116 (gap)
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, // 107 to 117 (gap)
    "cy",       // 128 langWelsh -Latn;
    "eu",       // 129 langBasque -Latn;            no region codes
    "ca",       // 130 langCatalan -Latn;
    "la",       // 131 langLatin -Latn;             no region codes
    "qu",       // 132 langQuechua -Latn;           no region codes
    "gn",       // 133 langGuarani -Latn;           no region codes
    "ay",       // 134 langAymara -Latn;            no region codes
    "tt-Cyrl",  // 135 langTatar -Cyrl;             no region codes
    "ug",       // 136 langUighur -Arab;            no region codes
    "dz",       // 137 langDzongkha -Tibt;
    "jv",       // 138 langJavaneseRom -Latn;       no region codes
    "su",       // 139 langSundaneseRom -Latn;      no region codes
    "gl",       // 140 langGalician -Latn;          no region codes
    "af",       // 141 langAfrikaans -Latn;
    "br",       // 142 langBreton -Latn;
    "iu",       // 143 langInuktitut -Cans;
    "gd",       // 144 langScottishGaelic;
    "gv",       // 145 langManxGaelic -Latn;
    "ga-Latg",  // 146 langIrishGaelicScript  -Latn-dots;           # "ga"                                      // <xx>
    "to",       // 147 langTongan -Latn;
    "grc",      // 148 langGreekAncient -Grek-poly;                 # "el"
    "kl",       // 149 langGreenlandic -Latn;
    "az-Latn",  // 150 langAzerbaijanRoman -Latn;   no region codes # "az"
    "nn",       // 151 langNynorsk -Latn;                           # (no entry)
};
enum {
    kNumLangCodeToLocaleString = sizeof(langCodeToLocaleString)/sizeof(char *)
};

static const KeyStringToResultString oldAppleLocaleToCanonical[] = {
    // Map obsolete/old-style Apple strings to canonical
    // Must be sorted according to how strcmp compares the strings in the first column
    //
    //    non-canonical             canonical       [  comment ]            # source/reason for non-canonical string
    //    string                    string
    //    -------------             ---------
    { "Afrikaans",              "af"        },  //                      # __RSBundleLanguageNamesArray
    { "Albanian",               "sq"        },  //                      # __RSBundleLanguageNamesArray
    { "Amharic",                "am"        },  //                      # __RSBundleLanguageNamesArray
    { "Arabic",                 "ar"        },  //                      # __RSBundleLanguageNamesArray
    { "Armenian",               "hy"        },  //                      # __RSBundleLanguageNamesArray
    { "Assamese",               "as"        },  //                      # __RSBundleLanguageNamesArray
    { "Aymara",                 "ay"        },  //                      # __RSBundleLanguageNamesArray
    { "Azerbaijani",            "az"        },  // -Arab,-Cyrl,-Latn?   # __RSBundleLanguageNamesArray (had 3 entries "Azerbaijani" for "az-Arab", "az-Cyrl", "az-Latn")
    { "Basque",                 "eu"        },  //                      # __RSBundleLanguageNamesArray
    { "Belarusian",             "be"        },  //                      # handle other names
    { "Belorussian",            "be"        },  //                      # handle other names
    { "Bengali",                "bn"        },  //                      # __RSBundleLanguageNamesArray
    { "Brazilian Portugese",    "pt-BR"     },  //                      # from Installer.app Info.plist IFLanguages key, misspelled
    { "Brazilian Portuguese",   "pt-BR"     },  //                      # correct spelling for above
    { "Breton",                 "br"        },  //                      # __RSBundleLanguageNamesArray
    { "Bulgarian",              "bg"        },  //                      # __RSBundleLanguageNamesArray
    { "Burmese",                "my"        },  //                      # __RSBundleLanguageNamesArray
    { "Byelorussian",           "be"        },  //                      # __RSBundleLanguageNamesArray
    { "Catalan",                "ca"        },  //                      # __RSBundleLanguageNamesArray
    { "Chewa",                  "ny"        },  //                      # handle other names
    { "Chichewa",               "ny"        },  //                      # handle other names
    { "Chinese",                "zh"        },  // -Hans,-Hant?         # __RSBundleLanguageNamesArray (had 2 entries "Chinese" for "zh-Hant", "zh-Hans")
    { "Chinese, Simplified",    "zh-Hans"   },  //                      # from Installer.app Info.plist IFLanguages key
    { "Chinese, Traditional",   "zh-Hant"   },  //                      # correct spelling for below
    { "Chinese, Tradtional",    "zh-Hant"   },  //                      # from Installer.app Info.plist IFLanguages key, misspelled
    { "Croatian",               "hr"        },  //                      # __RSBundleLanguageNamesArray
    { "Czech",                  "cs"        },  //                      # __RSBundleLanguageNamesArray
    { "Danish",                 "da"        },  //                      # __RSBundleLanguageNamesArray
    { "Dutch",                  "nl"        },  //                      # __RSBundleLanguageNamesArray (had 2 entries "Dutch" for "nl", "nl-BE")
    { "Dzongkha",               "dz"        },  //                      # __RSBundleLanguageNamesArray
    { "English",                "en"        },  //                      # __RSBundleLanguageNamesArray
    { "Esperanto",              "eo"        },  //                      # __RSBundleLanguageNamesArray
    { "Estonian",               "et"        },  //                      # __RSBundleLanguageNamesArray
    { "Faroese",                "fo"        },  //                      # __RSBundleLanguageNamesArray
    { "Farsi",                  "fa"        },  //                      # __RSBundleLanguageNamesArray
    { "Finnish",                "fi"        },  //                      # __RSBundleLanguageNamesArray
    { "Flemish",                "nl-BE"     },  //                      # handle other names
    { "French",                 "fr"        },  //                      # __RSBundleLanguageNamesArray
    { "Galician",               "gl"        },  //                      # __RSBundleLanguageNamesArray
    { "Gallegan",               "gl"        },  //                      # handle other names
    { "Georgian",               "ka"        },  //                      # __RSBundleLanguageNamesArray
    { "German",                 "de"        },  //                      # __RSBundleLanguageNamesArray
    { "Greek",                  "el"        },  //                      # __RSBundleLanguageNamesArray (had 2 entries "Greek" for "el", "grc")
    { "Greenlandic",            "kl"        },  //                      # __RSBundleLanguageNamesArray
    { "Guarani",                "gn"        },  //                      # __RSBundleLanguageNamesArray
    { "Gujarati",               "gu"        },  //                      # __RSBundleLanguageNamesArray
    { "Hawaiian",               "haw"       },  //                      # handle new languages
    { "Hebrew",                 "he"        },  //                      # __RSBundleLanguageNamesArray
    { "Hindi",                  "hi"        },  //                      # __RSBundleLanguageNamesArray
    { "Hungarian",              "hu"        },  //                      # __RSBundleLanguageNamesArray
    { "Icelandic",              "is"        },  //                      # __RSBundleLanguageNamesArray
    { "Indonesian",             "id"        },  //                      # __RSBundleLanguageNamesArray
    { "Inuktitut",              "iu"        },  //                      # __RSBundleLanguageNamesArray
    { "Irish",                  "ga"        },  //                      # __RSBundleLanguageNamesArray (had 2 entries "Irish" for "ga", "ga-dots")
    { "Italian",                "it"        },  //                      # __RSBundleLanguageNamesArray
    { "Japanese",               "ja"        },  //                      # __RSBundleLanguageNamesArray
    { "Javanese",               "jv"        },  //                      # __RSBundleLanguageNamesArray
    { "Kalaallisut",            "kl"        },  //                      # handle other names
    { "Kannada",                "kn"        },  //                      # __RSBundleLanguageNamesArray
    { "Kashmiri",               "ks"        },  //                      # __RSBundleLanguageNamesArray
    { "Kazakh",                 "kk"        },  //                      # __RSBundleLanguageNamesArray
    { "Khmer",                  "km"        },  //                      # __RSBundleLanguageNamesArray
    { "Kinyarwanda",            "rw"        },  //                      # __RSBundleLanguageNamesArray
    { "Kirghiz",                "ky"        },  //                      # __RSBundleLanguageNamesArray
    { "Korean",                 "ko"        },  //                      # __RSBundleLanguageNamesArray
    { "Kurdish",                "ku"        },  //                      # __RSBundleLanguageNamesArray
    { "Lao",                    "lo"        },  //                      # __RSBundleLanguageNamesArray
    { "Latin",                  "la"        },  //                      # __RSBundleLanguageNamesArray
    { "Latvian",                "lv"        },  //                      # __RSBundleLanguageNamesArray
    { "Lithuanian",             "lt"        },  //                      # __RSBundleLanguageNamesArray
    { "Macedonian",             "mk"        },  //                      # __RSBundleLanguageNamesArray
    { "Malagasy",               "mg"        },  //                      # __RSBundleLanguageNamesArray
    { "Malay",                  "ms"        },  // -Latn,-Arab?         # __RSBundleLanguageNamesArray (had 2 entries "Malay" for "ms-Latn", "ms-Arab")
    { "Malayalam",              "ml"        },  //                      # __RSBundleLanguageNamesArray
    { "Maltese",                "mt"        },  //                      # __RSBundleLanguageNamesArray
    { "Manx",                   "gv"        },  //                      # __RSBundleLanguageNamesArray
    { "Marathi",                "mr"        },  //                      # __RSBundleLanguageNamesArray
    { "Moldavian",              "mo"        },  //                      # __RSBundleLanguageNamesArray
    { "Mongolian",              "mn"        },  // -Mong,-Cyrl?         # __RSBundleLanguageNamesArray (had 2 entries "Mongolian" for "mn-Mong", "mn-Cyrl")
    { "Nepali",                 "ne"        },  //                      # __RSBundleLanguageNamesArray
    { "Norwegian",              "nb"        },  //                      # __RSBundleLanguageNamesArray (had "Norwegian" mapping to "no")
    { "Nyanja",                 "ny"        },  //                      # __RSBundleLanguageNamesArray
    { "Nynorsk",                "nn"        },  //                      # handle other names (no entry in __RSBundleLanguageNamesArray)
    { "Oriya",                  "or"        },  //                      # __RSBundleLanguageNamesArray
    { "Oromo",                  "om"        },  //                      # __RSBundleLanguageNamesArray
    { "Panjabi",                "pa"        },  //                      # handle other names
    { "Pashto",                 "ps"        },  //                      # __RSBundleLanguageNamesArray
    { "Persian",                "fa"        },  //                      # handle other names
    { "Polish",                 "pl"        },  //                      # __RSBundleLanguageNamesArray
    { "Portuguese",             "pt"        },  //                      # __RSBundleLanguageNamesArray
    { "Portuguese, Brazilian",  "pt-BR"     },  //                      # handle other names
    { "Punjabi",                "pa"        },  //                      # __RSBundleLanguageNamesArray
    { "Pushto",                 "ps"        },  //                      # handle other names
    { "Quechua",                "qu"        },  //                      # __RSBundleLanguageNamesArray
    { "Romanian",               "ro"        },  //                      # __RSBundleLanguageNamesArray
    { "Ruanda",                 "rw"        },  //                      # handle other names
    { "Rundi",                  "rn"        },  //                      # __RSBundleLanguageNamesArray
    { "Russian",                "ru"        },  //                      # __RSBundleLanguageNamesArray
    { "Sami",                   "se"        },  //                      # __RSBundleLanguageNamesArray
    { "Sanskrit",               "sa"        },  //                      # __RSBundleLanguageNamesArray
    { "Scottish",               "gd"        },  //                      # __RSBundleLanguageNamesArray
    { "Serbian",                "sr"        },  //                      # __RSBundleLanguageNamesArray
    { "Simplified Chinese",     "zh-Hans"   },  //                      # handle other names
    { "Sindhi",                 "sd"        },  //                      # __RSBundleLanguageNamesArray
    { "Sinhalese",              "si"        },  //                      # __RSBundleLanguageNamesArray
    { "Slovak",                 "sk"        },  //                      # __RSBundleLanguageNamesArray
    { "Slovenian",              "sl"        },  //                      # __RSBundleLanguageNamesArray
    { "Somali",                 "so"        },  //                      # __RSBundleLanguageNamesArray
    { "Spanish",                "es"        },  //                      # __RSBundleLanguageNamesArray
    { "Sundanese",              "su"        },  //                      # __RSBundleLanguageNamesArray
    { "Swahili",                "sw"        },  //                      # __RSBundleLanguageNamesArray
    { "Swedish",                "sv"        },  //                      # __RSBundleLanguageNamesArray
    { "Tagalog",                "tl"        },  //                      # __RSBundleLanguageNamesArray
    { "Tajik",                  "tg"        },  //                      # handle other names
    { "Tajiki",                 "tg"        },  //                      # __RSBundleLanguageNamesArray
    { "Tamil",                  "ta"        },  //                      # __RSBundleLanguageNamesArray
    { "Tatar",                  "tt"        },  //                      # __RSBundleLanguageNamesArray
    { "Telugu",                 "te"        },  //                      # __RSBundleLanguageNamesArray
    { "Thai",                   "th"        },  //                      # __RSBundleLanguageNamesArray
    { "Tibetan",                "bo"        },  //                      # __RSBundleLanguageNamesArray
    { "Tigrinya",               "ti"        },  //                      # __RSBundleLanguageNamesArray
    { "Tongan",                 "to"        },  //                      # __RSBundleLanguageNamesArray
    { "Traditional Chinese",    "zh-Hant"   },  //                      # handle other names
    { "Turkish",                "tr"        },  //                      # __RSBundleLanguageNamesArray
    { "Turkmen",                "tk"        },  //                      # __RSBundleLanguageNamesArray
    { "Uighur",                 "ug"        },  //                      # __RSBundleLanguageNamesArray
    { "Ukrainian",              "uk"        },  //                      # __RSBundleLanguageNamesArray
    { "Urdu",                   "ur"        },  //                      # __RSBundleLanguageNamesArray
    { "Uzbek",                  "uz"        },  //                      # __RSBundleLanguageNamesArray
    { "Vietnamese",             "vi"        },  //                      # __RSBundleLanguageNamesArray
    { "Welsh",                  "cy"        },  //                      # __RSBundleLanguageNamesArray
    { "Yiddish",                "yi"        },  //                      # __RSBundleLanguageNamesArray
    { "ar_??",                  "ar"        },  //                      # from old MapScriptInfoAndISOCodes
    { "az.Ar",                  "az-Arab"   },  //                      # from old LocaleRefGetPartString
    { "az.Cy",                  "az-Cyrl"   },  //                      # from old LocaleRefGetPartString
    { "az.La",                  "az-Latn"   },  //                      # from old LocaleRefGetPartString
    { "be_??",                  "be_BY"     },  //                      # from old MapScriptInfoAndISOCodes
    { "bn_??",                  "bn"        },  //                      # from old LocaleRefGetPartString
    { "bo_??",                  "bo"        },  //                      # from old MapScriptInfoAndISOCodes
    { "br_??",                  "br"        },  //                      # from old MapScriptInfoAndISOCodes
    { "cy_??",                  "cy"        },  //                      # from old MapScriptInfoAndISOCodes
    { "de-96",                  "de-1996"   },  //                      # from old MapScriptInfoAndISOCodes                     // <1.9>
    { "de_96",                  "de-1996"   },  //                      # from old MapScriptInfoAndISOCodes                     // <1.9>
    { "de_??",                  "de-1996"   },  //                      # from old MapScriptInfoAndISOCodes
    { "el.El-P",                "grc"       },  //                      # from old LocaleRefGetPartString
    { "en-ascii",               "en_001"    },  //                      # from earlier version of tables in this file!
    { "en_??",                  "en_001"    },  //                      # from old MapScriptInfoAndISOCodes
    { "eo_??",                  "eo"        },  //                      # from old MapScriptInfoAndISOCodes
    { "es_??",                  "es_419"    },  //                      # from old MapScriptInfoAndISOCodes
    { "es_XL",                  "es_419"    },  //                      # from earlier version of tables in this file!
    { "fr_??",                  "fr_001"    },  //                      # from old MapScriptInfoAndISOCodes
    { "ga-dots",                "ga-Latg"   },  //                      # from earlier version of tables in this file!          // <1.8>
    { "ga-dots_IE",             "ga-Latg_IE" }, //                      # from earlier version of tables in this file!          // <1.8>
    { "ga.Lg",                  "ga-Latg"   },  //                      # from old LocaleRefGetPartString                       // <1.8>
    { "ga.Lg_IE",               "ga-Latg_IE" }, //                      # from old LocaleRefGetPartString                       // <1.8>
    { "gd_??",                  "gd"        },  //                      # from old MapScriptInfoAndISOCodes
    { "gv_??",                  "gv"        },  //                      # from old MapScriptInfoAndISOCodes
    { "jv.La",                  "jv"        },  //                      # logical extension                                     // <1.9>
    { "jw.La",                  "jv"        },  //                      # from old LocaleRefGetPartString
    { "kk.Cy",                  "kk"        },  //                      # from old LocaleRefGetPartString
    { "kl.La",                  "kl"        },  //                      # from old LocaleRefGetPartString
    { "kl.La_GL",               "kl_GL"     },  //                      # from old LocaleRefGetPartString                       // <1.9>
    { "lp_??",                  "se"        },  //                      # from old MapScriptInfoAndISOCodes
    { "mk_??",                  "mk_MK"     },  //                      # from old MapScriptInfoAndISOCodes
    { "mn.Cy",                  "mn-Cyrl"   },  //                      # from old LocaleRefGetPartString
    { "mn.Mn",                  "mn-Mong"   },  //                      # from old LocaleRefGetPartString
    { "ms.Ar",                  "ms-Arab"   },  //                      # from old LocaleRefGetPartString
    { "ms.La",                  "ms"        },  //                      # from old LocaleRefGetPartString
    { "nl-be",                  "nl-BE"     },  //                      # from old LocaleRefGetPartString
    { "nl-be_BE",               "nl_BE"     },  //                      # from old LocaleRefGetPartString
    { "no-NO",					"nb-NO"     },  //                      # not handled by localeStringPrefixToCanonical
    { "no-NO_NO",				"nb-NO_NO"  },  //                      # not handled by localeStringPrefixToCanonical
    //  { "no-bok_NO",              "nb_NO"     },  //                      # from old LocaleRefGetPartString - handled by localeStringPrefixToCanonical
    //  { "no-nyn_NO",              "nn_NO"     },  //                      # from old LocaleRefGetPartString - handled by localeStringPrefixToCanonical
    //  { "nya",                    "ny"        },  //                      # from old LocaleRefGetPartString - handled by localeStringPrefixToCanonical
    { "pa_??",                  "pa"        },  //                      # from old LocaleRefGetPartString
    { "sa.Dv",                  "sa"        },  //                      # from old LocaleRefGetPartString
    { "sl_??",                  "sl_SI"     },  //                      # from old MapScriptInfoAndISOCodes
    { "sr_??",                  "sr_RS"     },  //                      # from old MapScriptInfoAndISOCodes						// <1.18>
    { "su.La",                  "su"        },  //                      # from old LocaleRefGetPartString
    { "yi.He",                  "yi"        },  //                      # from old LocaleRefGetPartString
    { "zh-simp",                "zh-Hans"   },  //                      # from earlier version of tables in this file!
    { "zh-trad",                "zh-Hant"   },  //                      # from earlier version of tables in this file!
    { "zh.Ha-S",                "zh-Hans"   },  //                      # from old LocaleRefGetPartString
    { "zh.Ha-S_CN",             "zh_CN"     },  //                      # from old LocaleRefGetPartString
    { "zh.Ha-T",                "zh-Hant"   },  //                      # from old LocaleRefGetPartString
    { "zh.Ha-T_TW",             "zh_TW"     },  //                      # from old LocaleRefGetPartString
};
enum {
    kNumOldAppleLocaleToCanonical = sizeof(oldAppleLocaleToCanonical)/sizeof(KeyStringToResultString)
};

static const KeyStringToResultString localeStringPrefixToCanonical[] = {
    // Map 3-letter & obsolete ISO 639 codes, plus obsolete RFC 3066 codes, to 2-letter ISO 639 code.
    // (special cases for 'sh' handled separately)
    // First column must be all lowercase; must be sorted according to how strcmp compares the strings in the first column.
    //
    //    non-canonical canonical       [  comment ]                # source/reason for non-canonical string
    //    prefix        prefix
    //    ------------- ---------
    
    { "afr",        "af"        },  // Afrikaans
    { "alb",        "sq"        },  // Albanian
    { "amh",        "am"        },  // Amharic
    { "ara",        "ar"        },  // Arabic
    { "arm",        "hy"        },  // Armenian
    { "asm",        "as"        },  // Assamese
    { "aym",        "ay"        },  // Aymara
    { "aze",        "az"        },  // Azerbaijani
    { "baq",        "eu"        },  // Basque
    { "bel",        "be"        },  // Belarusian
    { "ben",        "bn"        },  // Bengali
    { "bih",        "bh"        },  // Bihari
    { "bod",        "bo"        },  // Tibetan
    { "bos",        "bs"        },  // Bosnian
    { "bre",        "br"        },  // Breton
    { "bul",        "bg"        },  // Bulgarian
    { "bur",        "my"        },  // Burmese
    { "cat",        "ca"        },  // Catalan
    { "ces",        "cs"        },  // Czech
    { "che",        "ce"        },  // Chechen
    { "chi",        "zh"        },  // Chinese
    { "cor",        "kw"        },  // Cornish
    { "cos",        "co"        },  // Corsican
    { "cym",        "cy"        },  // Welsh
    { "cze",        "cs"        },  // Czech
    { "dan",        "da"        },  // Danish
    { "deu",        "de"        },  // German
    { "dut",        "nl"        },  // Dutch
    { "dzo",        "dz"        },  // Dzongkha
    { "ell",        "el"        },  // Greek, Modern (1453-)
    { "eng",        "en"        },  // English
    { "epo",        "eo"        },  // Esperanto
    { "est",        "et"        },  // Estonian
    { "eus",        "eu"        },  // Basque
    { "fao",        "fo"        },  // Faroese
    { "fas",        "fa"        },  // Persian
    { "fin",        "fi"        },  // Finnish
    { "fra",        "fr"        },  // French
    { "fre",        "fr"        },  // French
    { "geo",        "ka"        },  // Georgian
    { "ger",        "de"        },  // German
    { "gla",        "gd"        },  // Gaelic,Scottish
    { "gle",        "ga"        },  // Irish
    { "glg",        "gl"        },  // Gallegan
    { "glv",        "gv"        },  // Manx
    { "gre",        "el"        },  // Greek, Modern (1453-)
    { "grn",        "gn"        },  // Guarani
    { "guj",        "gu"        },  // Gujarati
    { "heb",        "he"        },  // Hebrew
    { "hin",        "hi"        },  // Hindi
    { "hrv",        "hr"        },  // Croatian
    { "hun",        "hu"        },  // Hungarian
    { "hye",        "hy"        },  // Armenian
    { "i-hak",      "zh-hakka"  },  // Hakka                    # deprecated RFC 3066
    { "i-lux",      "lb"        },  // Luxembourgish            # deprecated RFC 3066
    { "i-navajo",   "nv"        },  // Navajo                   # deprecated RFC 3066
    { "ice",        "is"        },  // Icelandic
    { "iku",        "iu"        },  // Inuktitut
    { "ile",        "ie"        },  // Interlingue
    { "in",         "id"        },  // Indonesian               # deprecated 639 code in -> id (1989)
    { "ina",        "ia"        },  // Interlingua
    { "ind",        "id"        },  // Indonesian
    { "isl",        "is"        },  // Icelandic
    { "ita",        "it"        },  // Italian
    { "iw",         "he"        },  // Hebrew                   # deprecated 639 code iw -> he (1989)
    { "jav",        "jv"        },  // Javanese
    { "jaw",        "jv"        },  // Javanese                 # deprecated 639 code jaw -> jv (2001)
    { "ji",         "yi"        },  // Yiddish                  # deprecated 639 code ji -> yi (1989)
    { "jpn",        "ja"        },  // Japanese
    { "kal",        "kl"        },  // Kalaallisut
    { "kan",        "kn"        },  // Kannada
    { "kas",        "ks"        },  // Kashmiri
    { "kat",        "ka"        },  // Georgian
    { "kaz",        "kk"        },  // Kazakh
    { "khm",        "km"        },  // Khmer
    { "kin",        "rw"        },  // Kinyarwanda
    { "kir",        "ky"        },  // Kirghiz
    { "kor",        "ko"        },  // Korean
    { "kur",        "ku"        },  // Kurdish
    { "lao",        "lo"        },  // Lao
    { "lat",        "la"        },  // Latin
    { "lav",        "lv"        },  // Latvian
    { "lit",        "lt"        },  // Lithuanian
    { "ltz",        "lb"        },  // Letzeburgesch
    { "mac",        "mk"        },  // Macedonian
    { "mal",        "ml"        },  // Malayalam
    { "mar",        "mr"        },  // Marathi
    { "may",        "ms"        },  // Malay
    { "mkd",        "mk"        },  // Macedonian
    { "mlg",        "mg"        },  // Malagasy
    { "mlt",        "mt"        },  // Maltese
    { "mol",        "mo"        },  // Moldavian
    { "mon",        "mn"        },  // Mongolian
    { "msa",        "ms"        },  // Malay
    { "mya",        "my"        },  // Burmese
    { "nep",        "ne"        },  // Nepali
    { "nld",        "nl"        },  // Dutch
    { "nno",        "nn"        },  // Norwegian Nynorsk
    { "no",         "nb"        },  // Norwegian generic        # ambiguous 639 code no -> nb
    { "no-bok",     "nb"        },  // Norwegian Bokmal         # deprecated RFC 3066 tag - used in old LocaleRefGetPartString
    { "no-nyn",     "nn"        },  // Norwegian Nynorsk        # deprecated RFC 3066 tag - used in old LocaleRefGetPartString
    { "nob",        "nb"        },  // Norwegian Bokmal
    { "nor",        "nb"        },  // Norwegian generic        # ambiguous 639 code nor -> nb
    { "nya",        "ny"        },  // Nyanja/Chewa/Chichewa    # 3-letter code used in old LocaleRefGetPartString
    { "oci",        "oc"        },  // Occitan/Provencal
    { "ori",        "or"        },  // Oriya
    { "orm",        "om"        },  // Oromo,Galla
    { "pan",        "pa"        },  // Panjabi
    { "per",        "fa"        },  // Persian
    { "pol",        "pl"        },  // Polish
    { "por",        "pt"        },  // Portuguese
    { "pus",        "ps"        },  // Pushto
    { "que",        "qu"        },  // Quechua
    { "roh",        "rm"        },  // Raeto-Romance
    { "ron",        "ro"        },  // Romanian
    { "rum",        "ro"        },  // Romanian
    { "run",        "rn"        },  // Rundi
    { "rus",        "ru"        },  // Russian
    { "san",        "sa"        },  // Sanskrit
    { "scc",        "sr"        },  // Serbian
    { "scr",        "hr"        },  // Croatian
    { "sin",        "si"        },  // Sinhalese
    { "slk",        "sk"        },  // Slovak
    { "slo",        "sk"        },  // Slovak
    { "slv",        "sl"        },  // Slovenian
    { "sme",        "se"        },  // Sami,Northern
    { "snd",        "sd"        },  // Sindhi
    { "som",        "so"        },  // Somali
    { "spa",        "es"        },  // Spanish
    { "sqi",        "sq"        },  // Albanian
    { "srp",        "sr"        },  // Serbian
    { "sun",        "su"        },  // Sundanese
    { "swa",        "sw"        },  // Swahili
    { "swe",        "sv"        },  // Swedish
    { "tam",        "ta"        },  // Tamil
    { "tat",        "tt"        },  // Tatar
    { "tel",        "te"        },  // Telugu
    { "tgk",        "tg"        },  // Tajik
    { "tgl",        "tl"        },  // Tagalog
    { "tha",        "th"        },  // Thai
    { "tib",        "bo"        },  // Tibetan
    { "tir",        "ti"        },  // Tigrinya
    { "ton",        "to"        },  // Tongan
    { "tuk",        "tk"        },  // Turkmen
    { "tur",        "tr"        },  // Turkish
    { "uig",        "ug"        },  // Uighur
    { "ukr",        "uk"        },  // Ukrainian
    { "urd",        "ur"        },  // Urdu
    { "uzb",        "uz"        },  // Uzbek
    { "vie",        "vi"        },  // Vietnamese
    { "wel",        "cy"        },  // Welsh
    { "yid",        "yi"        },  // Yiddish
    { "zho",        "zh"        },  // Chinese
};
enum {
    kNumLocaleStringPrefixToCanonical = sizeof(localeStringPrefixToCanonical)/sizeof(KeyStringToResultString)
};


static const SpecialCaseUpdates specialCases[] = {
    // Data for special cases
    // a) The 3166 code CS was used for Czechoslovakia until 1993, when that country split and the code was
    // replaced by CZ and SK. Then in 2003-07, the code YU (formerly designating all of Yugoslavia, then after
    // the 1990s breakup just designating what is now Serbia and Montenegro) was changed to CS! Then after
    // Serbia and Montenegro split, the code CS was replaced in 2006-09 with separate codes RS and ME. If we
    // see CS but a language of cs or sk, we change CS to CZ or SK. Otherwise, we change CS (and old YU) to RS.
    // b) The 639 code sh for Serbo-Croatian was also replaced in the 1990s by separate codes hr and sr, and
    // deprecated in 2000. We guess which one to map it to as follows: If there is a region tag of HR we use
    // hr; if there is a region tag of (now) RS we use sr; else we do not change it (not enough info).
    // c) There are other codes that have been updated without these issues (eg. TP to TL), plus among the
    // "exceptionally reserved" codes some are just alternates for standard codes (eg. UK for GB).
    {   NULL,   "-UK",  "GB",   NULL,   NULL    },  // always change UK to GB (UK is "exceptionally reserved" to mean GB)
    {   NULL,   "-TP",  "TL",   NULL,   NULL    },  // always change TP to TL (East Timor, code changed 2002-05)
    {   "cs",   "-CS",  "CZ",   NULL,   NULL    },  // if language is cs, change CS (pre-1993 Czechoslovakia) to CZ (Czech Republic)
    {   "sk",   "-CS",  "SK",   NULL,   NULL    },  // if language is sk, change CS (pre-1993 Czechoslovakia) to SK (Slovakia)
    {   NULL,   "-CS",  "RS",   NULL,   NULL    },  // otherwise map CS (assume Serbia+Montenegro) to RS (Serbia)
    {   NULL,   "-YU",  "RS",   NULL,   NULL    },  // also map old YU (assume Serbia+Montenegro) to RS (Serbia)
    {   "sh",   "-HR",  "hr",   "-RS",  "sr"    },  // then if language is old 'sh' (SerboCroatian), change it to 'hr' (Croatian)
    // if we find HR (Croatia) or to 'sr' (Serbian) if we find RS (Serbia).
    // Note: Do this after changing YU/CS toRS as above.
    {   NULL,   NULL,   NULL,   NULL,   NULL    }   // terminator
};


static const KeyStringToResultString localeStringRegionToDefaults[] = {
    // For some region-code suffixes, there are default substrings to strip off for canonical string.
    // Must be sorted according to how strcmp compares the strings in the first column
    //
    //  region      default writing
    //  suffix      system tags, strip     comment
    //  --------    -------------          ---------
    { "_CN",    "-Hans"         },  // mainland China, default is simplified
    { "_HK",    "-Hant"         },  // Hong Kong, default is traditional
    { "_MO",    "-Hant"         },  // Macao, default is traditional
    { "_SG",    "-Hans"         },  // Singapore, default is simplified
    { "_TW",    "-Hant"         },  // Taiwan, default is traditional
};
enum {
    kNumLocaleStringRegionToDefaults = sizeof(localeStringRegionToDefaults)/sizeof(KeyStringToResultString)
};

static const KeyStringToResultString localeStringPrefixToDefaults[] = {
    // For some initial portions of language tag, there are default substrings to strip off for canonical string.
    // Must be sorted according to how strcmp compares the strings in the first column
    //
    //  language    default writing
    //  tag prefix  system tags, strip     comment
    //  --------    -------------          ---------
    { "ab-",    "-Cyrl"         },  // Abkhazian
    { "af-",    "-Latn"         },  // Afrikaans
    { "am-",    "-Ethi"         },  // Amharic
    { "ar-",    "-Arab"         },  // Arabic
    { "as-",    "-Beng"         },  // Assamese
    { "ay-",    "-Latn"         },  // Aymara
    { "be-",    "-Cyrl"         },  // Belarusian
    { "bg-",    "-Cyrl"         },  // Bulgarian
    { "bn-",    "-Beng"         },  // Bengali
    { "bo-",    "-Tibt"         },  // Tibetan (? not Suppress-Script)
    { "br-",    "-Latn"         },  // Breton (? not Suppress-Script)
    { "bs-",    "-Latn"         },  // Bosnian
    { "ca-",    "-Latn"         },  // Catalan
    { "cs-",    "-Latn"         },  // Czech
    { "cy-",    "-Latn"         },  // Welsh
    { "da-",    "-Latn"         },  // Danish
    { "de-",    "-Latn -1901"   },  // German, traditional orthography
    { "dv-",    "-Thaa"         },  // Divehi/Maldivian
    { "dz-",    "-Tibt"         },  // Dzongkha
    { "el-",    "-Grek"         },  // Greek (modern, monotonic)
    { "en-",    "-Latn"         },  // English
    { "eo-",    "-Latn"         },  // Esperanto
    { "es-",    "-Latn"         },  // Spanish
    { "et-",    "-Latn"         },  // Estonian
    { "eu-",    "-Latn"         },  // Basque
    { "fa-",    "-Arab"         },  // Farsi
    { "fi-",    "-Latn"         },  // Finnish
    { "fo-",    "-Latn"         },  // Faroese
    { "fr-",    "-Latn"         },  // French
    { "ga-",    "-Latn"         },  // Irish
    { "gd-",    "-Latn"         },  // Scottish Gaelic (? not Suppress-Script)
    { "gl-",    "-Latn"         },  // Galician
    { "gn-",    "-Latn"         },  // Guarani
    { "gu-",    "-Gujr"         },  // Gujarati
    { "gv-",    "-Latn"         },  // Manx
    { "haw-",   "-Latn"         },  // Hawaiian (? not Suppress-Script)
    { "he-",    "-Hebr"         },  // Hebrew
    { "hi-",    "-Deva"         },  // Hindi
    { "hr-",    "-Latn"         },  // Croatian
    { "hu-",    "-Latn"         },  // Hungarian
    { "hy-",    "-Armn"         },  // Armenian
    { "id-",    "-Latn"         },  // Indonesian
    { "is-",    "-Latn"         },  // Icelandic
    { "it-",    "-Latn"         },  // Italian
    { "ja-",    "-Jpan"         },  // Japanese
    { "ka-",    "-Geor"         },  // Georgian
    { "kk-",    "-Cyrl"         },  // Kazakh
    { "kl-",    "-Latn"         },  // Kalaallisut/Greenlandic
    { "km-",    "-Khmr"         },  // Central Khmer
    { "kn-",    "-Knda"         },  // Kannada
    { "ko-",    "-Hang"         },  // Korean (? not Suppress-Script)
    { "kok-",   "-Deva"         },  // Konkani
    { "la-",    "-Latn"         },  // Latin
    { "lb-",    "-Latn"         },  // Luxembourgish
    { "lo-",    "-Laoo"         },  // Lao
    { "lt-",    "-Latn"         },  // Lithuanian
    { "lv-",    "-Latn"         },  // Latvian
    { "mg-",    "-Latn"         },  // Malagasy
    { "mk-",    "-Cyrl"         },  // Macedonian
    { "ml-",    "-Mlym"         },  // Malayalam
    { "mo-",    "-Latn"         },  // Moldavian
    { "mr-",    "-Deva"         },  // Marathi
    { "ms-",    "-Latn"         },  // Malay
    { "mt-",    "-Latn"         },  // Maltese
    { "my-",    "-Mymr"         },  // Burmese/Myanmar
    { "nb-",    "-Latn"         },  // Norwegian Bokmal
    { "ne-",    "-Deva"         },  // Nepali
    { "nl-",    "-Latn"         },  // Dutch
    { "nn-",    "-Latn"         },  // Norwegian Nynorsk
    { "ny-",    "-Latn"         },  // Chichewa/Nyanja
    { "om-",    "-Latn"         },  // Oromo
    { "or-",    "-Orya"         },  // Oriya
    { "pa-",    "-Guru"         },  // Punjabi
    { "pl-",    "-Latn"         },  // Polish
    { "ps-",    "-Arab"         },  // Pushto
    { "pt-",    "-Latn"         },  // Portuguese
    { "qu-",    "-Latn"         },  // Quechua
    { "rn-",    "-Latn"         },  // Rundi
    { "ro-",    "-Latn"         },  // Romanian
    { "ru-",    "-Cyrl"         },  // Russian
    { "rw-",    "-Latn"         },  // Kinyarwanda
    { "sa-",    "-Deva"         },  // Sanskrit (? not Suppress-Script)
    { "se-",    "-Latn"         },  // Sami (? not Suppress-Script)
    { "si-",    "-Sinh"         },  // Sinhala
    { "sk-",    "-Latn"         },  // Slovak
    { "sl-",    "-Latn"         },  // Slovenian
    { "so-",    "-Latn"         },  // Somali
    { "sq-",    "-Latn"         },  // Albanian
    { "sv-",    "-Latn"         },  // Swedish
    { "sw-",    "-Latn"         },  // Swahili
    { "ta-",    "-Taml"         },  // Tamil
    { "te-",    "-Telu"         },  // Telugu
    { "th-",    "-Thai"         },  // Thai
    { "ti-",    "-Ethi"         },  // Tigrinya
    { "tl-",    "-Latn"         },  // Tagalog
    { "tn-",    "-Latn"         },  // Tswana
    { "to-",    "-Latn"         },  // Tonga of Tonga Islands
    { "tr-",    "-Latn"         },  // Turkish
    { "uk-",    "-Cyrl"         },  // Ukrainian
    { "ur-",    "-Arab"         },  // Urdu
    { "vi-",    "-Latn"         },  // Vietnamese
    { "wo-",    "-Latn"         },  // Wolof
    { "xh-",    "-Latn"         },  // Xhosa
    { "yi-",    "-Hebr"         },  // Yiddish
    { "zh-",    "-Hani"         },  // Chinese (? not Suppress-Script)
    { "zu-",    "-Latn"         },  // Zulu
};
enum {
    kNumLocaleStringPrefixToDefaults = sizeof(localeStringPrefixToDefaults)/sizeof(KeyStringToResultString)
};

static const KeyStringToResultString appleLocaleToLanguageString[] = {
    // Map locale strings that Apple uses as language IDs to real language strings.
    // Must be sorted according to how strcmp compares the strings in the first column.
    // Note: Now we remove all transforms of the form ll_RR -> ll-RR, they are now
    // handled in the code. <1.19>
    //
    //    locale 			lang			[  comment ]
    //    string			string
    //    -------			-------
    { "en_US_POSIX",	"en-US-POSIX"	},  // POSIX locale, need as language string			// <1.17> [3840752]
    { "zh_CN",  		"zh-Hans"		},  // mainland China => simplified
    { "zh_HK",  		"zh-Hant"		},  // Hong Kong => traditional, not currently used
    { "zh_MO",  		"zh-Hant"		},  // Macao => traditional, not currently used
    { "zh_SG",  		"zh-Hans"		},  // Singapore => simplified, not currently used
    { "zh_TW",  		"zh-Hant"		},  // Taiwan => traditional
};
enum {
    kNumAppleLocaleToLanguageString = sizeof(appleLocaleToLanguageString)/sizeof(KeyStringToResultString)
};

static const KeyStringToResultString appleLocaleToLanguageStringForRSBundle[] = {
    // Map locale strings that Apple uses as language IDs to real language strings.
    // Must be sorted according to how strcmp compares the strings in the first column.
    //
    //    locale 			lang			[  comment ]
    //    string			string
    //    -------			-------
    { "de_AT",  		"de-AT"			},  // Austrian German
    { "de_CH",  		"de-CH"			},  // Swiss German
    //  { "de_DE",  		"de-DE"			},  // German for Germany (default), not currently used
    { "en_AU", 			"en-AU"			},  // Australian English
    { "en_CA",  		"en-CA"			},  // Canadian English
    { "en_GB",  		"en-GB"			},  // British English
    //  { "en_IE",  		"en-IE"			},  // Irish English, not currently used
    { "en_US",  		"en-US"			},  // U.S. English
    { "en_US_POSIX",	"en-US-POSIX"	},  // POSIX locale, need as language string			// <1.17> [3840752]
    //  { "fr_BE",  		"fr-BE"			},  // Belgian French, not currently used
    { "fr_CA",  		"fr-CA"			},  // Canadian French
    { "fr_CH",  		"fr-CH"			},  // Swiss French
    //  { "fr_FR",  		"fr-FR"			},  // French for France (default), not currently used
    { "nl_BE",  		"nl-BE"			},  // Flemish = Vlaams, Dutch for Belgium
    //  { "nl_NL",  		"nl-NL"			},  // Dutch for Netherlands (default), not currently used
    { "pt_BR",  		"pt-BR"			},  // Brazilian Portuguese
    { "pt_PT",  		"pt-PT"     	},  // Portuguese for Portugal
    { "zh_CN",  		"zh-Hans"		},  // mainland China => simplified
    { "zh_HK",  		"zh-Hant"		},  // Hong Kong => traditional, not currently used
    { "zh_MO",  		"zh-Hant"		},  // Macao => traditional, not currently used
    { "zh_SG",  		"zh-Hans"		},  // Singapore => simplified, not currently used
    { "zh_TW",  		"zh-Hant"		},  // Taiwan => traditional
};
enum {
    kNumAppleLocaleToLanguageStringForRSBundle = sizeof(appleLocaleToLanguageStringForRSBundle)/sizeof(KeyStringToResultString)
};


struct LocaleToLegacyCodes {
    const char *        locale;	// reduced to language plus one other component (script, region, variant), separators normalized to'_'
    RegionCode		    regCode;
    LangCode		    langCode;
    RSStringEncoding    encoding;
};
typedef struct LocaleToLegacyCodes LocaleToLegacyCodes;

static const LocaleToLegacyCodes localeToLegacyCodes[] = {
	//	locale			RegionCode					LangCode						RSStringEncoding
    {   "af"/*ZA*/,     102/*verAfrikaans*/,        141/*langAfrikaans*/,            0/*Roman*/              },  // Latn
    {   "am",            -1,                         85/*langAmharic*/,             28/*Ethiopic*/           },  // Ethi
    {   "ar",            16/*verArabic*/,            12/*langArabic*/,               4/*Arabic*/             },  // Arab;
    {   "as",            -1,                         68/*langAssamese*/,            13/*Bengali*/            },  // Beng;
    {   "ay",            -1,                        134/*langAymara*/,               0/*Roman*/              },  // Latn;
    {   "az",            -1,                         49/*langAzerbaijani*/,          7/*Cyrillic*/           },  // assume "az" defaults to -Cyrl
    {   "az_Arab",       -1,                         50/*langAzerbaijanAr*/,         4/*Arabic*/             },  // Arab;
    {   "az_Cyrl",       -1,                         49/*langAzerbaijani*/,          7/*Cyrillic*/           },  // Cyrl;
    {   "az_Latn",       -1,                        150/*langAzerbaijanRoman*/,      0/*Roman*/              },  // Latn;
    {   "be"/*BY*/,      61/*verBelarus*/,           46/*langBelorussian*/,          7/*Cyrillic*/           },  // Cyrl;
    {   "bg"/*BG*/,      72/*verBulgaria*/,          44/*langBulgarian*/,            7/*Cyrillic*/           },  // Cyrl;
    {   "bn",            60/*verBengali*/,           67/*langBengali*/,             13/*Bengali*/            },  // Beng;
    {   "bo",           105/*verTibetan*/,           63/*langTibetan*/,             26/*Tibetan*/            },  // Tibt;
    {   "br",            77/*verBreton*/,           142/*langBreton*/,              39/*Celtic*/             },  // Latn;
    {   "ca"/*ES*/,      73/*verCatalonia*/,        130/*langCatalan*/,              0/*Roman*/              },  // Latn;
    {   "cs"/*CZ*/,      56/*verCzech*/,             38/*langCzech*/,               29/*CentralEurRoman*/    },  // Latn;
    {   "cy",            79/*verWelsh*/,            128/*langWelsh*/,               39/*Celtic*/             },  // Latn;
    {   "da"/*DK*/,       9/*verDenmark*/,            7/*langDanish*/,               0/*Roman*/              },  // Latn;
    {   "de",             3/*verGermany*/,            2/*langGerman*/,               0/*Roman*/              },  // assume "de" defaults to verGermany
    {   "de_1996",       70/*verGermanReformed*/,     2/*langGerman*/,               0/*Roman*/              },
    {   "de_AT",         92/*verAustria*/,            2/*langGerman*/,               0/*Roman*/              },
    {   "de_CH",         19/*verGrSwiss*/,            2/*langGerman*/,               0/*Roman*/              },
    {   "de_DE",          3/*verGermany*/,            2/*langGerman*/,               0/*Roman*/              },
    {   "dz"/*BT*/,      83/*verBhutan*/,           137/*langDzongkha*/,            26/*Tibetan*/            },  // Tibt;
    {   "el",            20/*verGreece*/,            14/*langGreek*/,                6/*Greek*/              },  // assume "el" defaults to verGreece
    {   "el_CY",         23/*verCyprus*/,            14/*langGreek*/,                6/*Greek*/              },
    {   "el_GR",         20/*verGreece*/,            14/*langGreek*/,                6/*Greek*/              },  // modern monotonic
    {   "en",             0/*verUS*/,                 0/*langEnglish*/,              0/*Roman*/              },  // "en" defaults to verUS (per Chris Hansten)
    {   "en_001",        37/*verInternational*/,      0/*langEnglish*/,              0/*Roman*/              },
    {   "en_AU",         15/*verAustralia*/,          0/*langEnglish*/,              0/*Roman*/              },
    {   "en_CA",         82/*verEngCanada*/,          0/*langEnglish*/,              0/*Roman*/              },
    {   "en_GB",          2/*verBritain*/,            0/*langEnglish*/,              0/*Roman*/              },
    {   "en_IE",        108/*verIrelandEnglish*/,     0/*langEnglish*/,              0/*Roman*/              },
    {   "en_SG",        100/*verSingapore*/,          0/*langEnglish*/,              0/*Roman*/              },
    {   "en_US",          0/*verUS*/,                 0/*langEnglish*/,              0/*Roman*/              },
    {   "eo",           103/*verEsperanto*/,         94/*langEsperanto*/,            0/*Roman*/              },  // Latn;
    {   "es",             8/*verSpain*/,              6/*langSpanish*/,              0/*Roman*/              },  // "es" defaults to verSpain (per Chris Hansten)
    {   "es_419",        86/*verSpLatinAmerica*/,     6/*langSpanish*/,              0/*Roman*/              },  // new BCP 47 tag
    {   "es_ES",          8/*verSpain*/,              6/*langSpanish*/,              0/*Roman*/              },
    {   "es_MX",         86/*verSpLatinAmerica*/,     6/*langSpanish*/,              0/*Roman*/              },
    {   "es_US",         86/*verSpLatinAmerica*/,     6/*langSpanish*/,              0/*Roman*/              },
    {   "et"/*EE*/,      44/*verEstonia*/,           27/*langEstonian*/,            29/*CentralEurRoman*/    },
    {   "eu",            -1,                        129/*langBasque*/,               0/*Roman*/              },  // Latn;
    {   "fa"/*IR*/,      48/*verIran*/,              31/*langFarsi/Persian*/,       0x8C/*Farsi*/            },  // Arab;
    {   "fi"/*FI*/,      17/*verFinland*/,           13/*langFinnish*/,              0/*Roman*/              },
    {   "fo"/*FO*/,      47/*verFaroeIsl*/,          30/*langFaroese*/,             37/*Icelandic*/          },
    {   "fr",             1/*verFrance*/,             1/*langFrench*/,               0/*Roman*/              },  // "fr" defaults to verFrance (per Chris Hansten)
    {   "fr_001",        91/*verFrenchUniversal*/,    1/*langFrench*/,               0/*Roman*/              },
    {   "fr_BE",         98/*verFrBelgium*/,          1/*langFrench*/,               0/*Roman*/              },
    {   "fr_CA",         11/*verFrCanada*/,           1/*langFrench*/,               0/*Roman*/              },
    {   "fr_CH",         18/*verFrSwiss*/,            1/*langFrench*/,               0/*Roman*/              },
    {   "fr_FR",          1/*verFrance*/,             1/*langFrench*/,               0/*Roman*/              },
    {   "ga"/*IE*/,      50/*verIreland*/,           35/*langIrishGaelic*/,          0/*Roman*/              },  // no dots (h after)
    {   "ga_Latg"/*IE*/, 81/*verIrishGaelicScrip*/, 146/*langIrishGaelicScript*/,   40/*Gaelic*/             },  // using dots
    {   "gd",            75/*verScottishGaelic*/,   144/*langScottishGaelic*/,      39/*Celtic*/             },
    {   "gl",            -1,                        140/*langGalician*/,             0/*Roman*/              },  // Latn;
    {   "gn",            -1,                        133/*langGuarani*/,              0/*Roman*/              },  // Latn;
    {   "grc",           40/*verGreekAncient*/,     148/*langGreekAncient*/,         6/*Greek*/              },  // polytonic (MacGreek doesn't actually support it)
    {   "gu"/*IN*/,      94/*verGujarati*/,          69/*langGujarati*/,            11/*Gujarati*/           },  // Gujr;
    {   "gv",            76/*verManxGaelic*/,       145/*langManxGaelic*/,          39/*Celtic*/             },  // Latn;
    {   "he"/*IL*/,      13/*verIsrael*/,            10/*langHebrew*/,               5/*Hebrew*/             },  // Hebr;
    {   "hi"/*IN*/,      33/*verIndiaHindi*/,        21/*langHindi*/,                9/*Devanagari*/         },  // Deva;
    {   "hr"/*HR*/,      68/*verCroatia*/,           18/*langCroatian*/,            36/*Croatian*/           },
    {   "hu"/*HU*/,      43/*verHungary*/,           26/*langHungarian*/,           29/*CentralEurRoman*/    },
    {   "hy"/*AM*/,      84/*verArmenian*/,          51/*langArmenian*/,            24/*Armenian*/           },  // Armn;
    {   "id",            -1,                         81/*langIndonesian*/,           0/*Roman*/              },  // Latn;
    {   "is"/*IS*/,      21/*verIceland*/,           15/*langIcelandic*/,           37/*Icelandic*/          },
    {   "it",             4/*verItaly*/,              3/*langItalian*/,              0/*Roman*/              },  // "it" defaults to verItaly
    {   "it_CH",         36/*verItalianSwiss*/,       3/*langItalian*/,              0/*Roman*/              },
    {   "it_IT",          4/*verItaly*/,              3/*langItalian*/,              0/*Roman*/              },
    {   "iu"/*CA*/,      78/*verNunavut*/,          143/*langInuktitut*/,           0xEC/*Inuit*/            },  // Cans;
    {   "ja"/*JP*/,      14/*verJapan*/,             11/*langJapanese*/,             1/*Japanese*/           },  // Jpan;
    {   "jv",            -1,                        138/*langJavaneseRom*/,          0/*Roman*/              },  // Latn;
    {   "ka"/*GE*/,      85/*verGeorgian*/,          52/*langGeorgian*/,            23/*Georgian*/           },  // Geor;
    {   "kk",            -1,                         48/*langKazakh*/,               7/*Cyrillic*/           },  // "kk" defaults to -Cyrl; also have -Latn, -Arab
    {   "kl",           107/*verGreenland*/,        149/*langGreenlandic*/,          0/*Roman*/              },  // Latn;
    {   "km",            -1,                         78/*langKhmer*/,               20/*Khmer*/              },  // Khmr;
    {   "kn",            -1,                         73/*langKannada*/,             16/*Kannada*/            },  // Knda;
    {   "ko"/*KR*/,      51/*verKorea*/,             23/*langKorean*/,               3/*Korean*/             },  // Hang;
    {   "ks",            -1,                         61/*langKashmiri*/,             4/*Arabic*/             },  // Arab;
    {   "ku",            -1,                         60/*langKurdish*/,              4/*Arabic*/             },  // Arab;
    {   "ky",            -1,                         54/*langKirghiz*/,              7/*Cyrillic*/           },  // Cyrl; also -Latn, -Arab
    {   "la",            -1,                        131/*langLatin*/,                0/*Roman*/              },  // Latn;
    {   "lo",            -1,                         79/*langLao*/,                 22/*Laotian*/            },  // Laoo;
    {   "lt"/*LT*/,      41/*verLithuania*/,         24/*langLithuanian*/,          29/*CentralEurRoman*/    },
    {   "lv"/*LV*/,      45/*verLatvia*/,            28/*langLatvian*/,             29/*CentralEurRoman*/    },
    {   "mg",            -1,                         93/*langMalagasy*/,             0/*Roman*/              },  // Latn;
    {   "mk"/*MK*/,      67/*verMacedonian*/,        43/*langMacedonian*/,           7/*Cyrillic*/           },  // Cyrl;
    {   "ml",            -1,                         72/*langMalayalam*/,           17/*Malayalam*/          },  // Mlym;
    {   "mn",            -1,                         57/*langMongolian*/,           27/*Mongolian*/          },  // "mn" defaults to -Mong
    {   "mn_Cyrl",       -1,                         58/*langMongolianCyr*/,         7/*Cyrillic*/           },  // Cyrl;
    {   "mn_Mong",       -1,                         57/*langMongolian*/,           27/*Mongolian*/          },  // Mong;
    {   "mo",            -1,                         53/*langMoldavian*/,            7/*Cyrillic*/           },  // Cyrl;
    {   "mr"/*IN*/,     104/*verMarathi*/,           66/*langMarathi*/,              9/*Devanagari*/         },  // Deva;
    {   "ms",            -1,                         83/*langMalayRoman*/,           0/*Roman*/              },  // "ms" defaults to -Latn;
    {   "ms_Arab",       -1,                         84/*langMalayArabic*/,          4/*Arabic*/             },  // Arab;
    {   "mt"/*MT*/,      22/*verMalta*/,             16/*langMaltese*/,              0/*Roman*/              },  // Latn;
    {   "mul",           74/*verMultilingual*/,      -1,                             0                       },
    {   "my",            -1,                         77/*langBurmese*/,             19/*Burmese*/            },  // Mymr;
    {   "nb"/*NO*/,      12/*verNorway*/,             9/*langNorwegian*/,            0/*Roman*/              },
    {   "ne"/*NP*/,     106/*verNepal*/,             64/*langNepali*/,               9/*Devanagari*/         },  // Deva;
    {   "nl",             5/*verNetherlands*/,        4/*langDutch*/,                0/*Roman*/              },  // "nl" defaults to verNetherlands
    {   "nl_BE",          6/*verFlemish*/,           34/*langFlemish*/,              0/*Roman*/              },
    {   "nl_NL",          5/*verNetherlands*/,        4/*langDutch*/,                0/*Roman*/              },
    {   "nn"/*NO*/,     101/*verNynorsk*/,          151/*langNynorsk*/,              0/*Roman*/              },
    {   "ny",            -1,                         92/*langNyanja/Chewa*/,         0/*Roman*/              },  // Latn;
    {   "om",            -1,                         87/*langOromo*/,               28/*Ethiopic*/           },  // Ethi;
    {   "or",            -1,                         71/*langOriya*/,               12/*Oriya*/              },  // Orya;
    {   "pa",            95/*verPunjabi*/,           70/*langPunjabi*/,             10/*Gurmukhi*/           },  // Guru;
    {   "pl"/*PL*/,      42/*verPoland*/,            25/*langPolish*/,              29/*CentralEurRoman*/    },
    {   "ps",            -1,                         59/*langPashto*/,              0x8C/*Farsi*/            },  // Arab;
    {   "pt",            71/*verBrazil*/,             8/*langPortuguese*/,           0/*Roman*/              },  // "pt" defaults to verBrazil (per Chris Hansten)
    {   "pt_BR",         71/*verBrazil*/,             8/*langPortuguese*/,           0/*Roman*/              },
    {   "pt_PT",         10/*verPortugal*/,           8/*langPortuguese*/,           0/*Roman*/              },
    {   "qu",            -1,                        132/*langQuechua*/,              0/*Roman*/              },  // Latn;
    {   "rn",            -1,                         91/*langRundi*/,                0/*Roman*/              },  // Latn;
    {   "ro"/*RO*/,      39/*verRomania*/,           37/*langRomanian*/,            38/*Romanian*/           },
    {   "ru"/*RU*/,      49/*verRussia*/,            32/*langRussian*/,              7/*Cyrillic*/           },  // Cyrl;
    {   "rw",            -1,                         90/*langKinyarwanda*/,          0/*Roman*/              },  // Latn;
    {   "sa",            -1,                         65/*langSanskrit*/,             9/*Devanagari*/         },  // Deva;
    {   "sd",            -1,                         62/*langSindhi*/,              0x8C/*Farsi*/            },  // Arab;
    {   "se",            46/*verSami*/,              29/*langSami*/,                 0/*Roman*/              },
    {   "si",            -1,                         76/*langSinhalese*/,           18/*Sinhalese*/          },  // Sinh;
    {   "sk"/*SK*/,      57/*verSlovak*/,            39/*langSlovak*/,              29/*CentralEurRoman*/    },
    {   "sl"/*SI*/,      66/*verSlovenian*/,         40/*langSlovenian*/,           36/*Croatian*/           },
    {   "so",            -1,                         88/*langSomali*/,               0/*Roman*/              },  // Latn;
    {   "sq",            -1,                         36/*langAlbanian*/,             0/*Roman*/              },
    {   "sr"/*CS,RS*/,   65/*verSerbian*/,           42/*langSerbian*/,              7/*Cyrillic*/           },  // Cyrl;
    {   "su",            -1,                        139/*langSundaneseRom*/,         0/*Roman*/              },  // Latn;
    {   "sv"/*SE*/,       7/*verSweden*/,             5/*langSwedish*/,              0/*Roman*/              },
    {   "sw",            -1,                         89/*langSwahili*/,              0/*Roman*/              },  // Latn;
    {   "ta",            -1,                         74/*langTamil*/,               14/*Tamil*/              },  // Taml;
    {   "te",            -1,                         75/*langTelugu*/,              15/*Telugu*/             },  // Telu
    {   "tg",            -1,                         55/*langTajiki*/,               7/*Cyrillic*/           },  // "tg" defaults to "Cyrl"
    {   "tg_Cyrl",       -1,                         55/*langTajiki*/,               7/*Cyrillic*/           },  // Cyrl; also -Latn, -Arab
    {   "th"/*TH*/,      54/*verThailand*/,          22/*langThai*/,                21/*Thai*/               },  // Thai;
    {   "ti",            -1,                         86/*langTigrinya*/,            28/*Ethiopic*/           },  // Ethi;
    {   "tk",            -1,                         56/*langTurkmen*/,              7/*Cyrillic*/           },  // "tk" defaults to Cyrl
    {   "tk_Cyrl",       -1,                         56/*langTurkmen*/,              7/*Cyrillic*/           },  // Cyrl; also -Latn, -Arab
    {   "tl",            -1,                         82/*langTagalog*/,              0/*Roman*/              },  // Latn;
    {   "to"/*TO*/,      88/*verTonga*/,            147/*langTongan*/,               0/*Roman*/              },  // Latn;
    {   "tr"/*TR*/,      24/*verTurkey*/,            17/*langTurkish*/,             35/*Turkish*/            },  // Latn;
    {   "tt",            -1,                        135/*langTatar*/,                7/*Cyrillic*/           },  // Cyrl;
    {   "tt_Cyrl",       -1,                        135/*langTatar*/,                7/*Cyrillic*/           },  // Cyrl;
    {   "ug",            -1,                        136/*langUighur*/,               4/*Arabic*/             },  // Arab;
    {   "uk"/*UA*/,      62/*verUkraine*/,           45/*langUkrainian*/,            7/*Cyrillic*/           },  // Cyrl;
    {   "und",           55/*verScriptGeneric*/,     -1,                             0                       },
    {   "ur",            34/*verPakistanUrdu*/,      20/*langUrdu*/,                0x8C/*Farsi*/            },  // "ur" defaults to verPakistanUrdu
    {   "ur_IN",         96/*verIndiaUrdu*/,         20/*langUrdu*/,                0x8C/*Farsi*/            },  // Arab
    {   "ur_PK",         34/*verPakistanUrdu*/,      20/*langUrdu*/,                0x8C/*Farsi*/            },  // Arab
    {   "uz"/*UZ*/,      99/*verUzbek*/,             47/*langUzbek*/,                7/*Cyrillic*/           },  // Cyrl; also -Latn, -Arab
    {   "uz_Cyrl",       99/*verUzbek*/,             47/*langUzbek*/,                7/*Cyrillic*/           },
    {   "vi"/*VN*/,      97/*verVietnam*/,           80/*langVietnamese*/,          30/*Vietnamese*/         },  // Latn
    {   "yi",            -1,                         41/*langYiddish*/,              5/*Hebrew*/             },  // Hebr;
    {   "zh",            52/*verChina*/,             33/*langSimpChinese*/,         25/*ChineseSimp*/        },  // "zh" defaults to verChina, langSimpChinese
    {   "zh_CN",         52/*verChina*/,             33/*langSimpChinese*/,         25/*ChineseSimp*/        },
    {   "zh_HK",         53/*verTaiwan*/,            19/*langTradChinese*/,          2/*ChineseTrad*/        },
    {   "zh_Hans",       52/*verChina*/,             33/*langSimpChinese*/,         25/*ChineseSimp*/        },
    {   "zh_Hant",       53/*verTaiwan*/,            19/*langTradChinese*/,          2/*ChineseTrad*/        },
    {   "zh_MO",         53/*verTaiwan*/,            19/*langTradChinese*/,          2/*ChineseTrad*/        },
    {   "zh_SG",         52/*verChina*/,             33/*langSimpChinese*/,         25/*ChineseSimp*/        },
    {   "zh_TW",         53/*verTaiwan*/,            19/*langTradChinese*/,          2/*ChineseTrad*/        },
};
enum {
    kNumLocaleToLegacyCodes = sizeof(localeToLegacyCodes)/sizeof(localeToLegacyCodes[0])
};

/*
 For reference here is a list of ICU locales with variants and how some
 of them are canonicalized with the ICU function uloc_canonicalize:
 
 ICU 3.0 has:
 en_US_POSIX			x	no change
 hy_AM_REVISED		x	no change
 ja_JP_TRADITIONAL	->	ja_JP@calendar=japanese
 th_TH_TRADITIONAL	->	th_TH@calendar=buddhist
 
 ICU 2.8 also had the following (now obsolete):
 ca_ES_PREEURO
 de__PHONEBOOK		->	de@collation=phonebook
 de_AT_PREEURO
 de_DE_PREEURO
 de_LU_PREEURO
 el_GR_PREEURO
 en_BE_PREEURO
 en_GB_EURO			->	en_GB@currency=EUR
 en_IE_PREEURO		->	en_IE@currency=IEP
 es__TRADITIONAL		->	es@collation=traditional
 es_ES_PREEURO
 eu_ES_PREEURO
 fi_FI_PREEURO
 fr_BE_PREEURO
 fr_FR_PREEURO		->	fr_FR@currency=FRF
 fr_LU_PREEURO
 ga_IE_PREEURO
 gl_ES_PREEURO
 hi__DIRECT			->	hi@collation=direct
 it_IT_PREEURO
 nl_BE_PREEURO
 nl_NL_PREEURO
 pt_PT_PREEURO
 zh__PINYIN			->	zh@collation=pinyin
 zh_TW_STROKE		->	zh_TW@collation=stroke
 
 */

// _CompareTestEntryToTableEntryKey
// (Local function for RSLocaleCreateCanonicalLocaleIdentifierFromString)
// comparison function for bsearch
static int _CompareTestEntryToTableEntryKey(const void *testEntryPtr, const void *tableEntryKeyPtr) {
    return strcmp( ((const KeyStringToResultString *)testEntryPtr)->key, ((const KeyStringToResultString *)tableEntryKeyPtr)->key );
}

// _CompareTestEntryPrefixToTableEntryKey
// (Local function for RSLocaleCreateCanonicalLocaleIdentifierFromString)
// Comparison function for bsearch. Assumes prefix IS terminated with '-' or '_'.
// Do the following instead of strlen & strncmp so we don't walk tableEntry key twice.
static int _CompareTestEntryPrefixToTableEntryKey(const void *testEntryPtr, const void *tableEntryKeyPtr) {
    const char *    testPtr = ((const KeyStringToResultString *)testEntryPtr)->key;
    const char *    tablePtr = ((const KeyStringToResultString *)tableEntryKeyPtr)->key;
    
    while ( *testPtr == *tablePtr && *tablePtr != 0 ) {
        testPtr++; tablePtr++;
    }
    if ( *tablePtr != 0 ) {
        // strings are different, and the string in the table has not run out;
        // i.e. the table entry is not a prefix of the text string.
        return ( *testPtr < *tablePtr )? -1: 1;
    }
    return 0;
}

// _CompareLowerTestEntryPrefixToTableEntryKey
// (Local function for RSLocaleCreateCanonicalLocaleIdentifierFromString)
// Comparison function for bsearch. Assumes prefix NOT terminated with '-' or '_'.
// Lowercases the test string before comparison (the table should already have lowercased entries).
static int _CompareLowerTestEntryPrefixToTableEntryKey(const void *testEntryPtr, const void *tableEntryKeyPtr) {
    const char *    testPtr = ((const KeyStringToResultString *)testEntryPtr)->key;
    const char *    tablePtr = ((const KeyStringToResultString *)tableEntryKeyPtr)->key;
    char            lowerTestChar;
    
    while ( (lowerTestChar = tolower(*testPtr)) == *tablePtr && *tablePtr != 0 && lowerTestChar != '_' ) {  // <1.9>
        testPtr++; tablePtr++;
    }
    if ( *tablePtr != 0 ) {
        // strings are different, and the string in the table has not run out;
        // i.e. the table entry is not a prefix of the text string.
        if (lowerTestChar == '_')                                                           // <1.9>
            return -1;                                                                      // <1.9>
        return ( lowerTestChar < *tablePtr )? -1: 1;
    }
    // The string in the table has run out. If the test string char is not alnum,
    // then the string matches, else the test string sorts after.
    return ( !isalnum(lowerTestChar) )? 0: 1;
}

// _DeleteCharsAtPointer
// (Local function for RSLocaleCreateCanonicalLocaleIdentifierFromString)
// remove _length_ characters from the beginning of the string indicated by _stringPtr_
// (we know that the string has at least _length_ characters in it)
static void _DeleteCharsAtPointer(char *stringPtr, int length) {
    do {
        *stringPtr = stringPtr[length];
    } while (*stringPtr++ != 0);
}

// _CopyReplacementAtPointer
// (Local function for RSLocaleCreateCanonicalLocaleIdentifierFromString)
// Copy replacement string (*excluding* terminating NULL byte) to the place indicated by stringPtr
static void _CopyReplacementAtPointer(char *stringPtr, const char *replacementPtr) {
    while (*replacementPtr != 0) {
        *stringPtr++ = *replacementPtr++;
    }
}

// _CheckForTag
// (Local function for RSLocaleCreateCanonicalLocaleIdentifierFromString)
static BOOL _CheckForTag(const char *localeStringPtr, const char *tagPtr, int tagLen) {
    return ( strncmp(localeStringPtr, tagPtr, tagLen) == 0 && !isalnum(localeStringPtr[tagLen]) );
}

// _ReplacePrefix
// Move this code from _UpdateFullLocaleString into separate function                       // <1.10>
static void _ReplacePrefix(char locString[], int locStringMaxLen, int oldPrefixLen, const char *newPrefix) {
    int newPrefixLen = (int)strlen(newPrefix);
    int lengthDelta = newPrefixLen - oldPrefixLen;
    
    if (lengthDelta < 0) {
        // replacement is shorter, delete chars by shifting tail of string
        _DeleteCharsAtPointer(locString + newPrefixLen, -lengthDelta);
    } else if (lengthDelta > 0) {
        // replacement is longer...
        int stringLen = (int)strlen(locString);
        
        if (stringLen + lengthDelta < locStringMaxLen) {
            // make room by shifting tail of string
            char *  tailShiftPtr = locString + stringLen;
            char *  tailStartPtr = locString + oldPrefixLen;    // pointer to tail of string to shift
            
            while (tailShiftPtr >= tailStartPtr) {
                tailShiftPtr[lengthDelta] = *tailShiftPtr;
                tailShiftPtr--;
            }
        } else {
            // no room, can't do substitution
            newPrefix = NULL;
        }
    }
    
    if (newPrefix) {
        // do the substitution
        _CopyReplacementAtPointer(locString, newPrefix);
    }
}

// _UpdateFullLocaleString
// Given a locale string that uses standard codes (not a special old-style Apple string),
// update all the language codes and region codes to latest versions, map 3-letter
// language codes to 2-letter codes if possible, and normalize casing. If requested, return
// pointers to a language-region variant subtag (if present) and a region tag (if present).
// (add locStringMaxLen parameter)                                                          // <1.10>
static void _UpdateFullLocaleString(char inLocaleString[], int locStringMaxLen,
									char **langRegSubtagRef, char **regionTagRef,
									char varKeyValueString[])								// <1.17>
{
    KeyStringToResultString     testEntry;
    KeyStringToResultString *   foundEntry;
    const SpecialCaseUpdates *  specialCasePtr;
    char *      inLocalePtr;
    char *      subtagPtr;
    char *      langRegSubtag = NULL;
    char *      regionTag = NULL;
    char *		variantTag = NULL;
    BOOL     subtagHasDigits, pastPrimarySubtag, hadRegion;
    
    // 1. First replace any non-canonical prefix (case insensitive) with canonical
    // (change 3-letter ISO 639 code to 2-letter, update obsolete ISO 639 codes & RFC 3066 tags, etc.)
    
    testEntry.key = inLocaleString;
    foundEntry = (KeyStringToResultString *)bsearch( &testEntry, localeStringPrefixToCanonical, kNumLocaleStringPrefixToCanonical,
                                                    sizeof(KeyStringToResultString), _CompareLowerTestEntryPrefixToTableEntryKey );
    if (foundEntry) {
        // replace key (at beginning of string) with result
        _ReplacePrefix(inLocaleString, locStringMaxLen, (int)strlen(foundEntry->key), foundEntry->result);   // <1.10>
    }
    
    // 2. Walk through input string, normalizing case & marking use of ISO 3166 codes
    
    inLocalePtr = inLocaleString;
    subtagPtr = inLocaleString;
    subtagHasDigits = NO;
    pastPrimarySubtag = NO;
    hadRegion = NO;
    
    while ( YES ) {
        if ( isalpha(*inLocalePtr) ) {
            // if not past a region tag, then lowercase, else uppercase
            *inLocalePtr = (!hadRegion)? tolower(*inLocalePtr): toupper(*inLocalePtr);
        } else if ( isdigit(*inLocalePtr) ) {
            subtagHasDigits = YES;
        } else {
            
            if (!pastPrimarySubtag) {
                // may have a NULL primary subtag
                if (subtagHasDigits) {
                    break;
                }
                pastPrimarySubtag = YES;
            } else if (!hadRegion) {
                // We are after any primary language subtag, but not past any region tag.
                // This subtag is preceded by '-' or '_'.
                int subtagLength = (int)(inLocalePtr - subtagPtr); // includes leading '-' or '_'
                
				if (subtagLength == 3 && !subtagHasDigits) {
					// potential ISO 3166 code for region or language variant; if so, needs uppercasing
					if (*subtagPtr == '_') {
						regionTag = subtagPtr;
						hadRegion = YES;
						subtagPtr[1] = toupper(subtagPtr[1]);
						subtagPtr[2] = toupper(subtagPtr[2]);
					} else if (langRegSubtag == NULL) {
						langRegSubtag = subtagPtr;
						subtagPtr[1] = toupper(subtagPtr[1]);
						subtagPtr[2] = toupper(subtagPtr[2]);
					}
				} else if (subtagLength == 4 && subtagHasDigits) {
					// potential UN M.49 region code
					if (*subtagPtr == '_') {
						regionTag = subtagPtr;
						hadRegion = YES;
					} else if (langRegSubtag == NULL) {
						langRegSubtag = subtagPtr;
					}
				} else if (subtagLength == 5 && !subtagHasDigits) {
					// ISO 15924 script code, uppercase just the first letter
					subtagPtr[1] = toupper(subtagPtr[1]);
				} else if (subtagLength == 1 && *subtagPtr == '_') {						// <1.17>
					hadRegion = YES;
				}
                
                if (!hadRegion) {
                    // convert improper '_' to '-'
                    *subtagPtr = '-';
                }
            } else {
            	variantTag = subtagPtr;															// <1.17>
            }
            
            if (*inLocalePtr == '-' || *inLocalePtr == '_') {
                subtagPtr = inLocalePtr;
                subtagHasDigits = NO;
            } else {
                break;
            }
        }
        
        inLocalePtr++;
    }
    
    // 3 If there is a variant tag, see if ICU canonicalizes it to keywords.					// <1.17> [3577669]
    // If so, copy the keywords to varKeyValueString and delete the variant tag
    // from the original string (but don't otherwise use the ICU canonicalization).
    varKeyValueString[0] = 0;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    if (variantTag) {
		UErrorCode	icuStatus;
		int			icuCanonStringLen;
		char * 		varKeyValueStringPtr = varKeyValueString;
		
		icuStatus = U_ZERO_ERROR;
		icuCanonStringLen = uloc_canonicalize( inLocaleString, varKeyValueString, locStringMaxLen, &icuStatus );
		if ( U_SUCCESS(icuStatus) ) {
			char *	icuCanonStringPtr = varKeyValueString;
			
			if (icuCanonStringLen >= locStringMaxLen)
				icuCanonStringLen = locStringMaxLen - 1;
			varKeyValueString[icuCanonStringLen] = 0;
			while (*icuCanonStringPtr != 0 && *icuCanonStringPtr != ULOC_KEYWORD_SEPARATOR)
				++icuCanonStringPtr;
			if (*icuCanonStringPtr != 0) {
				// the canonicalized string has keywords
				// delete the variant tag in the original string (and other trailing '_' or '-')
				*variantTag-- = 0;
				while (*variantTag == '_')
					*variantTag-- = 0;
				// delete all of the canonicalized string except the keywords
				while (*icuCanonStringPtr != 0)
					*varKeyValueStringPtr++ = *icuCanonStringPtr++;
			}
            *varKeyValueStringPtr = 0;
		}
    }
#endif
    
    // 4. Handle special cases of updating region codes, or updating language codes based on
    // region code.
    for (specialCasePtr = specialCases; specialCasePtr->reg1 != NULL; specialCasePtr++) {
        if ( specialCasePtr->lang == NULL || _CheckForTag(inLocaleString, specialCasePtr->lang, 2) ) {
            // OK, we matched any language specified. Now what needs updating?
            char * foundTag;
            
            if ( isupper(specialCasePtr->update1[0]) ) {
                // updating a region code
                if ( ( foundTag = strstr(inLocaleString, specialCasePtr->reg1) ) && !isalnum(foundTag[3]) ) {
                    _CopyReplacementAtPointer(foundTag+1, specialCasePtr->update1);
                }
                if ( regionTag && _CheckForTag(regionTag+1, specialCasePtr->reg1 + 1, 2) ) {
                    _CopyReplacementAtPointer(regionTag+1, specialCasePtr->update1);
                }
                
            } else {
                // updating the language, there will be two choices based on region
                if        ( ( regionTag && _CheckForTag(regionTag+1, specialCasePtr->reg1 + 1, 2) ) ||
                           ( ( foundTag = strstr(inLocaleString, specialCasePtr->reg1) ) && !isalnum(foundTag[3]) ) ) {
                    _CopyReplacementAtPointer(inLocaleString, specialCasePtr->update1);
                } else if ( ( regionTag && _CheckForTag(regionTag+1, specialCasePtr->reg2 + 1, 2) ) ||
                           ( ( foundTag = strstr(inLocaleString, specialCasePtr->reg2) ) && !isalnum(foundTag[3]) ) ) {
                    _CopyReplacementAtPointer(inLocaleString, specialCasePtr->update2);
                }
            }
        }
    }
    
    // 5. return pointers if requested.
    if (langRegSubtagRef != NULL) {
        *langRegSubtagRef = langRegSubtag;
    }
    if (regionTagRef != NULL) {
        *regionTagRef = regionTag;
    }
}


// _RemoveSubstringsIfPresent
// (Local function for RSLocaleCreateCanonicalLocaleIdentifierFromString)
// substringList is a list of space-separated substrings to strip if found in localeString
static void _RemoveSubstringsIfPresent(char *localeString, const char *substringList) {
    while (*substringList != 0) {
        char    currentSubstring[kLocaleIdentifierCStringMax];
        int     substringLength = 0;
        char *  foundSubstring;
        
        // copy current substring & get its length
        while ( isgraph(*substringList) ) {
            currentSubstring[substringLength++] = *substringList++;
        }
        // move to next substring
        while ( isspace(*substringList) ) {
            substringList++;
        }
        
        // search for current substring in locale string
        if (substringLength == 0)
            continue;
        currentSubstring[substringLength] = 0;
        foundSubstring = strstr(localeString, currentSubstring);
        
        // if substring is found, delete it
        if (foundSubstring) {
            _DeleteCharsAtPointer(foundSubstring, substringLength);
        }
    }
}


// _GetKeyValueString                                                                       // <1.10>
// Removes any key-value string from inLocaleString, puts canonized version in keyValueString

static void _GetKeyValueString(char inLocaleString[], char keyValueString[]) {
    char *  inLocalePtr = inLocaleString;
    
    while (*inLocalePtr != 0 && *inLocalePtr != ULOC_KEYWORD_SEPARATOR) {
        inLocalePtr++;
    }
    if (*inLocalePtr != 0) {    // we found a key-value section
        char *  keyValuePtr = keyValueString;
        
        *keyValuePtr = *inLocalePtr;
        *inLocalePtr = 0;
        do {
            if ( *(++inLocalePtr) != ' ' ) {
                *(++keyValuePtr) = *inLocalePtr;    // remove "tolower() for *inLocalePtr"  // <1.11>
            }
        } while (*inLocalePtr != 0);
    } else {
        keyValueString[0] = 0;
    }
}

static void _AppendKeyValueString(char inLocaleString[], int locStringMaxLen, char keyValueString[]) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
	if (keyValueString[0] != 0) {
		UErrorCode		uerr = U_ZERO_ERROR;
		UEnumeration *	uenum = uloc_openKeywords(keyValueString, &uerr);
		if ( uenum != NULL ) {
			const char *	keyword;
			int32_t			length;
			char			value[ULOC_KEYWORDS_CAPACITY];	// use as max for keyword value
			while ( U_SUCCESS(uerr) ) {
				keyword = uenum_next(uenum, &length, &uerr);
				if ( keyword == NULL ) {
					break;
				}
				length = uloc_getKeywordValue( keyValueString, keyword, value, sizeof(value), &uerr );
				length = uloc_setKeywordValue( keyword, value, inLocaleString, locStringMaxLen, &uerr );
			}
			uenum_close(uenum);
		}
	}
#endif
}

// __private_extern__ RSStringRef _RSLocaleCreateCanonicalLanguageIdentifierForRSBundle(RSAllocatorRef allocator, RSStringRef localeIdentifier) {}
extern RSStringRef  RSStringCreateWithCString(RSAllocatorRef alloc, const char *cStr, RSStringEncoding encoding);

RSStringRef RSLocaleCreateCanonicalLanguageIdentifierFromString(RSAllocatorRef allocator, RSStringRef localeIdentifier) {
    char            inLocaleString[kLocaleIdentifierCStringMax];
    RSStringRef     outStringRef = NULL;
    
    if ( localeIdentifier && 0 <= RSStringGetCString(localeIdentifier, inLocaleString,  sizeof(inLocaleString), RSStringEncodingASCII) ) {
        KeyStringToResultString     testEntry;
        KeyStringToResultString *   foundEntry;
        char                        keyValueString[sizeof(inLocaleString)];				// <1.10>
        char						varKeyValueString[sizeof(inLocaleString)];			// <1.17>
        
        _GetKeyValueString(inLocaleString, keyValueString);								// <1.10>
        testEntry.result = NULL;
        
        // A. First check if input string matches an old-style string that has a replacement
        // (do this before case normalization)
        testEntry.key = inLocaleString;
        foundEntry = (KeyStringToResultString *)bsearch( &testEntry, oldAppleLocaleToCanonical, kNumOldAppleLocaleToCanonical,
                                                        sizeof(KeyStringToResultString), _CompareTestEntryToTableEntryKey );
        if (foundEntry) {
            // It does match, so replace old string with new
            strlcpy(inLocaleString, foundEntry->result, sizeof(inLocaleString));
            varKeyValueString[0] = 0;
        } else {
            char *      langRegSubtag = NULL;
            char *      regionTag = NULL;
            
            // B. No match with an old-style string, use input string but update codes, normalize case, etc.
            _UpdateFullLocaleString(inLocaleString, sizeof(inLocaleString), &langRegSubtag, &regionTag, varKeyValueString);   // <1.10><1.17><1.19>
            
            // if the language part already includes a regional variant, then delete any region tag. <1.19>
            if (langRegSubtag && regionTag)
            	*regionTag = 0;
        }
        
        // C. Now we have an up-to-date locale string, but we need to strip defaults and turn it into a language string
        
        // 1. Strip defaults in input string based on initial part of locale string
        // (mainly to strip default script tag for a language)
        testEntry.key = inLocaleString;
        foundEntry = (KeyStringToResultString *)bsearch( &testEntry, localeStringPrefixToDefaults, kNumLocaleStringPrefixToDefaults,
                                                        sizeof(KeyStringToResultString), _CompareTestEntryPrefixToTableEntryKey );
        if (foundEntry) {
            // The input string begins with a character sequence for which
            // there are default substrings which should be stripped if present
            _RemoveSubstringsIfPresent(inLocaleString, foundEntry->result);
        }
        
        // 2. If the string matches a locale string used by Apple as a language string, turn it into a language string
        testEntry.key = inLocaleString;
        foundEntry = (KeyStringToResultString *)bsearch( &testEntry, appleLocaleToLanguageString, kNumAppleLocaleToLanguageString,
                                                        sizeof(KeyStringToResultString), _CompareTestEntryToTableEntryKey );
        if (foundEntry) {
            // it does match
            strlcpy(inLocaleString, foundEntry->result, sizeof(inLocaleString));
        } else {
            // skip to any region tag or java-type variant
            char *  inLocalePtr = inLocaleString;
            while (*inLocalePtr != 0 && *inLocalePtr != '_') {
                inLocalePtr++;
            }
            // if there is still a region tag, turn it into a language variant <1.19>
            if (*inLocalePtr == '_') {
            	// handle 3-digit regions in addition to 2-letter ones
            	char *	regionTag = inLocalePtr++;
            	long	expectedLength = 0;
            	if ( isalpha(*inLocalePtr) ) {
            		while ( isalpha(*(++inLocalePtr)) )
            			;
            		expectedLength = 3;
            	} else if ( isdigit(*inLocalePtr) ) {
            		while ( isdigit(*(++inLocalePtr)) )
            			;
            		expectedLength = 4;
            	}
            	*regionTag = (inLocalePtr - regionTag == expectedLength)? '-': 0;
            }
            // anything else at/after '_' just gets deleted
            *inLocalePtr = 0;
        }
        
        // D. Re-append any key-value strings, now canonical										// <1.10><1.17>
		_AppendKeyValueString( inLocaleString, sizeof(inLocaleString), varKeyValueString );
		_AppendKeyValueString( inLocaleString, sizeof(inLocaleString), keyValueString );
        
        // All done, return what we came up with.
        outStringRef = RSStringCreateWithCString(allocator, inLocaleString, RSStringEncodingASCII);
    }
    
    return outStringRef;
}

RSStringRef RSLocaleCreateCanonicalLocaleIdentifierFromString(RSAllocatorRef allocator, RSStringRef localeIdentifier) {
    char            inLocaleString[kLocaleIdentifierCStringMax];
    RSStringRef     outStringRef = NULL;
    
    if ( localeIdentifier && 0 <= RSStringGetCString(localeIdentifier, inLocaleString,  sizeof(inLocaleString), RSStringEncodingASCII) ) {
        KeyStringToResultString     testEntry;
        KeyStringToResultString *   foundEntry;
        char                        keyValueString[sizeof(inLocaleString)];				// <1.10>
        char			    		varKeyValueString[sizeof(inLocaleString)];			// <1.17>
        
        _GetKeyValueString(inLocaleString, keyValueString);								// <1.10>
        testEntry.result = NULL;
        
        // A. First check if input string matches an old-style Apple string that has a replacement
        // (do this before case normalization)
        testEntry.key = inLocaleString;
        foundEntry = (KeyStringToResultString *)bsearch( &testEntry, oldAppleLocaleToCanonical, kNumOldAppleLocaleToCanonical,
                                                        sizeof(KeyStringToResultString), _CompareTestEntryToTableEntryKey );
        if (foundEntry) {
            // It does match, so replace old string with new                                // <1.10>
            strlcpy(inLocaleString, foundEntry->result, sizeof(inLocaleString));
            varKeyValueString[0] = 0;
        } else {
            char *      langRegSubtag = NULL;
            char *      regionTag = NULL;
            
            // B. No match with an old-style string, use input string but update codes, normalize case, etc.
            _UpdateFullLocaleString(inLocaleString, sizeof(inLocaleString), &langRegSubtag, &regionTag, varKeyValueString);   // <1.10><1.17>
            
            
            // C. Now strip defaults that are implied by other fields.
            
            // 1. If an ISO 3166 region tag matches an ISO 3166 regional language variant subtag, strip the latter.
            if ( langRegSubtag && regionTag && strncmp(langRegSubtag+1, regionTag+1, 2) == 0 ) {
                _DeleteCharsAtPointer(langRegSubtag, 3);
            }
            
            // 2. Strip defaults in input string based on final region tag in locale string
            // (mainly for Chinese, to strip -Hans for _CN/_SG, -Hant for _TW/_HK/_MO)
            if ( regionTag ) {
                testEntry.key = regionTag;
                foundEntry = (KeyStringToResultString *)bsearch( &testEntry, localeStringRegionToDefaults, kNumLocaleStringRegionToDefaults,
                                                                sizeof(KeyStringToResultString), _CompareTestEntryToTableEntryKey );
                if (foundEntry) {
                    _RemoveSubstringsIfPresent(inLocaleString, foundEntry->result);
                }
            }
            
            // 3. Strip defaults in input string based on initial part of locale string
            // (mainly to strip default script tag for a language)
            testEntry.key = inLocaleString;
            foundEntry = (KeyStringToResultString *)bsearch( &testEntry, localeStringPrefixToDefaults, kNumLocaleStringPrefixToDefaults,
                                                            sizeof(KeyStringToResultString), _CompareTestEntryPrefixToTableEntryKey );
            if (foundEntry) {
                // The input string begins with a character sequence for which
                // there are default substrings which should be stripped if present
                _RemoveSubstringsIfPresent(inLocaleString, foundEntry->result);
            }
        }
        
        // D. Re-append any key-value strings, now canonical								// <1.10><1.17>
		_AppendKeyValueString( inLocaleString, sizeof(inLocaleString), varKeyValueString );
		_AppendKeyValueString( inLocaleString, sizeof(inLocaleString), keyValueString );
        
        // Now create the RSString (even if empty!)
        outStringRef = RSStringCreateWithCString(allocator, inLocaleString, RSStringEncodingASCII);
    }
    
    return outStringRef;
}

// RSLocaleCreateCanonicalLocaleIdentifierFromScriptManagerCodes, based on
// the first part of the SPI RSBundleCopyLocalizationForLocalizationInfo in RSBundle_Resources.c
extern RSStringRef  RSStringCreateWithCStringNoCopy(RSAllocatorRef alloc, const char *cStr, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator);
RSStringRef RSLocaleCreateCanonicalLocaleIdentifierFromScriptManagerCodes(RSAllocatorRef allocator, LangCode lcode, RegionCode rcode) {
    RSStringRef result = NULL;
    if (0 <= rcode && rcode < kNumRegionCodeToLocaleString) {
        const char *localeString = regionCodeToLocaleString[rcode];
        if (localeString != NULL && *localeString != '\0') {
            result = RSStringCreateWithCStringNoCopy(allocator, localeString, RSStringEncodingASCII, nil);
        }
    }
    if (result) return result;
    if (0 <= lcode && lcode < kNumLangCodeToLocaleString) {
        const char *localeString = langCodeToLocaleString[lcode];
        if (localeString != NULL && *localeString != '\0') {
            result = RSStringCreateWithCStringNoCopy(allocator, localeString, RSStringEncodingASCII, nil);
        }
    }
    return result;
}


/*
 SPI:  RSLocaleGetLanguageRegionEncodingForLocaleIdentifier gets the appropriate language and region codes,
 and the default legacy script code and encoding, for the specified locale (or language) string.
 Returns NO if RSLocale has no information about the given locale (in which case none of the by-reference return values are set);
 otherwise may set *langCode and/or *regCode to -1 if there is no appropriate legacy value for the locale.
 This is a replacement for the RSBundle SPI RSBundleGetLocalizationInfoForLocalization (which was intended to be temporary and transitional);
 this function is more up-to-date in its handling of locale strings, and is in RSLocale where this functionality should belong. Compared
 to RSBundleGetLocalizationInfoForLocalization, this function does not spcially interpret a NULL localeIdentifier to mean use the single most
 preferred localization in the current context (this function returns NO for a NULL localeIdentifier); and in this function
 langCode, regCode, and scriptCode are all SInt16* (not SInt32* like the equivalent parameters in RSBundleGetLocalizationInfoForLocalization).
 */
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
static int CompareLocaleToLegacyCodesEntries( const void *entry1, const void *entry2 );
#endif

BOOL RSLocaleGetLanguageRegionEncodingForLocaleIdentifier(RSStringRef localeIdentifier, LangCode *langCode, RegionCode *regCode, ScriptCode *scriptCode, RSStringEncoding *stringEncoding) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
	BOOL		returnValue = NO;
	RSStringRef	canonicalIdentifier = RSLocaleCreateCanonicalLocaleIdentifierFromString(NULL, localeIdentifier);
	if (canonicalIdentifier)
    {
    	char	localeCString[kLocaleIdentifierCStringMax];
		if ( 0 <= RSStringGetCString(canonicalIdentifier, localeCString,  sizeof(localeCString), RSStringEncodingASCII) )
        {
			UErrorCode	icuStatus = U_ZERO_ERROR;
			int32_t		languagelength;
			char		searchString[ULOC_LANG_CAPACITY + ULOC_FULLNAME_CAPACITY];
			
			languagelength = uloc_getLanguage( localeCString, searchString, ULOC_LANG_CAPACITY, &icuStatus );
			if ( U_SUCCESS(icuStatus) && languagelength > 0 )
            {
				// OK, here we have at least a language code, check for other components in order
				LocaleToLegacyCodes			searchEntry = { (const char *)searchString, 0, 0, 0 };
				const LocaleToLegacyCodes *	foundEntryPtr;
				int32_t						componentLength;
				char						componentString[ULOC_FULLNAME_CAPACITY];
				
				languagelength = (int)strlen(searchString);	// in case it got truncated
				icuStatus = U_ZERO_ERROR;
				componentLength = uloc_getScript( localeCString, componentString, sizeof(componentString), &icuStatus );
				if ( U_FAILURE(icuStatus) || componentLength == 0 )
                {
					icuStatus = U_ZERO_ERROR;
					componentLength = uloc_getCountry( localeCString, componentString, sizeof(componentString), &icuStatus );
					if ( U_FAILURE(icuStatus) || componentLength == 0 )
                    {
						icuStatus = U_ZERO_ERROR;
						componentLength = uloc_getVariant( localeCString, componentString, sizeof(componentString), &icuStatus );
						if ( U_FAILURE(icuStatus) )
                        {
							componentLength = 0;
						}
					}
				}
				
				// Append whichever other component we first found
				if (componentLength > 0)
                {
					strlcat(searchString, "_", sizeof(searchString));
					strlcat(searchString, componentString, sizeof(searchString));
				}
				
				// Search
				foundEntryPtr = (const LocaleToLegacyCodes *)bsearch( &searchEntry, localeToLegacyCodes, kNumLocaleToLegacyCodes, sizeof(LocaleToLegacyCodes), CompareLocaleToLegacyCodesEntries );
				if (foundEntryPtr == NULL && (int32_t) strlen(searchString) > languagelength)
                {
					// truncate to language al;one and try again
					searchString[languagelength] = 0;
					foundEntryPtr = (const LocaleToLegacyCodes *)bsearch( &searchEntry, localeToLegacyCodes, kNumLocaleToLegacyCodes, sizeof(LocaleToLegacyCodes), CompareLocaleToLegacyCodesEntries );
				}
                
				// If found a matching entry, return requested values
				if (foundEntryPtr)
                {
					returnValue = YES;
					if (langCode)		*langCode		= foundEntryPtr->langCode;
					if (regCode)		*regCode		= foundEntryPtr->regCode;
					if (stringEncoding)	*stringEncoding	= foundEntryPtr->encoding;
					if (scriptCode)
                    {
						// map RSStringEncoding to ScriptCode
						if (foundEntryPtr->encoding < 33/*RSStringEncodingMacSymbol*/)
                        {
							*scriptCode	= foundEntryPtr->encoding;
						}
                        else
                        {
							switch (foundEntryPtr->encoding)
                            {
								case 0x8C/*RSStringEncodingMacFarsi*/:		*scriptCode	= 4/*smArabic*/; break;
								case 0x98/*RSStringEncodingMacUkrainian*/:	*scriptCode	= 7/*smCyrillic*/; break;
								case 0xEC/*RSStringEncodingMacInuit*/:		*scriptCode	= 28/*smEthiopic*/; break;
								case 0xFC/*RSStringEncodingMacVT100*/:		*scriptCode	= 32/*smUninterp*/; break;
								default:									*scriptCode	= 0/*smRoman*/; break;
							}
						}
					}
				}
			}
		}
		RSRelease(canonicalIdentifier);
	}
	return returnValue;
#else
    return NO;
#endif
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
static int CompareLocaleToLegacyCodesEntries( const void *entry1, const void *entry2 ) {
	const char *	localeString1 = ((const LocaleToLegacyCodes *)entry1)->locale;
	const char *	localeString2 = ((const LocaleToLegacyCodes *)entry2)->locale;
	return strcmp(localeString1, localeString2);
}
#endif


RSDictionaryRef RSLocaleCreateComponentsFromLocaleIdentifier(RSAllocatorRef allocator, RSStringRef localeID) {
    RSMutableDictionaryRef working = RSDictionaryCreateMutable(allocator, 10, RSDictionaryRSTypeContext);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    char cLocaleID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    char buffer[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    
    UErrorCode icuStatus = U_ZERO_ERROR;
    int32_t length = 0;
    
    if (!localeID) goto out;
    
    // Extract the C string locale ID, for ICU
    RSIndex outBytes = 0;
    RSStringGetBytes(localeID, RSMakeRange(0, RSStringGetLength(localeID)), RSStringEncodingASCII, (UInt8) '?', YES, (unsigned char *)cLocaleID, sizeof(cLocaleID)/sizeof(char) - 1, &outBytes);
    cLocaleID[outBytes] = '\0';
    
    // Get the components
    length = uloc_getLanguage(cLocaleID, buffer, sizeof(buffer)/sizeof(char), &icuStatus);
    if (U_SUCCESS(icuStatus) && length > 0)
    {
        RSStringRef string = RSStringCreateWithBytes(allocator, (UInt8 *)buffer, length, RSStringEncodingASCII, YES);
        RSDictionarySetValue(working, RSLocaleLanguageCodeKey, string);
        RSRelease(string);
    }
    icuStatus = U_ZERO_ERROR;
    
    length = uloc_getScript(cLocaleID, buffer, sizeof(buffer)/sizeof(char), &icuStatus);
    if (U_SUCCESS(icuStatus) && length > 0)
    {
        RSStringRef string = RSStringCreateWithBytes(allocator, (UInt8 *)buffer, length, RSStringEncodingASCII, YES);
        RSDictionarySetValue(working, RSLocaleScriptCodeKey, string);
        RSRelease(string);
    }
    icuStatus = U_ZERO_ERROR;
    
    length = uloc_getCountry(cLocaleID, buffer, sizeof(buffer)/sizeof(char), &icuStatus);
    if (U_SUCCESS(icuStatus) && length > 0)
    {
        RSStringRef string = RSStringCreateWithBytes(allocator, (UInt8 *)buffer, length, RSStringEncodingASCII, YES);
        RSDictionarySetValue(working, RSLocaleCountryCodeKey, string);
        RSRelease(string);
    }
    icuStatus = U_ZERO_ERROR;
    
    length = uloc_getVariant(cLocaleID, buffer, sizeof(buffer)/sizeof(char), &icuStatus);
    if (U_SUCCESS(icuStatus) && length > 0)
    {
        RSStringRef string = RSStringCreateWithBytes(allocator, (UInt8 *)buffer, length, RSStringEncodingASCII, YES);
        RSDictionarySetValue(working, RSLocaleVariantCodeKey, string);
        RSRelease(string);
    }
    icuStatus = U_ZERO_ERROR;
    
    // Now get the keywords; open an enumerator on them
    UEnumeration *iter = uloc_openKeywords(cLocaleID, &icuStatus);
    const char *locKey = NULL;
    int32_t locKeyLen = 0;
    while ((locKey = uenum_next(iter, &locKeyLen, &icuStatus)) && U_SUCCESS(icuStatus))
    {
        char locValue[ULOC_KEYWORD_AND_VALUES_CAPACITY];
        
        // Get the value for this keyword
        if (uloc_getKeywordValue(cLocaleID, locKey, locValue, sizeof(locValue)/sizeof(char), &icuStatus) > 0
            && U_SUCCESS(icuStatus))
        {
            RSStringRef key = RSStringCreateWithBytes(allocator, (UInt8 *)locKey, strlen(locKey), RSStringEncodingASCII, YES);
            RSStringRef value = RSStringCreateWithBytes(allocator, (UInt8 *)locValue, strlen(locValue), RSStringEncodingASCII, YES);
            if (key && value)
                RSDictionarySetValue(working, key, value);
            if (key)
                RSRelease(key);
            if (value)
                RSRelease(value);
        }
    }
    uenum_close(iter);
    
out:;
#endif
    // Convert to an immutable dictionary and return
    RSDictionaryRef result = RSCopy(allocator, working);
    RSRelease(working);
    return result;
}

static char *__CStringFromString(RSStringRef str) {
    if (!str) return NULL;
    RSRange rg = RSMakeRange(0, RSStringGetLength(str));
    RSIndex neededLength = 0;
    RSStringGetBytes(str, rg, RSStringEncodingASCII, (UInt8)'?', NO, NULL, 0, &neededLength);
    char *buf = (char *)malloc(neededLength + 1);
    RSStringGetBytes(str, rg, RSStringEncodingASCII, (UInt8)'?', NO, (uint8_t *)buf, neededLength, &neededLength);
    buf[neededLength] = '\0';
    return buf;
}

RSStringRef RSLocaleCreateLocaleIdentifierFromComponents(RSAllocatorRef allocator, RSDictionaryRef dictionary) {
    if (!dictionary) return NULL;
    
    RSIndex cnt = RSDictionaryGetCount(dictionary);
    STACK_BUFFER_DECL(RSStringRef, values, cnt);
    STACK_BUFFER_DECL(RSStringRef, keys, cnt);
    RSDictionaryGetKeysAndValues(dictionary, (const void **)&keys, (const void **)&values);
    
    char *language = NULL, *script = NULL, *country = NULL, *variant = NULL;
    for (RSIndex idx = 0; idx < cnt; idx++) {
        if (RSEqual(RSLocaleLanguageCodeKey, keys[idx])) {
            language = __CStringFromString(values[idx]);
            keys[idx] = NULL;
        } else if (RSEqual(RSLocaleScriptCodeKey, keys[idx])) {
            script = __CStringFromString(values[idx]);
            keys[idx] = NULL;
        } else if (RSEqual(RSLocaleCountryCodeKey, keys[idx])) {
            country = __CStringFromString(values[idx]);
            keys[idx] = NULL;
        } else if (RSEqual(RSLocaleVariantCodeKey, keys[idx])) {
            variant = __CStringFromString(values[idx]);
            keys[idx] = NULL;
        }
    }
    
    char *buf1 = NULL;	// (|L)(|_S)(|_C|_C_V|__V)
    asprintf(&buf1, "%s%s%s%s%s%s%s", language ? language : "", script ? "_" : "", script ? script : "", (country || variant ? "_" : ""), country ? country : "", variant ? "_" : "", variant ? variant : "");
    
    char cLocaleID[2 * ULOC_FULLNAME_CAPACITY + 2 * ULOC_KEYWORD_AND_VALUES_CAPACITY];
    strlcpy(cLocaleID, buf1, sizeof(cLocaleID));
    free(language);
    free(script);
    free(country);
    free(variant);
    free(buf1);
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    for (RSIndex idx = 0; idx < cnt; idx++) {
        if (keys[idx]) {
            char *key = __CStringFromString(keys[idx]);
            char *value;
            if (0 == strcmp(key, "RSLocaleCalendarKey")) {
                // For interchangeability convenience, we alternatively allow a
                // calendar object to be passed in, with the alternate key, and
                // we'll extract the identifier.
                RSCalendarRef cal = (RSCalendarRef)values[idx];
                RSStringRef ident = (RSStringRef)RSCalendarGetIdentifier(cal);
                value = __CStringFromString(ident);
                char *oldkey = key;
                key = strdup("calendar");
                free(oldkey);
            } else {
                value = __CStringFromString(values[idx]);
            }
            UErrorCode status = U_ZERO_ERROR;
            uloc_setKeywordValue(key, value, cLocaleID, sizeof(cLocaleID), &status);
            free(key);
            free(value);
        }
    }
#endif
    
    return RSStringCreateWithCString(allocator, cLocaleID, RSStringEncodingASCII);
}


static RSLocaleRef __RSLocaleSystemId = NULL;
static RSMutableDictionaryRef __RSLocaleCache = NULL;
static RSSpinLock __RSLocaleGlobalLock = RSSpinLockInit;

struct __RSLocale
{
    RSRuntimeBase _base;
    RSStringRef _identifier;
    RSMutableDictionaryRef _cache;
    RSMutableDictionaryRef _overrides;
    RSDictionaryRef _prefs;
    RSSpinLock _lock;
    BOOL _nullLocale;
};

typedef struct __RSLocale* __RSLocaleRef;

/* Flag bits */
enum {      /* Bits 0-1 */
    __RSLocaleOrdinary = 0,
    __RSLocaleSystem = 1,
    __RSLocaleUser = 2,
    __RSLocaleCustom = 3
};


__private_extern__ BOOL __RSLocaleGetNullLocale(struct __RSLocale *locale) {
    return locale->_nullLocale;
}

__private_extern__ void __RSLocaleSetNullLocale(struct __RSLocale *locale) {
    locale->_nullLocale = YES;
}

RSInline RSIndex __RSLocaleGetType(RSLocaleRef locale) {
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(locale), 1, 0);
}

RSInline void __RSLocaleSetType(RSLocaleRef locale, RSIndex type) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(locale), 1, 0, (uint8_t)type);
}

RSInline void __RSLocaleLockGlobal(void) {
    RSSpinLockLock(&__RSLocaleGlobalLock);
}

RSInline void __RSLocaleUnlockGlobal(void) {
    RSSpinLockUnlock(&__RSLocaleGlobalLock);
}

RSInline void __RSLocaleLock(RSLocaleRef locale) {
    RSSpinLockLock(&((struct __RSLocale *)locale)->_lock);
}

RSInline void __RSLocaleUnlock(RSLocaleRef locale) {
    RSSpinLockUnlock(&((struct __RSLocale *)locale)->_lock);
}


static void __RSLocaleClassInit(RSTypeRef rs)
{
    
}

static RSTypeRef __RSLocaleClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSLocaleClassDeallocate(RSTypeRef rs)
{
    __RSLocaleRef locale = (__RSLocaleRef)rs;
    RSRelease(locale->_identifier);
    if (NULL != locale->_cache) RSRelease(locale->_cache);
    if (NULL != locale->_overrides) RSRelease(locale->_overrides);
    if (NULL != locale->_prefs) RSRelease(locale->_prefs);}

static BOOL __RSLocaleClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSLocaleRef RSLocale1 = (RSLocaleRef)rs1;
    RSLocaleRef RSLocale2 = (RSLocaleRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSLocale1->_identifier, RSLocale2->_identifier);
    
    return result;
}

static RSHashCode __RSLocaleClassHash(RSTypeRef rs)
{
    return RSHash(((RSLocaleRef)rs)->_identifier);
}

static RSStringRef __RSLocaleClassDescription(RSTypeRef rs)
{
    RSLocaleRef locale = (RSLocaleRef)rs;
    const char *type = NULL;
    switch (__RSLocaleGetType(locale)) {
        case __RSLocaleOrdinary: type = "ordinary"; break;
        case __RSLocaleSystem: type = "system"; break;
        case __RSLocaleUser: type = "user"; break;
        case __RSLocaleCustom: type = "custom"; break;
    }
    return RSStringCreateWithFormat(RSGetAllocator(locale), NULL, RSSTR("<RSLocale %p [%p]>{type = %s, identifier = '%r'}"), rs, RSGetAllocator(locale), type, locale->_identifier);
}

static RSRuntimeClass __RSLocaleClass =
{
    _RSRuntimeScannedObject,
    "RSLocale",
    __RSLocaleClassInit,
    __RSLocaleClassCopy,
    __RSLocaleClassDeallocate,
    __RSLocaleClassEqual,
    __RSLocaleClassHash,
    __RSLocaleClassDescription,
    nil,
    nil
};

static BOOL __RSLocaleEqual(RSTypeRef cf1, RSTypeRef cf2) {
    RSLocaleRef locale1 = (RSLocaleRef)cf1;
    RSLocaleRef locale2 = (RSLocaleRef)cf2;
    // a user locale and a locale created with an ident are not the same even if their contents are
    if (__RSLocaleGetType(locale1) != __RSLocaleGetType(locale2)) return NO;
    if (!RSEqual(locale1->_identifier, locale2->_identifier)) return NO;
    if (NULL == locale1->_overrides && NULL != locale2->_overrides) return NO;
    if (NULL != locale1->_overrides && NULL == locale2->_overrides) return NO;
    if (NULL != locale1->_overrides && !RSEqual(locale1->_overrides, locale2->_overrides)) return NO;
    if (__RSLocaleUser == __RSLocaleGetType(locale1)) {
        return RSEqual(locale1->_prefs, locale2->_prefs);
    }
    return YES;
}

static RSHashCode __RSLocaleHash(RSTypeRef cf) {
    RSLocaleRef locale = (RSLocaleRef)cf;
    return RSHash(locale->_identifier);
}

static RSTypeID _RSLocaleTypeID = _RSRuntimeNotATypeID;

static void __RSLocaleInitialize(void) {
    RSIndex idx;
    _RSLocaleTypeID = __RSRuntimeRegisterClass(&__RSLocaleClass);
    for (idx = 0; idx < __RSLocaleKeyTableCount; idx++) {
        // table fixup to workaround compiler/language limitations
        __RSLocaleKeyTable[idx].key = *((RSStringRef *)__RSLocaleKeyTable[idx].key);
        if (NULL != __RSLocaleKeyTable[idx].context) {
            __RSLocaleKeyTable[idx].context = *((RSStringRef *)__RSLocaleKeyTable[idx].context);
        }
    }
}


RSExport RSTypeID RSLocaleGetTypeID()
{
    if (_RSLocaleTypeID == _RSRuntimeNotATypeID) __RSLocaleInitialize();
    return _RSLocaleTypeID;
}

RSPrivate void __RSLocaleDeallocate()
{
//
}

static RSLocaleRef __RSLocaleCreateInstance(RSAllocatorRef allocator, RSStringRef identifier)
{
    __RSLocaleRef instance = (__RSLocaleRef)__RSRuntimeCreateInstance(allocator, _RSLocaleTypeID, sizeof(struct __RSLocale) - sizeof(RSRuntimeBase));
    
    instance->_identifier = RSRetain(identifier);
    
    return instance;
}


RSLocaleRef RSLocaleGetSystem(void) {
    RSLocaleRef locale;
    __RSLocaleLockGlobal();
    if (NULL == __RSLocaleSystemId) {
        __RSLocaleUnlockGlobal();
        locale = RSLocaleCreate(RSAllocatorSystemDefault, RSSTR(""));
        if (!locale) return NULL;
        __RSLocaleSetType(locale, __RSLocaleSystem);
        __RSLocaleLockGlobal();
        if (NULL == __RSLocaleSystemId) {
            __RSLocaleSystemId = locale;
        } else {
            if (locale) RSRelease(locale);
        }
    }
    locale = __RSLocaleSystem ? (RSLocaleRef)RSRetain(__RSLocaleSystem) : NULL;
    __RSLocaleUnlockGlobal();
    return locale;
}

extern RSDictionaryRef __RSXPreferencesCopyCurrentApplicationState(void);

static RSLocaleRef __RSLocaleCurrent = NULL;


#if DEPLOYMENT_TARGET_MACOSX
#define FALLBACK_LOCALE_NAME RSSTR("")
#elif DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define FALLBACK_LOCALE_NAME RSSTR("en_US")
#elif DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
#define FALLBACK_LOCALE_NAME RSSTR("en_US")
#endif

RSLocaleRef RSLocaleCopyCurrent(void)
{
    
    RSStringRef name = NULL, ident = NULL;
    // We cannot be helpful here, because it causes performance problems,
    // even though the preference lookup is relatively quick, as there are
    // things which call this function thousands or millions of times in
    // a short period.
#if 0 // DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    name = (RSStringRef)RSPreferencesCopyAppValue(RSSTR("AppleLocale"), RSPreferencesCurrentApplication);
#endif
    if (name && (RSStringGetTypeID() == RSGetTypeID(name))) {
        ident = RSLocaleCreateCanonicalLocaleIdentifierFromString(RSAllocatorSystemDefault, name);
    }
    if (name) RSRelease(name);
    RSLocaleRef oldLocale = NULL;
    __RSLocaleLockGlobal();
    if (__RSLocaleCurrent)
    {
        if (ident && !RSEqual(__RSLocaleCurrent->_identifier, ident))
        {
            oldLocale = __RSLocaleCurrent;
            __RSLocaleCurrent = NULL;
        }
        else
        {
            RSLocaleRef res = __RSLocaleCurrent;
            RSRetain(res);
            __RSLocaleUnlockGlobal();
            if (ident) RSRelease(ident);
            return res;
        }
    }
    __RSLocaleUnlockGlobal();
    if (oldLocale) RSRelease(oldLocale);
    if (ident) RSRelease(ident);
    // We could *probably* re-use ident down below, but that would't
    // get us out of querying RSPrefs for the current application state.
    
    RSDictionaryRef prefs = NULL;
    RSStringRef identifier = NULL;
    
    struct __RSLocale *locale;
    uint32_t size = sizeof(struct __RSLocale) - sizeof(RSRuntimeBase);
    locale = (struct __RSLocale *)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, RSLocaleGetTypeID(), size);
    if (NULL == locale) {
        if (prefs) RSRelease(prefs);
        if (identifier) RSRelease(identifier);
        return NULL;
    }
    __RSLocaleSetType(locale, __RSLocaleUser);
    if (NULL == identifier) identifier = (RSStringRef)RSRetain(FALLBACK_LOCALE_NAME);
    locale->_identifier = identifier;
    locale->_cache = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    locale->_overrides = NULL;
    locale->_prefs = prefs;
    locale->_lock = RSSpinLockInit;
    locale->_nullLocale = NO;
    
    __RSLocaleLockGlobal();
    if (NULL == __RSLocaleCurrent) {
        __RSLocaleCurrent = locale;
    } else {
        RSRelease(locale);
    }
    locale = (struct __RSLocale *)RSRetain(__RSLocaleCurrent);
    __RSLocaleUnlockGlobal();
    return locale;
}

__private_extern__ RSDictionaryRef __RSLocaleGetPrefs(RSLocaleRef locale) {
    RS_OBJC_FUNCDISPATCHV(RSLocaleGetTypeID(), RSDictionaryRef, (NSLocale *)locale, _prefs);
    return locale->_prefs;
}

RSExport RSLocaleRef RSLocaleCreate(RSAllocatorRef allocator, RSStringRef identifier)
{
    
    __RSGenericValidInstance(allocator, RSAllocatorGetTypeID());
    __RSGenericValidInstance(identifier, RSStringGetTypeID());
    RSStringRef localeIdentifier = NULL;
    if (identifier) {
        localeIdentifier = RSLocaleCreateCanonicalLocaleIdentifierFromString(allocator, identifier);
    }
    if (NULL == localeIdentifier) return NULL;
    RSStringRef old = localeIdentifier;
    localeIdentifier = (RSStringRef)RSCopy(allocator, localeIdentifier);
    RSRelease(old);
    __RSLocaleLockGlobal();
    // Look for cases where we can return a cached instance.
    // We only use cached objects if the allocator is the system
    // default allocator.
    BOOL canCache = (allocator == RSAllocatorSystemDefault);
    if (canCache && __RSLocaleCache) {
        RSLocaleRef locale = (RSLocaleRef)RSDictionaryGetValue(__RSLocaleCache, localeIdentifier);
        if (locale) {
            RSRetain(locale);
            __RSLocaleUnlockGlobal();
            RSRelease(localeIdentifier);
            return locale;
        }
    }
    struct __RSLocale *locale = NULL;
    uint32_t size = sizeof(struct __RSLocale) - sizeof(RSRuntimeBase);
    locale = (struct __RSLocale *)__RSRuntimeCreateInstance(allocator, RSLocaleGetTypeID(), size);
    if (NULL == locale) {
        return NULL;
    }
    __RSLocaleSetType(locale, __RSLocaleOrdinary);
    locale->_identifier = localeIdentifier;
    locale->_cache = RSDictionaryCreateMutable(allocator, 0, RSDictionaryRSTypeContext);// key context may be nil
    locale->_overrides = NULL;
    locale->_prefs = NULL;
    locale->_lock = RSSpinLockInit;
    if (canCache) {
        if (NULL == __RSLocaleCache) {
            __RSLocaleCache = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        }
        RSDictionarySetValue(__RSLocaleCache, localeIdentifier, locale);
    }
    __RSLocaleUnlockGlobal();
    return (RSLocaleRef)locale;
}

RSLocaleRef RSLocaleCreateCopy(RSAllocatorRef allocator, RSLocaleRef locale) {
    return (RSLocaleRef)RSRetain(locale);
}

RSStringRef RSLocaleGetIdentifier(RSLocaleRef locale) {
    RS_OBJC_FUNCDISPATCHV(RSLocaleGetTypeID(), RSStringRef, (NSLocale *)locale, localeIdentifier);
    return locale->_identifier;
}

RSExport RSTypeRef RSLocaleGetValue(RSLocaleRef locale, RSStringRef key) {
#if DEPLOYMENT_TARGET_MACOSX
    if (!_RSExecutableLinkedOnOrAfter(RSSystemVersionSnowLeopard)) {
        // Hack for Opera, which is using the hard-coded string value below instead of
        // the perfectly good public RSLocaleCountryCode constant, for whatever reason.
        if (key && RSEqual(key, RSSTR("locale:country code"))) {
            key = (RSStringRef)RSLocaleCountryCodeKey;
        }
    }
#endif
    RS_OBJC_FUNCDISPATCHV(RSLocaleGetTypeID(), RSTypeRef, (NSLocale *)locale, objectForKey:(id)key);
    RSIndex idx, slot = -1;
    for (idx = 0; idx < __RSLocaleKeyTableCount; idx++) {
        if (__RSLocaleKeyTable[idx].key == key) {
            slot = idx;
            break;
        }
    }
    if (-1 == slot && NULL != key) {
        for (idx = 0; idx < __RSLocaleKeyTableCount; idx++) {
            if (RSEqual(__RSLocaleKeyTable[idx].key, key)) {
                slot = idx;
                break;
            }
        }
    }
    if (-1 == slot) {
        return NULL;
    }
    RSTypeRef value;
    if (NULL != locale->_overrides && (value = RSDictionaryGetValue(locale->_overrides, __RSLocaleKeyTable[slot].key))) {
        return value;
    }
    __RSLocaleLock(locale);
    if ((value = RSDictionaryGetValue(locale->_cache, __RSLocaleKeyTable[slot].key))) {
        __RSLocaleUnlock(locale);
        return value;
    }
    if (__RSLocaleUser == __RSLocaleGetType(locale) && __RSLocaleKeyTable[slot].get(locale, YES, &value, __RSLocaleKeyTable[slot].context)) {
        if (value) RSDictionarySetValue(locale->_cache, __RSLocaleKeyTable[idx].key, value);
        if (value) RSRelease(value);
        __RSLocaleUnlock(locale);
        return value;
    }
    if (__RSLocaleKeyTable[slot].get(locale, NO, &value, __RSLocaleKeyTable[slot].context)) {
        if (value) RSDictionarySetValue(locale->_cache, __RSLocaleKeyTable[idx].key, value);
        if (value) RSRelease(value);
        __RSLocaleUnlock(locale);
        return value;
    }
    __RSLocaleUnlock(locale);
    return NULL;
}

RSStringRef RSLocaleCopyDisplayNameForPropertyValue(RSLocaleRef displayLocale, RSStringRef key, RSStringRef value) {
    RS_OBJC_FUNCDISPATCHV(RSLocaleGetTypeID(), RSStringRef, (NSLocale *)displayLocale, _copyDisplayNameForKey:(id)key value:(id)value);
    RSIndex idx, slot = -1;
    for (idx = 0; idx < __RSLocaleKeyTableCount; idx++) {
        if (__RSLocaleKeyTable[idx].key == key) {
            slot = idx;
            break;
        }
    }
    if (-1 == slot && NULL != key) {
        for (idx = 0; idx < __RSLocaleKeyTableCount; idx++) {
            if (RSEqual(__RSLocaleKeyTable[idx].key, key)) {
                slot = idx;
                break;
            }
        }
    }
    if (-1 == slot || !value) {
        return NULL;
    }
    // Get the locale ID as a C string
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    char cValue[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    if (0 <= RSStringGetCString(displayLocale->_identifier, localeID, sizeof(localeID)/sizeof(localeID[0]), RSStringEncodingASCII) && 0 <= RSStringGetCString(value, cValue, sizeof(cValue)/sizeof(char), RSStringEncodingASCII))
    {
        RSStringRef result;
        if ((NULL == displayLocale->_prefs) && __RSLocaleKeyTable[slot].name(localeID, cValue, &result)) {
            return result;
        }
        
        // We could not find a result using the requested language. Fall back through all preferred languages.
        RSArrayRef langPref = NULL;
        if (displayLocale->_prefs) {
            langPref = (RSArrayRef)RSDictionaryGetValue(displayLocale->_prefs, RSSTR("AppleLanguages"));
            if (langPref) RSRetain(langPref);
        } else {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//            langPref = (RSArrayRef)RSPreferencesCopyAppValue(RSSTR("AppleLanguages"), RSPreferencesCurrentApplication);
#endif
        }
        if (langPref != NULL) {
            RSIndex count = RSArrayGetCount(langPref);
            RSIndex i;
            bool success = NO;
            for (i = 0; i < count && !success; ++i) {
                RSStringRef language = (RSStringRef)RSArrayObjectAtIndex(langPref, i);
                RSStringRef cleanLanguage = RSLocaleCreateCanonicalLanguageIdentifierFromString(RSAllocatorSystemDefault, language);
                if (0 <= RSStringGetCString(cleanLanguage, localeID, sizeof(localeID)/sizeof(localeID[0]), RSStringEncodingASCII)) {
                    success = __RSLocaleKeyTable[slot].name(localeID, cValue, &result);
                }
                RSRelease(cleanLanguage);
            }
            RSRelease(langPref);
            if (success)
                return result;
        }
    }
    return NULL;
}

RSArrayRef RSLocaleCopyAvailableLocaleIdentifiers(void) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    int32_t locale, localeCount = uloc_countAvailable();
    RSMutableSetRef working = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    for (locale = 0; locale < localeCount; ++locale) {
        const char *localeID = uloc_getAvailable(locale);
        RSStringRef string1 = RSStringCreateWithCString(RSAllocatorSystemDefault, localeID, RSStringEncodingASCII);
        // do not include canonicalized version as IntlFormats cannot cope with that in its popup
        RSSetAddValue(working, string1);
        RSRelease(string1);
    }
    RSIndex cnt = RSSetGetCount(working);
    STACK_BUFFER_DECL(const void *, buffer, cnt);
    RSSetGetValues(working, buffer);
    RSArrayRef result = RSArrayCreateWithObjects(RSAllocatorSystemDefault, buffer, cnt);
    RSRelease(working);
    return result;
#else
    return RSArrayCreate(RSAllocatorSystemDefault, NULL, 0, &RSTypeArrayCallBacks);
#endif
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
static RSArrayRef __RSLocaleCopyCStringsAsArray(const char* const* p) {
    RSMutableArrayRef working = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    for (; *p; ++p) {
        RSStringRef string = RSStringCreateWithCString(RSAllocatorSystemDefault, *p, RSStringEncodingASCII);
        RSArrayAddObject(working, string);
        RSRelease(string);
    }
    RSArrayRef result = RSArrayCreateCopy(RSAllocatorSystemDefault, working);
    RSRelease(working);
    return result;
}

static RSArrayRef __RSLocaleCopyUEnumerationAsArray(UEnumeration *enumer, UErrorCode *icuErr) {
    const UChar *next = NULL;
    int32_t len = 0;
    RSMutableArrayRef working = NULL;
    if (U_SUCCESS(*icuErr)) {
        working = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    }
    while ((next = uenum_unext(enumer, &len, icuErr)) && U_SUCCESS(*icuErr)) {
        RSStringRef string = RSStringCreateWithCharacters(RSAllocatorSystemDefault, (const UniChar *)next, (RSIndex) len);
        RSArrayAddObject(working, string);
        RSRelease(string);
    }
    if (*icuErr == U_INDEX_OUTOFBOUNDS_ERROR) {
        *icuErr = U_ZERO_ERROR;      // Temp: Work around bug (ICU 5220) in ucurr enumerator
    }
    RSArrayRef result = NULL;
    if (U_SUCCESS(*icuErr)) {
        result = RSArrayCreateCopy(RSAllocatorSystemDefault, working);
    }
    if (working != NULL) {
        RSRelease(working);
    }
    return result;
}
#endif

RSArrayRef RSLocaleCopyISOLanguageCodes(void) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    const char* const* p = uloc_getISOLanguages();
    return __RSLocaleCopyCStringsAsArray(p);
#else
    return RSArrayCreate(RSAllocatorSystemDefault, NULL, 0, &RSTypeArrayCallBacks);
#endif
}

RSArrayRef RSLocaleCopyISOCountryCodes(void) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    const char* const* p = uloc_getISOCountries();
    return __RSLocaleCopyCStringsAsArray(p);
#else
    return RSArrayCreate(RSAllocatorSystemDefault, NULL, 0, &RSTypeArrayCallBacks);
#endif
}

RSArrayRef RSLocaleCopyISOCurrencyCodes(void) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    UErrorCode icuStatus = U_ZERO_ERROR;
    UEnumeration *enumer = ucurr_openISOCurrencies(UCURR_ALL, &icuStatus);
    RSArrayRef result = __RSLocaleCopyUEnumerationAsArray(enumer, &icuStatus);
    uenum_close(enumer);
#else
    RSArrayRef result = RSArrayCreate(RSAllocatorSystemDefault, NULL, 0, &RSTypeArrayCallBacks);
#endif
    return result;
}

RSArrayRef RSLocaleCopyCommonISOCurrencyCodes(void) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    UErrorCode icuStatus = U_ZERO_ERROR;
    UEnumeration *enumer = ucurr_openISOCurrencies(UCURR_COMMON|UCURR_NON_DEPRECATED, &icuStatus);
    RSArrayRef result = __RSLocaleCopyUEnumerationAsArray(enumer, &icuStatus);
    uenum_close(enumer);
#else
    RSArrayRef result = RSArrayCreate(RSAllocatorSystemDefault, NULL, 0, &RSTypeArrayCallBacks);
#endif
    return result;
}

RSStringRef RSLocaleCreateLocaleIdentifierFromWindowsLocaleCode(RSAllocatorRef allocator, uint32_t lcid) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    char buffer[kMaxICUNameSize];
    UErrorCode status = U_ZERO_ERROR;
    int32_t ret = uloc_getLocaleForLCID(lcid, buffer, kMaxICUNameSize, &status);
    if (U_FAILURE(status) || kMaxICUNameSize <= ret) return NULL;
    RSStringRef str = RSStringCreateWithCString(RSAllocatorSystemDefault, buffer, RSStringEncodingASCII);
    RSStringRef ident = RSLocaleCreateCanonicalLocaleIdentifierFromString(RSAllocatorSystemDefault, str);
    RSRelease(str);
    return ident;
#else
    return RSSTR("");
#endif
}

uint32_t RSLocaleGetWindowsLocaleCodeFromLocaleIdentifier(RSStringRef localeIdentifier) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    RSStringRef ident = RSLocaleCreateCanonicalLocaleIdentifierFromString(RSAllocatorSystemDefault, localeIdentifier);
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    BOOL b = ident ? RSStringGetCString(ident, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) : NO;
    if (ident) RSRelease(ident);
    return b >= 0 ? uloc_getLCID(localeID) : 0;
#else
    return 0;
#endif
}

RSLocaleLanguageDirection RSLocaleGetLanguageCharacterDirection(RSStringRef isoLangCode) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    BOOL b = isoLangCode ? RSStringGetCString(isoLangCode, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) : NO;
    RSLocaleLanguageDirection dir;
    UErrorCode status = U_ZERO_ERROR;
    ULayoutType idir = b >= 0 ? uloc_getCharacterOrientation(localeID, &status) : ULOC_LAYOUT_UNKNOWN;
    switch (idir) {
        case ULOC_LAYOUT_LTR: dir = RSLocaleLanguageDirectionLeftToRight; break;
        case ULOC_LAYOUT_RTL: dir = RSLocaleLanguageDirectionRightToLeft; break;
        case ULOC_LAYOUT_TTB: dir = RSLocaleLanguageDirectionTopToBottom; break;
        case ULOC_LAYOUT_BTT: dir = RSLocaleLanguageDirectionBottomToTop; break;
        default: dir = RSLocaleLanguageDirectionUnknown; break;
    }
    return dir;
#else
    return RSLocaleLanguageDirectionLeftToRight;
#endif
}

RSLocaleLanguageDirection RSLocaleGetLanguageLineDirection(RSStringRef isoLangCode) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    BOOL b = isoLangCode ? RSStringGetCString(isoLangCode, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) : NO;
    RSLocaleLanguageDirection dir;
    UErrorCode status = U_ZERO_ERROR;
    ULayoutType idir = b >= 0 ? uloc_getLineOrientation(localeID, &status) : ULOC_LAYOUT_UNKNOWN;
    switch (idir) {
        case ULOC_LAYOUT_LTR: dir = RSLocaleLanguageDirectionLeftToRight; break;
        case ULOC_LAYOUT_RTL: dir = RSLocaleLanguageDirectionRightToLeft; break;
        case ULOC_LAYOUT_TTB: dir = RSLocaleLanguageDirectionTopToBottom; break;
        case ULOC_LAYOUT_BTT: dir = RSLocaleLanguageDirectionBottomToTop; break;
        default: dir = RSLocaleLanguageDirectionUnknown; break;
    }
    return dir;
#else
    return RSLocaleLanguageDirectionLeftToRight;
#endif
}

RSArrayRef RSLocaleCopyPreferredLanguages(void) {
    RSMutableArrayRef newArray = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS
    RSArrayRef languagesArray = nil; // (RSArrayRef)RSPreferencesCopyAppValue(RSSTR("AppleLanguages"), RSPreferencesCurrentApplication);
    
    if (languagesArray && (RSArrayGetTypeID() == RSGetTypeID(languagesArray)))
    {
        for (RSIndex idx = 0, cnt = RSArrayGetCount(languagesArray); idx < cnt; idx++)
        {
            RSStringRef str = (RSStringRef)RSArrayObjectAtIndex(languagesArray, idx);
            if (str && (RSStringGetTypeID() == RSGetTypeID(str)))
            {
                RSStringRef ident = RSLocaleCreateCanonicalLanguageIdentifierFromString(RSAllocatorSystemDefault, str);
                RSArrayAddObject(newArray, ident);
                RSRelease(ident);
            }
        }
    }
    if (languagesArray)	RSRelease(languagesArray);
#endif
    return newArray;
}

// -------- -------- -------- -------- -------- --------

// These functions return YES or NO depending on the success or failure of the function.
// In the Copy case, this is failure to fill the *cf out parameter, and that out parameter is
// returned by reference WITH a retain on it.
static bool __RSLocaleSetNOP(RSMutableLocaleRef locale, RSTypeRef cf, RSStringRef context) {
    return NO;
}

static bool __RSLocaleCopyLocaleID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    *cf = RSRetain(locale->_identifier);
    return YES;
}


static bool __RSLocaleCopyCodes(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    RSDictionaryRef codes = NULL;
    // this access of _cache is protected by the lock in RSLocaleGetValue()
    if (!(codes = RSDictionaryGetValue(locale->_cache, RSSTR("__RSLocaleCodes"))))
    {
        codes = RSLocaleCreateComponentsFromLocaleIdentifier(RSAllocatorSystemDefault, locale->_identifier);
        if (codes) RSDictionarySetValue(locale->_cache, RSSTR("__RSLocaleCodes"), codes);
        if (codes) RSRelease(codes);
    }
    if (codes) {
        RSStringRef value = (RSStringRef)RSDictionaryGetValue(codes, context); // context is one of RSLocale*Code constants
        if (value) RSRetain(value);
        *cf = value;
        return YES;
    }
    return NO;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
RSCharacterSetRef _RSCreateCharacterSetFromUSet(USet *set)
{
    UErrorCode icuErr = U_ZERO_ERROR;
    RSMutableCharacterSetRef working = RSCharacterSetCreateMutable(NULL);
    UChar   buffer[2048];   // Suitable for most small sets
    int32_t stringLen;
    
    if (working == NULL)
        return NULL;
    
    int32_t itemCount = uset_getItemCount(set);
    int32_t i;
    for (i = 0; i < itemCount; ++i)
    {
        UChar32   start, end;
        UChar * string;
        
        string = buffer;
        stringLen = uset_getItem(set, i, &start, &end, buffer, sizeof(buffer)/sizeof(UChar), &icuErr);
        if (icuErr == U_BUFFER_OVERFLOW_ERROR)
        {
            string = (UChar *) malloc(sizeof(UChar)*(stringLen+1));
            if (!string)
            {
                RSRelease(working);
                return NULL;
            }
            icuErr = U_ZERO_ERROR;
            (void) uset_getItem(set, i, &start, &end, string, stringLen+1, &icuErr);
        }
        if (U_FAILURE(icuErr))
        {
            if (string != buffer)
                free(string);
            RSRelease(working);
            return NULL;
        }
        if (stringLen <= 0)
            RSCharacterSetAddCharactersInRange(working, RSMakeRange(start, end-start+1));
        else
        {
            RSStringRef cfString = RSStringCreateWithCharactersNoCopy(RSAllocatorSystemDefault, (UniChar *)string, stringLen, nil);
            RSCharacterSetAddCharactersInString(working, cfString);
            RSRelease(cfString);
        }
        if (string != buffer)
            free(string);
    }
    
    RSCharacterSetRef   result = RSCharacterSetCreateCopy(RSAllocatorSystemDefault, working);
    RSRelease(working);
    return result;
}
#endif

static bool __RSLocaleCopyExemplarCharSet(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context)
{
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    if (RSStringGetCString(locale->_identifier, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) >= 0) {
        UErrorCode icuStatus = U_ZERO_ERROR;
        ULocaleData* uld = ulocdata_open(localeID, &icuStatus);
        USet *set = ulocdata_getExemplarSet(uld, NULL, USET_ADD_CASE_MAPPINGS, ULOCDATA_ES_STANDARD, &icuStatus);
        ulocdata_close(uld);
        if (U_FAILURE(icuStatus))
            return NO;
        if (icuStatus == U_USING_DEFAULT_WARNING)   // If default locale used, force to empty set
            uset_clear(set);
        *cf = (RSTypeRef) _RSCreateCharacterSetFromUSet(set);
        uset_close(set);
        return (*cf != NULL);
    }
#endif
    return NO;
}

static bool __RSLocaleCopyICUKeyword(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context, const char *keyword)
{
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    if (RSStringGetCString(locale->_identifier, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) >= 0)
    {
        char value[ULOC_KEYWORD_AND_VALUES_CAPACITY];
        UErrorCode icuStatus = U_ZERO_ERROR;
        if (uloc_getKeywordValue(localeID, keyword, value, sizeof(value)/sizeof(char), &icuStatus) > 0 && U_SUCCESS(icuStatus))
        {
            *cf = (RSTypeRef) RSStringCreateWithCString(RSAllocatorSystemDefault, value, RSStringEncodingASCII);
            return YES;
        }
    }
#endif
    *cf = NULL;
    return NO;
}

static bool __RSLocaleCopyICUCalendarID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context, const char *keyword) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    if (RSStringGetCString(locale->_identifier, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) >= 0) {
        UErrorCode icuStatus = U_ZERO_ERROR;
        UEnumeration *en = ucal_getKeywordValuesForLocale(keyword, localeID, TRUE, &icuStatus);
        int32_t len;
        const char *value = uenum_next(en, &len, &icuStatus);
        if (U_SUCCESS(icuStatus)) {
            *cf = (RSTypeRef) RSStringCreateWithCString(RSAllocatorSystemDefault, value, RSStringEncodingASCII);
            uenum_close(en);
            return YES;
        }
        uenum_close(en);
    }
#endif
    *cf = NULL;
    return NO;
}

static bool __RSLocaleCopyCalendarID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    bool succeeded = __RSLocaleCopyICUKeyword(locale, user, cf, context, kCalendarKeyword);
    if (!succeeded) {
        succeeded = __RSLocaleCopyICUCalendarID(locale, user, cf, context, kCalendarKeyword);
    }
    if (succeeded) {
        if (RSEqual(*cf, RSCalendarIdentifierGregorian)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierGregorian);
        } else if (RSEqual(*cf, RSCalendarIdentifierBuddhist)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierBuddhist);
        } else if (RSEqual(*cf, RSCalendarIdentifierJapanese)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierJapanese);
        } else if (RSEqual(*cf, RSCalendarIdentifierIslamic)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierIslamic);
        } else if (RSEqual(*cf, RSCalendarIdentifierIslamicCivil)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierIslamicCivil);
        } else if (RSEqual(*cf, RSCalendarIdentifierHebrew)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierHebrew);
        } else if (RSEqual(*cf, RSCalendarIdentifierChinese)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierChinese);
        } else if (RSEqual(*cf, RSCalendarIdentifierRepublicOfChina)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierRepublicOfChina);
        } else if (RSEqual(*cf, RSCalendarIdentifierPersian)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierPersian);
        } else if (RSEqual(*cf, RSCalendarIdentifierIndian)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierIndian);
        } else if (RSEqual(*cf, RSCalendarIdentifierISO8601)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierISO8601);
        } else if (RSEqual(*cf, RSCalendarIdentifierCoptic)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierCoptic);
        } else if (RSEqual(*cf, RSCalendarIdentifierEthiopicAmeteMihret)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierEthiopicAmeteMihret);
        } else if (RSEqual(*cf, RSCalendarIdentifierEthiopicAmeteAlem)) {
            RSRelease(*cf);
            *cf = RSRetain(RSCalendarIdentifierEthiopicAmeteAlem);
        } else {
            RSRelease(*cf);
            *cf = NULL;
            return NO;
        }
    } else {
        *cf = RSRetain(RSCalendarIdentifierGregorian);
    }
    return YES;
}

static bool __RSLocaleCopyCalendar(RSLocaleRef locale, bool user, RSTypeRef *rs, RSStringRef context)
{
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    if (__RSLocaleCopyCalendarID(locale, user, rs, context)) {
//        RSCalendarRef calendar = RSCalendarCreateWithIdentifier(RSAllocatorSystemDefault, (RSStringRef)*rs);
//        RSCalendarSetLocale(calendar, locale);
        RSDictionaryRef prefs = __RSLocaleGetPrefs(locale);
        RSTypeRef metapref = prefs ? RSDictionaryGetValue(prefs, RSSTR("AppleFirstWeekday")) : NULL;
        if (NULL != metapref && RSGetTypeID(metapref) == RSDictionaryGetTypeID()) {
            metapref = (RSNumberRef)RSDictionaryGetValue((RSDictionaryRef)metapref, *rs);
        }
        if (NULL != metapref && RSGetTypeID(metapref) == RSNumberGetTypeID()) {
            RSIndex wkdy;
            if (RSNumberGetValue((RSNumberRef)metapref, &wkdy)) {
//                RSCalendarSetFirstWeekday(calendar, wkdy);
            }
        }
        metapref = prefs ? RSDictionaryGetValue(prefs, RSSTR("AppleMinDaysInFirstWeek")) : NULL;
        if (NULL != metapref && RSGetTypeID(metapref) == RSDictionaryGetTypeID()) {
            metapref = (RSNumberRef)RSDictionaryGetValue((RSDictionaryRef)metapref, *rs);
        }
        if (NULL != metapref && RSGetTypeID(metapref) == RSNumberGetTypeID()) {
            RSIndex mwd;
            if (RSNumberGetValue((RSNumberRef)metapref, &mwd)) {
//                RSCalendarSetMinimumDaysInFirstWeek(calendar, mwd);
            }
        }
        RSRelease(*rs);
//        *rs = calendar;
        return YES;
    }
#endif
    return NO;
}

static bool __RSLocaleCopyDelimiter(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    ULocaleDataDelimiterType type = (ULocaleDataDelimiterType)0;
    if (context == RSLocaleQuotationBeginDelimiterKey) {
        type = ULOCDATA_QUOTATION_START;
    } else if (context == RSLocaleQuotationEndDelimiterKey) {
        type = ULOCDATA_QUOTATION_END;
    } else if (context == RSLocaleAlternateQuotationBeginDelimiterKey) {
        type = ULOCDATA_ALT_QUOTATION_START;
    } else if (context == RSLocaleAlternateQuotationEndDelimiterKey) {
        type = ULOCDATA_ALT_QUOTATION_END;
    } else {
        return NO;
    }
    
    char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    if (RSStringGetCString(locale->_identifier, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) < 0) {
        return NO;
    }
    
    UChar buffer[130];
    UErrorCode status = U_ZERO_ERROR;
    ULocaleData *uld = ulocdata_open(localeID, &status);
    int32_t len = ulocdata_getDelimiter(uld, type, buffer, sizeof(buffer) / sizeof(buffer[0]), &status);
    ulocdata_close(uld);
    if (U_FAILURE(status) || sizeof(buffer) / sizeof(buffer[0]) < len) {
        return NO;
    }
    
    *cf = RSStringCreateWithCharacters(RSAllocatorSystemDefault, (UniChar *)buffer, len);
    return (*cf != NULL);
#else
    if (context == RSLocaleQuotationBeginDelimiterKey || context == RSLocaleQuotationEndDelimiterKey || context == RSLocaleAlternateQuotationBeginDelimiterKey || context == RSLocaleAlternateQuotationEndDelimiterKey) {
        *cf = RSRetain(RSSTR("\""));
        return YES;
    } else {
        return NO;
    }
#endif
}

static bool __RSLocaleCopyCollationID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    return __RSLocaleCopyICUKeyword(locale, user, cf, context, kCollationKeyword);
}

static bool __RSLocaleCopyCollatorID(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    RSStringRef canonLocaleRSStr = NULL;
    if (user && locale->_prefs) {
        RSStringRef pref = (RSStringRef)RSDictionaryGetValue(locale->_prefs, RSSTR("AppleCollationOrder"));
        if (pref) {
            // Canonicalize pref string in case it's not in the canonical format.
            canonLocaleRSStr = RSLocaleCreateCanonicalLanguageIdentifierFromString(RSAllocatorSystemDefault, pref);
        } else {
            RSArrayRef languagesArray = (RSArrayRef)RSDictionaryGetValue(locale->_prefs, RSSTR("AppleLanguages"));
            if (languagesArray && (RSArrayGetTypeID() == RSGetTypeID(languagesArray))) {
                if (0 < RSArrayGetCount(languagesArray)) {
                    RSStringRef str = (RSStringRef)RSArrayObjectAtIndex(languagesArray, 0);
                    if (str && (RSStringGetTypeID() == RSGetTypeID(str))) {
                        canonLocaleRSStr = RSLocaleCreateCanonicalLanguageIdentifierFromString(RSAllocatorSystemDefault, str);
                    }
                }
            }
        }
    }
    if (!canonLocaleRSStr) {
        canonLocaleRSStr = RSLocaleGetIdentifier(locale);
        RSRetain(canonLocaleRSStr);
    }
    *cf = canonLocaleRSStr;
    return canonLocaleRSStr ? YES : NO;
}

static bool __RSLocaleCopyUsesMetric(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    bool us = NO;    // Default is Metric
    bool done = NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    if (user) {
        RSTypeRef pref = RSDictionaryGetValue(locale->_prefs, RSSTR("AppleMetricUnits"));
        if (pref) {
            us = (RSBooleanFalse == pref);
            done = YES;
        } else {
            pref = RSDictionaryGetValue(locale->_prefs, RSSTR("AppleMeasurementUnits"));
            if (pref) {
                us = RSEqual(pref, RSSTR("Inches"));
                done = YES;
            }
        }
    }
#endif
    if (!done) {
        char localeID[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
        if (RSStringGetCString(locale->_identifier, localeID, sizeof(localeID)/sizeof(char), RSStringEncodingASCII) >= 0) {
            UErrorCode  icuStatus = U_ZERO_ERROR;
            UMeasurementSystem ms = UMS_SI;
            ms = ulocdata_getMeasurementSystem(localeID, &icuStatus);
            if (U_SUCCESS(icuStatus)) {
                us = (ms == UMS_US);
                done = YES;
            }
        }
    }
    if (!done)
        us = NO;
    *cf = us ? RSRetain(RSBooleanFalse) : RSRetain(RSBooleanTrue);
    return YES;
#else
    *cf = RSRetain(RSBOOLFalse);
    return YES;
#endif
}

static bool __RSLocaleCopyMeasurementSystem(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    if (__RSLocaleCopyUsesMetric(locale, user, cf, context)) {
        bool us = (*cf == RSBooleanFalse);
        RSRelease(*cf);
        *cf = us ? RSRetain(RSSTR("U.S.")) : RSRetain(RSSTR("Metric"));
        return YES;
    }
    return NO;
}

static bool __RSLocaleCopyNumberFormat(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    RSStringRef str = NULL;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//    RSNumberFormatterRef nf = RSNumberFormatterCreate(RSAllocatorSystemDefault, locale, RSNumberFormatterDecimalStyle);
//    str = nf ? (RSStringRef)RSNumberFormatterCopyProperty(nf, context) : NULL;
//    if (nf) RSRelease(nf);
#endif
    if (str) {
        *cf = str;
        return YES;
    }
    return NO;
}

// ICU does not reliably set up currency info for other than Currency-type formatters,
// so we have to have another routine here which creates a Currency number formatter.
static bool __RSLocaleCopyNumberFormat2(RSLocaleRef locale, bool user, RSTypeRef *cf, RSStringRef context) {
    RSStringRef str = NULL;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//    RSNumberFormatterRef nf = RSNumberFormatterCreate(RSAllocatorSystemDefault, locale, RSNumberFormatterCurrencyStyle);
//    str = nf ? (RSStringRef)RSNumberFormatterCopyProperty(nf, context) : NULL;
//    if (nf) RSRelease(nf);
#endif
    if (str) {
        *cf = str;
        return YES;
    }
    return NO;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
typedef int32_t (*__RSICUFunction)(const char *, const char *, UChar *, int32_t, UErrorCode *);

static bool __RSLocaleICUName(const char *locale, const char *valLocale, RSStringRef *out, __RSICUFunction icu) {
    UErrorCode icuStatus = U_ZERO_ERROR;
    int32_t size;
    UChar name[kMaxICUNameSize];
    
    size = (*icu)(valLocale, locale, name, kMaxICUNameSize, &icuStatus);
    if (U_SUCCESS(icuStatus) && size > 0 && icuStatus != U_USING_DEFAULT_WARNING) {
        *out = RSStringCreateWithCharacters(RSAllocatorSystemDefault, (UniChar *)name, size);
        return (*out != NULL);
    }
    return NO;
}

static bool __RSLocaleICUKeywordValueName(const char *locale, const char *value, const char *keyword, RSStringRef *out) {
    UErrorCode icuStatus = U_ZERO_ERROR;
    int32_t size = 0;
    UChar name[kMaxICUNameSize];
    // Need to make a fake locale ID
    char lid[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    if (strlen(value) < ULOC_KEYWORD_AND_VALUES_CAPACITY) {
        strlcpy(lid, "en_US@", sizeof(lid));
        strlcat(lid, keyword, sizeof(lid));
        strlcat(lid, "=", sizeof(lid));
        strlcat(lid, value, sizeof(lid));
        size = uloc_getDisplayKeywordValue(lid, keyword, locale, name, kMaxICUNameSize, &icuStatus);
        if (U_SUCCESS(icuStatus) && size > 0 && icuStatus != U_USING_DEFAULT_WARNING) {
            *out = RSStringCreateWithCharacters(RSAllocatorSystemDefault, (UniChar *)name, size);
            return (*out != NULL);
        }
    }
    return NO;
}

static bool __RSLocaleICUCurrencyName(const char *locale, const char *value, UCurrNameStyle style, RSStringRef *out) {
    int valLen = (int)strlen(value);
    if (valLen != 3) // not a valid ISO code
        return NO;
    UChar curr[4];
    UBool isChoice = FALSE;
    int32_t size = 0;
    UErrorCode icuStatus = U_ZERO_ERROR;
    u_charsToUChars(value, curr, valLen);
    curr[valLen] = '\0';
    const UChar *name;
    name = ucurr_getName(curr, locale, style, &isChoice, &size, &icuStatus);
    if (U_FAILURE(icuStatus) || icuStatus == U_USING_DEFAULT_WARNING)
        return NO;
    UChar result[kMaxICUNameSize];
    if (isChoice)
    {
        UChar pattern[kMaxICUNameSize];
        RSStringRef patternRef = RSStringCreateWithFormat(RSAllocatorSystemDefault, NULL, RSSTR("{0,choice,%S}"), name);
        RSIndex pattlen = RSStringGetLength(patternRef);
        RSStringGetCharacters(patternRef, RSMakeRange(0, pattlen), (UniChar *)pattern);
        RSRelease(patternRef);
        pattern[pattlen] = '\0';        // null terminate the pattern
        // Format the message assuming a large amount of the currency
        size = u_formatMessage("en_US", pattern, (int)pattlen, result, kMaxICUNameSize, &icuStatus, 10.0);
        if (U_FAILURE(icuStatus))
            return NO;
        name = result;
        
    }
    *out = RSStringCreateWithCharacters(RSAllocatorSystemDefault, (UniChar *)name, size);
    return (*out != NULL);
}
#endif

static bool __RSLocaleFullName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    UErrorCode icuStatus = U_ZERO_ERROR;
    int32_t size;
    UChar name[kMaxICUNameSize];
    
    // First, try to get the full locale.
    size = uloc_getDisplayName(value, locale, name, kMaxICUNameSize, &icuStatus);
    if (U_FAILURE(icuStatus) || size <= 0)
        return NO;
    
    // Did we wind up using a default somewhere?
    if (icuStatus == U_USING_DEFAULT_WARNING) {
        // For some locale IDs, there may be no language which has a translation for every
        // piece. Rather than return nothing, see if we can at least handle
        // the language part of the locale.
        UErrorCode localStatus = U_ZERO_ERROR;
        int32_t localSize;
        UChar localName[kMaxICUNameSize];
        localSize = uloc_getDisplayLanguage(value, locale, localName, kMaxICUNameSize, &localStatus);
        if (U_FAILURE(localStatus) || size <= 0 || localStatus == U_USING_DEFAULT_WARNING)
            return NO;
    }
    
    // This locale is OK, so use the result.
    *out = RSStringCreateWithCharacters(RSAllocatorSystemDefault, (UniChar *)name, size);
    return (*out != NULL);
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleLanguageName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    return __RSLocaleICUName(locale, value, out, uloc_getDisplayLanguage);
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleCountryName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    // Need to make a fake locale ID
    char lid[ULOC_FULLNAME_CAPACITY];
    if (strlen(value) < sizeof(lid) - 3) {
        strlcpy(lid, "en_", sizeof(lid));
        strlcat(lid, value, sizeof(lid));
        return __RSLocaleICUName(locale, lid, out, uloc_getDisplayCountry);
    }
    return NO;
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleScriptName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    // Need to make a fake locale ID
    char lid[ULOC_FULLNAME_CAPACITY];
    if (strlen(value) == 4) {
        strlcpy(lid, "en_", sizeof(lid));
        strlcat(lid, value, sizeof(lid));
        strlcat(lid, "_US", sizeof(lid));
        return __RSLocaleICUName(locale, lid, out, uloc_getDisplayScript);
    }
    return NO;
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleVariantName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    // Need to make a fake locale ID
    char lid[ULOC_FULLNAME_CAPACITY+ULOC_KEYWORD_AND_VALUES_CAPACITY];
    if (strlen(value) < sizeof(lid) - 6) {
        strlcpy(lid, "en_US_", sizeof(lid));
        strlcat(lid, value, sizeof(lid));
        return __RSLocaleICUName(locale, lid, out, uloc_getDisplayVariant);
    }
    return NO;
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleCalendarName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    return __RSLocaleICUKeywordValueName(locale, value, kCalendarKeyword, out);
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleCollationName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    return __RSLocaleICUKeywordValueName(locale, value, kCollationKeyword, out);
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleCurrencyShortName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    return __RSLocaleICUCurrencyName(locale, value, UCURR_SYMBOL_NAME, out);
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleCurrencyFullName(const char *locale, const char *value, RSStringRef *out) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    return __RSLocaleICUCurrencyName(locale, value, UCURR_LONG_NAME, out);
#else
    *out = RSRetain(RSSTR("(none)"));
    return YES;
#endif
}

static bool __RSLocaleNoName(const char *locale, const char *value, RSStringRef *out) {
    return NO;
}

#undef kMaxICUNameSize

