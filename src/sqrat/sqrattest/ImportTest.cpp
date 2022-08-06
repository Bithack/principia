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
#include <sqratimport.h>
#include "Fixture.h"

using namespace Sqrat;

TEST_F(SqratTest, ImportScript) {
    DefaultVM::Set(vm);

    sqrat_register_importlib(vm);

    Script script;

    try {
        script.CompileString(_SC(" \
			::import(\"scripts/samplemodule\"); \
			\
			gTest.EXPECT_FLOAT_EQ(3.1415, ::PI); \
			gTest.EXPECT_INT_EQ(10, ::RectArea(2, 5)); \
			gTest.EXPECT_FLOAT_EQ(12.566, ::CircleArea(2)); \
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

TEST_F(SqratTest, ImportScriptIntoTable) {
    DefaultVM::Set(vm);

    sqrat_register_importlib(vm);

    Script script;

    try {
        script.CompileString(_SC(" \
			mod <- ::import(\"scripts/samplemodule\", {}); \
			\
			gTest.EXPECT_FLOAT_EQ(3.1415, mod.PI); \
			gTest.EXPECT_INT_EQ(10, mod.RectArea(2, 5)); \
			gTest.EXPECT_FLOAT_EQ(12.566, mod.CircleArea(2)); \
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