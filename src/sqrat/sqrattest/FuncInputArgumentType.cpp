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

class Vector2
{
public:
    float value;
    
    //some Vector Math overloads and members here
    operator float() const 
    {
        return 1.0f; /* test only */    
    }
    
    
    Vector2 operator +(const Vector2& v) 
    {
        value += v.value;
        return v; /* test only */    
    }
    
    inline float add(const Vector2& v) 
    {
        return (*this) + v; //it crashes right here as it references to an nonexistent obj
    }

    
    bool boolFunc() 
    {
        return true;
    }
    
    bool boolFunc2() 
    {
        return false;
    }
    
};

static const SQChar *sq_code = _SC("\
    local v = Vector2();\
    local v2 = Vector2();\
    \
    local b = v2.boolFunc(); \
    \
    \
    local b = v2.boolFunc(); \
    \
	gTest.EXPECT_TRUE(b); \
	gTest.EXPECT_INT_EQ(b, 1); \
	gTest.EXPECT_FLOAT_EQ(b, 1.0); \
    b = v2.boolFunc2(); \
    \
	gTest.EXPECT_FALSE(b); \
	gTest.EXPECT_INT_EQ(b, 0); \
	gTest.EXPECT_FLOAT_EQ(b, 0.0); \
    \
    print (b) ; \
    b = v2.boolFunc2(); \
    \
    print (b) ; \
    \
    v.add(v2); /*good*/  \
    print(\"1\\n\");\
    local raised = false;\
    try { \
        v.add(10); /*was crashing*/  \
		gTest.EXPECT_INT_EQ(0, 1); \
	} catch (ex) {\
        raised = true;\
        print(ex + \"\\n\"); \
    }\
    gTest.EXPECT_TRUE(raised); \
    print(\"2\\n\");\
    raised = false;\
    try { \
        v.add(); /*was crashing*/  \
		gTest.EXPECT_INT_EQ(0, 1); \
	} catch (ex) {\
        raised = true;\
        print(ex + \"\\n\"); \
    }\
    gTest.EXPECT_TRUE(raised); \
    print(\"3\\n\");\
    raised = false;\
    try {\
        v.add(\"text\"); /*was crashing*/  \
		gTest.EXPECT_INT_EQ(0, 1); \
	} catch (ex) {\
        raised = true;\
        print(ex + \"\\n\"); \
    }\
    gTest.EXPECT_TRUE(raised); \
    print(\"4\\n\");\
       ");



TEST_F(SqratTest, NumericArgumentTypeConversionAndCheck) {
    DefaultVM::Set(vm);
    
    Sqrat::Class<Vector2> classVector2(vm);
    
    classVector2.Func(_SC("add"), &Vector2::add);
    
    classVector2.Func(_SC("boolFunc"), &Vector2::boolFunc);
    classVector2.Func(_SC("boolFunc2"), &Vector2::boolFunc2);
    Sqrat::RootTable(vm).Bind(_SC("Vector2"), classVector2);
            
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

}
