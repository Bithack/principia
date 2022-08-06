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

TEST_F(SqratTest, CallSquirrelFunction) {
    DefaultVM::Set(vm);

    Script script;

    try {
        script.CompileString(_SC(" \
			function AddTwo(a, b) { \
				return a + b; \
			} \
			function MultiplyTwo(a, b) { \
				return a * b; \
			} \
			function returnTrue() { \
			    return true; \
			}\
			function returnFalse() { \
			    return false; \
			}\
			"));
    } catch(Exception ex) {
        FAIL() << _SC("Script Compile Failed: ") << ex.Message();
    }

    try {
        script.Run(); // Must run the script before the function will be available
    } catch(Exception ex) {
        FAIL() << _SC("Script Run Failed: ") << ex.Message();
    }

    // Method one for function retrieval: via the constructor
    Function addTwo(RootTable(), _SC("AddTwo"));
    ASSERT_FALSE(addTwo.IsNull());
    EXPECT_EQ(addTwo.Evaluate<int>(1, 2), 3);

    // Method two for function retrieval: from the class or table
    Function multiplyTwo = RootTable().GetFunction(_SC("MultiplyTwo"));
    ASSERT_FALSE(multiplyTwo.IsNull());
    EXPECT_EQ(multiplyTwo.Evaluate<int>(2, 3), 6);
    
    Function returnTrue = RootTable().GetFunction(_SC("returnTrue"));
    ASSERT_FALSE(returnTrue.IsNull());
    ASSERT_TRUE(returnTrue.Evaluate<bool>());
    ASSERT_TRUE(returnTrue.Evaluate<SQBool>()); 
 
    Function returnFalse = RootTable().GetFunction(_SC("returnFalse"));
    ASSERT_FALSE(returnFalse.IsNull());
    ASSERT_FALSE(returnFalse.Evaluate<bool>());
    ASSERT_FALSE(returnFalse.Evaluate<SQBool>());  
 
}

int NativeOp(int a, int b, Function opFunc) {
    if(opFunc.IsNull()) {
        return -1;
    }
    return opFunc.Evaluate<int>(a, b);
}

TEST_F(SqratTest, FunctionAsArgument) {
    DefaultVM::Set(vm);

    RootTable().Func(_SC("NativeOp"), &NativeOp);

    Script script;

    try {
        script.CompileString(_SC(" \
			function SubTwo(a, b) { \
				return a - b; \
			} \
			gTest.EXPECT_INT_EQ(::NativeOp(5, 1, SubTwo), 4); \
			"));
    } catch(Exception ex) {
        FAIL() << _SC("Script Compile Failed: ") << ex.Message();
    }

    try {
        script.Run();
    } catch(Exception ex) {
        FAIL() << _SC("Script Run Failed: ") << ex.Message();
    }
}