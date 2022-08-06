//
// Copyright (c) 2009 Brandon Jones
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

TEST_F(SqratTest, ConstBindings) {
    DefaultVM::Set(vm);

    ConstTable(vm).Enum(_SC("Color"), Enumeration(vm)
                        .Const(_SC("Black"), 0)
                        .Const(_SC("Red"), 1)
                        .Const(_SC("Green"), 2)
                        .Const(_SC("Blue"), 3)
                       );

    ConstTable().Const(_SC("Version"), _SC("1.0.0"));

    Script script;

    try {
        script.CompileString(_SC(" \
			gTest.EXPECT_INT_EQ(Color.Black, 0); \
			gTest.EXPECT_INT_EQ(Color.Red, 1); \
			gTest.EXPECT_INT_EQ(Color.Green, 2); \
			gTest.EXPECT_INT_EQ(Color.Blue, 3); \
			gTest.EXPECT_STR_EQ(Version, \"1.0.0\"); \
			"));
    } catch(Exception ex) {
        FAIL() << _SC("Compile Failed: ") << ex.Message();
    }

    try {
        script.Run();
    } catch(Exception ex) {
        FAIL() << _SC("Run Failed: ") << ex.Message();
    }
}
