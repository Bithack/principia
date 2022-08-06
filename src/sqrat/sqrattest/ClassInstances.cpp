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

#include <iostream>
#include <gtest/gtest.h>
#include <sqrat.h>
#include "Fixture.h"

using namespace Sqrat;

class Employee {
public:
    Employee() : supervisor(NULL) {}

    Employee(const Employee& e) :
        firstName(e.firstName), lastName(e.lastName),
        age(e.age), department(e.department),
        wage(e.wage), supervisor(e.supervisor) {

    }

    void GiveRaise(float percent) {
        wage += (wage * percent);
    }

    const string ToString() const {
        std::basic_stringstream<SQChar> out;
        out << _SC("Employee: ") << lastName << _SC(", ") << firstName << _SC("\n");
        out << _SC("Age: ") << age << _SC("\n");
        out << _SC("Department: ") << department << _SC("\n");
        out << _SC("Hourly Wage: ") << wage << _SC("\n");
        if(supervisor != NULL) {
            out << _SC("Supervisor: ") << supervisor->lastName << _SC(", ") << supervisor->firstName << _SC("\n");
        }
        return out.str();
    }

    string firstName;
    string lastName;
    int age;
    string department;
    float wage;
    const SQChar * middleName;
    const SQChar * gender;
    Employee* supervisor;
    static string sharedData;
};

string Employee::sharedData;

TEST_F(SqratTest, ClassInstances) {
    DefaultVM::Set(vm);

    Class<Employee> employee;
    employee
    .Var(_SC("firstName"), &Employee::firstName)
    .Var(_SC("lastName"), &Employee::lastName)
    .Var(_SC("age"), &Employee::age)
    .Var(_SC("department"), &Employee::department)
    .Var(_SC("wage"), &Employee::wage)
    .Var(_SC("supervisor"), &Employee::supervisor)
    .Var(_SC("middleName"), &Employee::middleName)
    .Var(_SC("gender"), &Employee::gender)
    .StaticVar(_SC("sharedData"), &Employee::sharedData)

    .Func(_SC("GiveRaise"), &Employee::GiveRaise)
    .Func(_SC("_tostring"), &Employee::ToString)
    ;

    // Bind the class to the root table
    RootTable().Bind(_SC("Employee"), employee);

    // Create an employee and set it as an instance in the script
    Employee bob;
    bob.firstName = _SC("Bob");
    bob.lastName = _SC("Smith");
    bob.age = 42;
    bob.department = _SC("Accounting");
    bob.wage = 21.95f;
    bob.gender = _SC("Male");
    bob.middleName = _SC("A");
    bob.sharedData = _SC("1234");
    
    RootTable().SetInstance(_SC("bob"), &bob);

    Script script;

    try {
        script.CompileString(_SC(" \
			steve <- Employee(); \
			steve.firstName = \"Steve\"; \
			steve.lastName = \"Jones\"; \
			steve.age = 34; \
			steve.wage = 35.00; \
			steve.department = \"Management\"; \
			steve.gender = \"male\"; \
			steve.middleName = \"B\"; \
			\
			gTest.EXPECT_INT_EQ(steve.age, 34); \
			gTest.EXPECT_FLOAT_EQ(steve.wage, 35.00); \
			gTest.EXPECT_STR_EQ(steve.lastName, \"Jones\"); \
			gTest.EXPECT_STR_EQ(steve.middleName, \"B\"); \
			gTest.EXPECT_STR_EQ(steve.gender, \"male\"); \
			\
			\
			bob.age += 1; \
			bob.GiveRaise(0.02); \
			bob.supervisor = steve; \
			\
			gTest.EXPECT_INT_EQ(bob.age, 43); \
			gTest.EXPECT_FLOAT_EQ(bob.wage, 22.389); \
			gTest.EXPECT_STR_EQ(bob.lastName, \"Smith\"); \
			gTest.EXPECT_STR_EQ(bob.middleName, \"A\"); \
			gTest.EXPECT_STR_EQ(bob.gender, \"Male\"); \
			gTest.EXPECT_STR_EQ(bob.sharedData, \"1234\"); \
			\
			// Uncomment the following to see _tostring demonstrated \
			//::print(steve); \
			//::print(\"===========\\n\"); \
			//::print(bob); \
			"));
    } catch(Exception ex) {
        FAIL() << _SC("Compile Failed: ") << ex.Message();
    }

    try {
        script.Run();
    } catch(Exception ex) {
        FAIL() << _SC("Run Failed: ") << ex.Message();
    }

    // Since he was set as an instance, changes to Bob in the script carry through to the native object
    EXPECT_EQ(bob.age, 43);
    EXPECT_FLOAT_EQ(bob.wage, 22.389f);

    // Steve can also be retreived from the script as an employee:
    Object steveObj = RootTable().GetSlot(_SC("steve"));
    ASSERT_FALSE(steveObj.IsNull());

    Employee* steve = steveObj.Cast<Employee*>();
    ASSERT_FALSE(steve == NULL);

    EXPECT_EQ(steve->age, 34);
    EXPECT_FLOAT_EQ(steve->wage, 35.00f);
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
    
    B& getB()
    {
        return *this;
    }

    B& getB2(int, char *)
    {
        return *this;
    }

    B& getB4( const B, B *, const B, int)
    {
        return *this;
    }
    
    B* getBPtr()
    {
        return this;
    }
    
    static string shared;
    static int sharedInt;
};


string B::shared ;
int B::sharedInt = -1;

TEST_F(SqratTest, InstanceReferencesAndStaticMembers) {
    DefaultVM::Set(vm);

    Class<B> _B;
    _B
    .Func(_SC("set"), &B::set)
    .Func(_SC("get"), &B::get)
    .Func(_SC("getB"), &B::getB)
    .Func(_SC("getB2"), &B::getB2)
    .Func(_SC("getB4"), &B::getB4)
    .Func(_SC("getBPtr"), &B::getBPtr)
    .StaticVar(_SC("shared"), &B::shared)
    .StaticVar(_SC("sharedInt"), &B::sharedInt);
    
    RootTable().Bind(_SC("B"), _B);
    
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
			local b1 = b.getBPtr();\
            b.set(20);\
			gTest.EXPECT_INT_EQ(b1.get(), 20); \
			local b2 = b.getB();\
            b.set(40);\
			gTest.EXPECT_INT_EQ(b1.get(), 40); \
			gTest.EXPECT_INT_EQ(b2.get(), 40); \
			local b3 = b.getB2(12, \"test\");\
            b.set(60);\
			gTest.EXPECT_INT_EQ(b2.get(), 60); \
			gTest.EXPECT_INT_EQ(b3.get(), 60); \
			local b4 = b.getB4(b, b, b, 33);\
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

class A
{
protected:
    int v;
public:
    A(): v(-1) {}
    int getv() 
    {
        return v;
    }
    
    
};


class AA: public A
{
public:
    int setv(int v_) 
    {
        v = v_;
    }
    
    
};


class AAA: public AA
{
    
    
};

class AB: public A, public B
{
    
    
};

class BB: public B
{
    
};

int abc(A & a, AA & aa, AB *ab)
{
    aa.setv(12);    
    ab->set(34);
    return a.getv();
}

class W
{
public:
    void f1(A &a) {}
    void f2(A * a) {}

    void f3(AAA aaa) 
    {   
    }

    void f4(const AB ab) {}    
    int abc(A * a, AA & aa, B *b)
    {
        aa.setv(12);    
        b->set(34);
        return a->getv();
    }
    
};

        
    
TEST_F(SqratTest, SimpleTypeChecking) {
    DefaultVM::Set(vm);

    Class<B> _B(vm, _SC("B"));
    _B
    .Func(_SC("set"), &B::set)
    .Func(_SC("get"), &B::get)
    .Func(_SC("getB"), &B::getB)
    .Func(_SC("getBPtr"), &B::getBPtr);
    
    RootTable().Bind(_SC("B"), _B);

    DerivedClass<BB, B> _BB(vm, _SC("BB"));
    RootTable().Bind(_SC("BB"), _BB);
    
    Class<A> _A(vm, _SC("A"));
    RootTable().Bind(_SC("A"), _A);
    DerivedClass<AA, A> _AA(vm, _SC("AA"));
    RootTable().Bind(_SC("AA"), _AA);
    DerivedClass<AAA, A> _AAA(vm, _SC("AAA"));
    RootTable().Bind(_SC("AAA"), _AAA);
    DerivedClass<AB, B> _AB(vm, _SC("AB"));
    RootTable().Bind(_SC("AB"), _AB);
    Class<W> _W(vm, _SC("W"));
    _W.Func(_SC("f1"), &W::f1);
    _W.Func(_SC("f2"), &W::f2);
    _W.Func(_SC("f3"), &W::f3);
    _W.Func(_SC("f4"), &W::f4);
    _W.Func(_SC("abc"), &W::abc);
    
    RootTable().Bind(_SC("W"), _W);

    RootTable().Func(_SC("abc"), &abc);
    
    Script script;
    try {
        script.CompileString(_SC(" \
            b <- B();\
            bb <- BB(); \
            a <- A(); \
            aa <- AA(); \
            aaa <- AAA(); \
            ab <- AB(); \
            abc(a, aa, ab); \
            w <- W(); \
            w.f1(a); \
            w.f1(aaa); \
            w.f2(a); \
            w.f3(aaa); \
            w.f4(ab); \
            w.abc(aaa, aa, bb); \
            w.abc(aa, aa, b); \
            \
            local raised = false;\
            try { \
                w.f1(b);\
			    gTest.EXPECT_INT_EQ(0, 1); \
            }\
            catch (ex) {\
                raised = true;\
                print(ex + \"\\n\"); \
            }\
            gTest.EXPECT_TRUE(raised); \
            \
            raised = false;\
            try { \
                w.abc(aa, a, ab); \
			    gTest.EXPECT_INT_EQ(0, 1); \
            }\
            catch (ex) {\
                raised = true;\
                print(ex + \"\\n\"); \
            }\
            gTest.EXPECT_TRUE(raised); \
            \
            \
            raised = false;\
            try { \
                w.f3(a); \
			    gTest.EXPECT_INT_EQ(0, 1); \
            }\
            catch (ex) {\
                raised = true;\
                print(ex + \"\\n\"); \
            }\
            gTest.EXPECT_TRUE(raised); \
            \
            \
            raised = false;\
            try { \
                w.abc(a, aa); \
			    gTest.EXPECT_INT_EQ(0, 1); \
            }\
            catch (ex) {\
                raised = true;\
                print(ex + \"\\n\"); \
            }\
            gTest.EXPECT_TRUE(raised); \
            \
            raised = false;\
            try { \
                w.f4(ab, b); \
			    gTest.EXPECT_INT_EQ(0, 1); \
            }\
            catch (ex) {\
                raised = true;\
                print(ex + \"\\n\"); \
            }\
            gTest.EXPECT_TRUE(raised); \
            \
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

    