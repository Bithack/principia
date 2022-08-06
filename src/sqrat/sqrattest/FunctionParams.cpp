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

int f0()
{
    return 1;
}

int f1(int a1)
{
    return (a1 == 1);
}


int f2(int a1, int a2)
{
    return (a1 == 1) && (a2 == 2);
}


int f3(int a1, int a2, int a3)
{
    return (a1 == 1) && (a2 == 2) && (a3 == 3);
}


int f4(int a1, int a2, int a3, int a4)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4);
}


int f5(int a1, int a2, int a3, int a4, int a5)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5);
}


int f6(int a1, int a2, int a3, int a4, int a5, int a6)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6);
}

int f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7);
}

int f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8);
}

int f9(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
       int a9)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8) && (a9 == 9);
}

int f10(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
        int a9, int a10)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8) && (a9 == 9) && (a10 == 10);
}

int f11(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
        int a9, int a10, int a11)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8)&& (a9 == 9) && (a10 == 10)
           && (a11 == 11);
}

int f12(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
        int a9, int a10, int a11, int a12)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8)&& (a9 == 9) && (a10 == 10)
           && (a11 == 11) && (a12 == 12);
}

int f13(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
        int a9, int a10, int a11, int a12, int a13)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8) && (a9 == 9) && (a10 == 10)
           && (a11 == 11) && (a12 == 12) && (a13 == 13);
}

int f14(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
        int a9, int a10, int a11, int a12, int a13, int a14)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8)&& (a9 == 9) && (a10 == 10)
           && (a11 == 11) && (a12 == 12) && (a13 == 13) && (a14 == 14);
}

TEST_F(SqratTest, GlobalFunction) {
    DefaultVM::Set(vm);

    RootTable().Func(_SC("f0"), &f0);
    RootTable().Func(_SC("f1"), &f1);
    RootTable().Func(_SC("f2"), &f2);
    RootTable().Func(_SC("f3"), &f3);
    RootTable().Func(_SC("f4"), &f4);
    RootTable().Func(_SC("f5"), &f5);
    RootTable().Func(_SC("f6"), &f6);
    RootTable().Func(_SC("f7"), &f7);
    RootTable().Func(_SC("f8"), &f8);
    RootTable().Func(_SC("f9"), &f9);
    RootTable().Func(_SC("f10"), &f10);
    RootTable().Func(_SC("f11"), &f11);
    RootTable().Func(_SC("f12"), &f12);
    RootTable().Func(_SC("f13"), &f13);
    RootTable().Func(_SC("f14"), &f14);

    Script script;

    try {
        script.CompileString(_SC(" \
			gTest.EXPECT_INT_EQ(1, f0()); print(0);\
			gTest.EXPECT_INT_EQ(1, f1(1)); print(1); \
			gTest.EXPECT_INT_EQ(1, f2(1, 2)); print(2);\
			gTest.EXPECT_INT_EQ(1, f3(1, 2, 3)); print(3);\
			gTest.EXPECT_INT_EQ(1, f4(1, 2, 3, 4)); print(4);\
			gTest.EXPECT_INT_EQ(1, f5(1, 2, 3, 4, 5)); print(5);\
			gTest.EXPECT_INT_EQ(1, f6(1, 2, 3, 4, 5, 6)); print(6);\
			gTest.EXPECT_INT_EQ(1, f7(1, 2, 3, 4, 5, 6, 7)); print(7);\
			gTest.EXPECT_INT_EQ(1, f8(1, 2, 3, 4, 5, 6, 7, 8)); print(8);\
			gTest.EXPECT_INT_EQ(1, f9(1, 2, 3, 4, 5, 6, 7, 8, 9)); print(9);\
			gTest.EXPECT_INT_EQ(1, f10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)); print(10);\
			gTest.EXPECT_INT_EQ(1, f11(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)); print(11);\
			gTest.EXPECT_INT_EQ(1, f12(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)); print(12);\
			gTest.EXPECT_INT_EQ(1, f13(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)); print(13);\
			gTest.EXPECT_INT_EQ(1, f14(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14)); print(14);\
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

class C
{
public:
    int f0();
    int f1(int a1);
    int f2(int a1, int a2);
    int f3(int a1, int a2, int a3);
    int f4(int a1, int a2, int a3, int a4);
    int f5(int a1, int a2, int a3, int a4, int a5);
    int f6(int a1, int a2, int a3, int a4, int a5, int a6);
    int f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
    int f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
    int f9(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
           int a9);
    int f10(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
            int a9, int a10);
    int f11(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
            int a9, int a10, int a11);
    int f12(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
            int a9, int a10, int a11, int a12);
    int f13(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
            int a9, int a10, int a11, int a12, int a13);
    int f14(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
            int a9, int a10, int a11, int a12, int a13, int a14);
};

int C::f0()
{
    return 1;
}

int C::f1(int a1)
{
    return (a1 == 1);
}


int C::f2(int a1, int a2)
{
    return (a1 == 1) && (a2 == 2);
}


int C::f3(int a1, int a2, int a3)
{
    return (a1 == 1) && (a2 == 2) && (a3 == 3);
}


int C::f4(int a1, int a2, int a3, int a4)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4);
}


int C::f5(int a1, int a2, int a3, int a4, int a5)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5);
}


int C::f6(int a1, int a2, int a3, int a4, int a5, int a6)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6);
}

int C::f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7);
}

int C::f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8);
}

int C::f9(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
          int a9)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8) && (a9 == 9);
}

int C::f10(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
           int a9, int a10)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8) && (a9 == 9) && (a10 == 10);
}

int C::f11(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
           int a9, int a10, int a11)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8)&& (a9 == 9) && (a10 == 10)
           && (a11 == 11);
}

int C::f12(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
           int a9, int a10, int a11, int a12)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8)&& (a9 == 9) && (a10 == 10)
           && (a11 == 11) && (a12 == 12);
}

int C::f13(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
           int a9, int a10, int a11, int a12, int a13)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8) && (a9 == 9) && (a10 == 10)
           && (a11 == 11) && (a12 == 12) && (a13 == 13);
}

int C::f14(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
           int a9, int a10, int a11, int a12, int a13, int a14)
{
    return (a1 == 1)&& (a2 == 2) && (a3 == 3) && (a4 == 4) && (a5 == 5)
           && (a6 == 6) && (a7 == 7) && (a8 == 8)&& (a9 == 9) && (a10 == 10)
           && (a11 == 11) && (a12 == 12) && (a13 == 13) && (a14 == 14);
}

TEST_F(SqratTest, MemberFunction) {
    DefaultVM::Set(vm);

    Class<C> CC;
    CC.Func(_SC("f0"), &C::f0);
    CC.Func(_SC("f1"), &C::f1);
    CC.Func(_SC("f2"), &C::f2);
    CC.Func(_SC("f3"), &C::f3);
    CC.Func(_SC("f4"), &C::f4);
    CC.Func(_SC("f5"), &C::f5);
    CC.Func(_SC("f6"), &C::f6);
    CC.Func(_SC("f7"), &C::f7);
    CC.Func(_SC("f8"), &C::f8);
    CC.Func(_SC("f9"), &C::f9);
    CC.Func(_SC("f10"), &C::f10);
    CC.Func(_SC("f11"), &C::f11);
    CC.Func(_SC("f12"), &C::f12);
    CC.Func(_SC("f13"), &C::f13);
    CC.Func(_SC("f14"), &C::f14);

    RootTable().Bind(_SC("C"), CC);

    Script script;

    try {
        script.CompileString(_SC(" \
		    c <- C(); \
			gTest.EXPECT_INT_EQ(1, c.f0()); print(0);\
			gTest.EXPECT_INT_EQ(1, c.f1(1)); print(1); \
			gTest.EXPECT_INT_EQ(1, c.f2(1, 2)); print(2);\
			gTest.EXPECT_INT_EQ(1, c.f3(1, 2, 3)); print(3);\
			gTest.EXPECT_INT_EQ(1, c.f4(1, 2, 3, 4)); print(4);\
			gTest.EXPECT_INT_EQ(1, c.f5(1, 2, 3, 4, 5)); print(5);\
			gTest.EXPECT_INT_EQ(1, c.f6(1, 2, 3, 4, 5, 6)); print(6);\
			gTest.EXPECT_INT_EQ(1, c.f7(1, 2, 3, 4, 5, 6, 7)); print(7);\
			gTest.EXPECT_INT_EQ(1, c.f8(1, 2, 3, 4, 5, 6, 7, 8)); print(8);\
			gTest.EXPECT_INT_EQ(1, c.f9(1, 2, 3, 4, 5, 6, 7, 8, 9)); print(9);\
			gTest.EXPECT_INT_EQ(1, c.f10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)); print(10);\
			gTest.EXPECT_INT_EQ(1, c.f11(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)); print(11);\
			gTest.EXPECT_INT_EQ(1, c.f12(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)); print(12);\
			gTest.EXPECT_INT_EQ(1, c.f13(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)); print(13);\
			gTest.EXPECT_INT_EQ(1, c.f14(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14)); print(14);\
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

