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

class Speaker {
public:
    int Echo() {
        return 0;
    }
    int Echo(int val) {
        return val;
    }
};

int GlobalEcho() {
    return 0;
}
int GlobalEcho(int val) {
    return val;
}


class StaticTestClass
{
public:
    static int i1, i2;
    
    static void set(int a1) { i1 = a1; }
    static void set(int a1, int a2) { i1 = a1; i2 = a2; }
    static int get_i1() { return i1; }
    static int get_i2() { return i2; }
};

int StaticTestClass::i1 = -1;
int StaticTestClass::i2 = -1;

TEST_F(SqratTest, OverloadedMemberFunction) {
    DefaultVM::Set(vm);

    // Member function overloads
    RootTable().Bind(_SC("Speaker"),
                     Class<Speaker>()
                     .Overload<int (Speaker::*)()>(_SC("Echo"), &Speaker::Echo)
                     .Overload<int (Speaker::*)(int)>(_SC("Echo"), &Speaker::Echo)
                    );
    // static Member function overloads
    RootTable().Bind(_SC("StaticTestClass"),
                     Class<StaticTestClass>()
                     .StaticOverload<void (*)(int)>(_SC("set"), &StaticTestClass::set)
                     .StaticOverload<void (*)(int, int)>(_SC("set"), &StaticTestClass::set)
                     .StaticFunc(_SC("get_i1"), &StaticTestClass::get_i1)
                     .StaticFunc(_SC("get_i2"), &StaticTestClass::get_i2)
                     );

    // Global Function overloads
    RootTable().Overload<int (*)()>(_SC("GlobalEcho"), &GlobalEcho);
    RootTable().Overload<int (*)(int)>(_SC("GlobalEcho"), &GlobalEcho);

    Script script;

    try {
        script.CompileString(_SC(" \
			s <- Speaker(); \
			\
			gTest.EXPECT_INT_EQ(0, s.Echo()); \
			gTest.EXPECT_INT_EQ(1, s.Echo(1)); \
			gTest.EXPECT_INT_EQ(0, GlobalEcho()); \
			gTest.EXPECT_INT_EQ(1, GlobalEcho(1)); \
			"));
    } catch(Exception ex) {
        FAIL() << _SC("Compile Failed: ") << ex.Message();
    }

    try {
        script.Run();
    } catch(Exception ex) {
        FAIL() << _SC("Run Failed: ") << ex.Message();
    }

    try {
        script.CompileString(_SC(" \
			s <- StaticTestClass(); \
			\
			gTest.EXPECT_INT_EQ(-1, s.get_i1()); \
			gTest.EXPECT_INT_EQ(-1, s.get_i2()); \
		    s.set(2); \
			gTest.EXPECT_INT_EQ(2, s.get_i1()); \
			gTest.EXPECT_INT_EQ(-1, s.get_i2()); \
		    s.set(4, 6); \
			gTest.EXPECT_INT_EQ(4, s.get_i1()); \
			gTest.EXPECT_INT_EQ(6, s.get_i2()); \
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

//
// Overload test with const functions, based on scenario provided by emeyex
//

class Entity {
public:
    unsigned int QueryEnumValue( unsigned int enumKey, unsigned int enumValueDefault ) const {
        return enumKey;
    }
    unsigned int QueryEnumValue( unsigned int enumKey ) const {
        return QueryEnumValue( enumKey, 0 );
    }
};

TEST_F(SqratTest, ConstOverloadTest) {
    DefaultVM::Set(vm);

    // Member function overloads
    RootTable().Bind(_SC("Entity"),
                     Class<Entity>()
                     .Overload<unsigned int (Entity::*)(unsigned int, unsigned int) const>(_SC("QueryEnumValue"), &Entity::QueryEnumValue)
                     .Overload<unsigned int (Entity::*)(unsigned int) const>(_SC("QueryEnumValue"), &Entity::QueryEnumValue)
                    );

    Script script;

    try {
        script.CompileString(_SC(" \
			e <- Entity(); \
			\
			gTest.EXPECT_INT_EQ(1, e.QueryEnumValue(1, 0)); \
			gTest.EXPECT_INT_EQ(2, e.QueryEnumValue(2)); \
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



class B 
{
private:
    int value;
public:
    B(): value(-1) {}
    
    int set(int v)
    {
        value = v;
    }
    int get()
    {
        //std::cout << "B's address is " << (long) this << std::endl;
        return value;
    }
    
    static string shared;
    static int sharedInt;
};

    
static B& getB(B &b)
{
    return b;
}

static B& getB2(B *b, int, char *)
{
    return *b;
}

static B& getB4( B & b, B *, const B, int)
{
    return b;
}

static B* getBPtr(B *b)
{
    return b;
}

string B::shared ;
int B::sharedInt = -1;

TEST_F(SqratTest, FunctionReturningReferencesToClassesWithStaticMembers) {
    DefaultVM::Set(vm);

    Class<B> _B;
    _B
    .Func(_SC("set"), &B::set)
    .Func(_SC("get"), &B::get)
    .StaticVar(_SC("shared"), &B::shared)
    .StaticVar(_SC("sharedInt"), &B::sharedInt);
    
    RootTable().Bind(_SC("B"), _B);
    RootTable().Func(_SC("getB"), &getB);
    RootTable().Func(_SC("getB2"), &getB2);
    RootTable().Func(_SC("getB4"), &getB4);
    RootTable().Func(_SC("getBPtr"), &getBPtr);
    
    Script script;
    try {
        script.CompileString(_SC(" \
            b <- B();\
            bb <- B(); \
            \
			gTest.EXPECT_INT_EQ(b.get(), -1); \
			gTest.EXPECT_INT_EQ(bb.sharedInt, -1); \
			gTest.EXPECT_INT_EQ(b.sharedInt, -1); \
            b.set(12);\
            b.shared = \"a long string\"; \
            b.sharedInt = 1234; \
            gTest.EXPECT_STR_EQ(bb.shared, \"a long string\"); \
            gTest.EXPECT_STR_EQ(b.shared, \"a long string\"); \
			gTest.EXPECT_INT_EQ(bb.sharedInt, 1234); \
			gTest.EXPECT_INT_EQ(b.get(), 12); \
			local b1 = getBPtr(b);\
            b.set(20);\
			gTest.EXPECT_INT_EQ(b1.get(), 20); \
			local b2 = getB(b);\
            b.set(40);\
			gTest.EXPECT_INT_EQ(b1.get(), 40); \
			gTest.EXPECT_INT_EQ(b2.get(), 40); \
			local b3 = getB2(b, 12, \"test\");\
            b.set(60);\
			gTest.EXPECT_INT_EQ(b2.get(), 60); \
			gTest.EXPECT_INT_EQ(b3.get(), 60); \
			local b4 = getB4(b, b, b, 33);\
            b.set(80);\
			gTest.EXPECT_INT_EQ(b3.get(), 80); \
			gTest.EXPECT_INT_EQ(b4.get(), 80); \
            \
            bb.shared = \"short str\"; \
            gTest.EXPECT_STR_EQ(b2.shared, \"short str\"); \
            gTest.EXPECT_STR_EQ(b3.shared, \"short str\"); \
            gTest.EXPECT_STR_EQ(b.shared, \"short str\"); \
            gTest.EXPECT_STR_EQ(b1.shared, \"short str\"); \
            gTest.EXPECT_STR_EQ(b4.shared, \"short str\"); \
			gTest.EXPECT_INT_EQ(b.sharedInt, 1234); \
			gTest.EXPECT_INT_EQ(b1.sharedInt, 1234); \
			gTest.EXPECT_INT_EQ(b2.sharedInt, 1234); \
			gTest.EXPECT_INT_EQ(b3.sharedInt, 1234); \
			gTest.EXPECT_INT_EQ(b4.sharedInt, 1234); \
            b4.shared = \"abcde\"; \
            b4.sharedInt = 9999; \
            gTest.EXPECT_STR_EQ(bb.shared, \"abcde\"); \
			gTest.EXPECT_INT_EQ(bb.sharedInt, 9999); \
			gTest.EXPECT_INT_EQ(b1.sharedInt, 9999); \
            "));
    }
    catch (Sqrat::Exception ex) {
        FAIL() << _SC("Compile Failed: ") << ex.Message();
    }

    try {
        script.Run();
    }
    catch (Exception ex) {
        FAIL() << _SC("Run Failed: ") << ex.Message();
    }
    
}

