//
// Copyright (c) 2013 Li-Cheng (Andy) Tai
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#include <gtest/gtest.h>
#include <sqrat.h>
#include "Fixture.h"


using namespace Sqrat;

TEST_F(SqratTest, ArrayGet) {

    static const SQChar *sq_code = _SC("\
        local i; \
        for (i = 0; i < 12; i++) \
            a.append(i);\
        \
           ");
    int i;
    DefaultVM::Set(vm);
    
    Array array(vm);
    RootTable(vm).Bind(_SC("a"), array);
        
    Script script;
    try {
        script.CompileString(sq_code);
    } catch(Exception ex) {
        FAIL() << _SC("Compile Failed: ") << ex.Message();
    }

    try {
        script.Run();
    } catch(Exception ex) {
        FAIL() << _SC("Run Failed: ") << ex.Message();
    }
    
    const int length = array.Length();
    EXPECT_EQ(length, 12);
    
    for ( i = 0; i < length; i++)
    {
        int t;
        int j = array.GetElement(i, t);
        EXPECT_EQ(j, 1);
        EXPECT_EQ(t, i);
        float f;
        j = array.GetElement(i, f);
        EXPECT_EQ(j, 1);
        EXPECT_EQ((float) i, f);
    }
    int t[length];
    int j = array.GetArray(t, sizeof(t) / sizeof(t[0]));
    EXPECT_EQ(j, 1);
    for (i = 0; i < length; i++)
    {
        EXPECT_EQ(t[i], i);
    }
    double d[length];
    j = array.GetArray(d, sizeof(d) / sizeof(d[0]));
    EXPECT_EQ(j, 1);
    for (i = 0; i < length; i++)
    {
        EXPECT_EQ(d[i], (double) i);
    }    
    double d2[15];
    j = array.GetArray(d2, sizeof(d2) / sizeof(d2[0]));
    EXPECT_NE(j, 1);
    double d3[5];
    j = array.GetArray(d3, sizeof(d3) / sizeof(d3[0]));
    EXPECT_NE(j, 1);
        
}
