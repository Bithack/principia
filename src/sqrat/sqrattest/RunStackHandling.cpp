//
// Copyright (c) 2012 Li-Cheng (Andy) Tai, atai@atai.org
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

/* this test would generally not fail.  Its main purpose is to test for leaks 
   when compiling scripts in memory multiple times; need to use valgrind or just watch
   memory increase when run */
   
using namespace Sqrat;

//#define DUMPSTACK sq_dumpstack(vm); /* require atai's version of Squirrel */
#define DUMPSTACK 

TEST_F(SqratTest, RunStackHandling)
{
    DefaultVM::Set(vm);
    static const int num_run = 1024 * 10 ; /* 10 times the typical Squirrel stack size */
    int i; 
    Script script;

    try
    {
        script.CompileString(_SC("function f(a) { return a + 1; } ; t <- 1;  t = f(t); "));
    }
    catch(Exception ex)
    {
        FAIL() << _SC("Compile failed: ") << ex.Message();
    }    
    try
    {
        script.Run();
        Script script2;
        DUMPSTACK
        for (i = 0; i < num_run; i++)
        {
            
            script2.CompileString(_SC("t = f(t); /*print(t.tostring() + \"\\n\");*/"));
            script2.Run();  
            DUMPSTACK
        }
    }
    catch (Exception ex)
    {
        FAIL() << _SC("Run failed: ") << ex.Message();
    }
    string err_msg;
    bool b ;

    for (i = 0; i < num_run; i++)
    {
        Script script3;
        err_msg = _SC("");
        script3.CompileString(_SC("t = f(t); /*print(t.tostring() + \"\\n\");*/"));
        b = script3.Run(err_msg);    
        ASSERT_TRUE(b);
        ASSERT_TRUE(err_msg == _SC(""));
        DUMPSTACK
    }
    
}