//
//  BasicHash.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/16/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#include "BasicHash.hpp"

namespace RSFoundation {
    namespace Basic {
        
        const BasicHashTable::TableElementType BasicHashTable::TableSizes[BasicHashTable::TableSize] = {
            0, 3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
            2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
            214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
            6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
            111638519, 180634607, 292272623, 472907251,
#if __LP64__
            765180413UL, 1238087663UL, 2003267557UL, 3241355263UL, 5244622819UL,
#if 0
            8485977589UL, 13730600407UL, 22216578047UL, 35947178479UL,
            58163756537UL, 94110934997UL, 152274691561UL, 246385626107UL,
            398660317687UL, 645045943807UL, 1043706260983UL, 1688752204787UL,
            2732458465769UL, 4421210670577UL, 7153669136377UL,
            11574879807461UL, 18728548943849UL, 30303428750843UL
#endif
#endif
        };
        
        const BasicHashTable::TableElementType BasicHashTable::TableCapacities[BasicHashTable::TableSize] = {
            0, 3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
            1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
            132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
            3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
            68996399, 111638327, 180634415, 292272755,
#if __LP64__
            472907503UL, 765180257UL, 1238087439UL, 2003267722UL, 3241355160UL,
#if 0
            5244622578UL, 8485977737UL, 13730600347UL, 22216578100UL,
            35947178453UL, 58163756541UL, 94110935011UL, 152274691274UL,
            246385626296UL, 398660317578UL, 645045943559UL, 1043706261135UL,
            1688752204693UL, 2732458465840UL, 4421210670552UL,
            7153669136706UL, 11574879807265UL, 18728548943682UL
#endif
#endif
        };
        
        const BasicHashTable::TableElementType BasicHashTable::PrimitiveRoots[BasicHashTable::TableSize] = {
            0, 2, 3, 2, 5, 6, 7, 3, 19, 6, 5, 3, 3, 3,
            2, 5, 6, 3, 3, 6, 2, 3, 3,
            3, 5, 10, 3, 3, 22, 3,
            3, 3, 5, 2, 22, 2,
            11, 5, 5, 2,
#if __LP64__
            3, 10, 2, 3, 10,
            2, 3, 5, 3,
            3, 2, 7, 2,
            3, 3, 3, 2,
            3, 5, 5,
            2, 3, 2
#endif
        };
    }
    
    class TestBasicHash {
    public:
        TestBasicHash() {
            OptionFlags flags = BasicHash::LinearHashing;
            flags |= BasicHash::HasKeys;
            basicHash = BasicHash::Create(flags);
            basicHash->SuppressRC();
        }
        
        ~TestBasicHash() {
            Allocator<BasicHash>::SystemDefault.Deallocate(basicHash);
            basicHash = nullptr;
        }
        BasicHash *basicHash;
    };
    
    __RS_FOUNDATION_INIT_ROUTINE(1000) void TestBasicHashFunc() {
        TestBasicHash tbh;
        BasicHash *ht = tbh.basicHash;
        for (int i = 1; i < 10; ++i) {
            ht->AddValue(i, i);
        }
        
        for (int i = 1; i < 10; ++i) {
            const BasicHash::Bucket &bucket = ht->FindBucket(i);
            if (bucket.count) {
                assert(bucket.weak_value == i);
            }
        }
    }

}