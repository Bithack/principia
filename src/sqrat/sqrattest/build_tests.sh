#!/bin/sh -ex

#shopt -s nullglob

SQUIRREL_INCLUDE=/usr/local/include/squirrel
SQUIRREL_LIB=/usr/local/lib
CFLAGS="-g -O0 -I. -I../include -I../gtest-1.3.0/include -I${SQUIRREL_INCLUDE}" 
LDFLAGS=-L${SQUIRREL_LIB}
LIBS="../gtest-1.3.0/libgtest.a -lsqstdlib -lsquirrel -lstdc++ -lm "

mkdir -p bin

gcc $CFLAGS \
     ../sqimport/sqratimport.cpp ImportTest.cpp Main.cpp \
     -o bin/ImportTest  ${LDFLAGS} ${LIBS} -ldl
     
TEST_CPPS="ClassBinding.cpp\
    ClassInstances.cpp\
    ClassProperties.cpp\
    ConstBindings.cpp\
    FunctionOverload.cpp\
    ScriptLoading.cpp\
    SquirrelFunctions.cpp\
    TableBinding.cpp\
    FunctionParams.cpp \
    RunStackHandling.cpp \
    SuspendVM.cpp \
    NullPointerReturn.cpp\
    FuncInputArgumentType.cpp \
    ArrayBinding.cpp "

for f in $TEST_CPPS; do
    gcc $CFLAGS \
    ${f} Vector.cpp Main.cpp \
    -o bin/${f%.cpp}  ${LDFLAGS} ${LIBS}
done
