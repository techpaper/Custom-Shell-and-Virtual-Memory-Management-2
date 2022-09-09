#include <bits/stdc++.h>

#include "memlab.hpp"

using namespace std;

VMemory vmem;

void fun1(integer x, integer y) {
    vmem.fn_begin();
    integer var = vmem.createArr(INTEGER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % INT32_MAX);
    }
    vmem.fn_end();
}
void fun2(character x, character y) {
    vmem.fn_begin();
    character var = vmem.createArr(CHARACTER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % (26) + 'A');
    }
    vmem.fn_end();
}
void fun3(boolean x, boolean y) {
    vmem.fn_begin();
    boolean var = vmem.createArr(BOOLEAN, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % 2);
    }
    vmem.fn_end();
}
void fun4(mid_integer x, mid_integer y) {
    vmem.fn_begin();
    mid_integer var = vmem.createArr(MID_INTEGER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % 8388608);
    }
    vmem.fn_end();
}
void fun5(integer x, integer y) {
    vmem.fn_begin();
    integer var = vmem.createArr(INTEGER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % INT32_MAX);
    }
    vmem.fn_end();
}
void fun6(character x, character y) {
    vmem.fn_begin();
    character var = vmem.createArr(CHARACTER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % (26) + 'A');
    }
    vmem.fn_end();
}
void fun7(boolean x, boolean y) {
    vmem.fn_begin();
    boolean var = vmem.createArr(BOOLEAN, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % 2);
    }
    vmem.fn_end();
}
void fun8(mid_integer x, mid_integer y) {
    vmem.fn_begin();
    mid_integer var = vmem.createArr(MID_INTEGER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % 8388608);
    }
    vmem.fn_end();
}
void fun9(integer x, integer y) {
    vmem.fn_begin();
    integer var = vmem.createArr(INTEGER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % INT32_MAX);
    }
    vmem.fn_end();
}
void fun10(character x, character y) {
    vmem.fn_begin();
    character var = vmem.createArr(CHARACTER, 50000);
    for (int i = 0; i < 50000; i++) {
        vmem.assignArr(var, i, rand() % (26) + 'A');
    }
    vmem.fn_end();
}

int main() {
    vmem.createMem(256);
    vmem.fn_begin();
    integer var1 = vmem.createVar(INTEGER);
    integer var2 = vmem.createVar(INTEGER);

    character var3 = vmem.createVar(CHARACTER);
    character var4 = vmem.createVar(CHARACTER);

    boolean var5 = vmem.createVar(BOOLEAN);
    boolean var6 = vmem.createVar(BOOLEAN);
    
    mid_integer var7 = vmem.createVar(MID_INTEGER);
    mid_integer var8 = vmem.createVar(MID_INTEGER);

    integer var9 = vmem.createArr(INTEGER, 5000);
    integer var10 = vmem.createArr(INTEGER, 5000);

    character var11 = vmem.createArr(CHARACTER, 5000);
    character var12 = vmem.createArr(CHARACTER, 5000);

    boolean var13 = vmem.createArr(BOOLEAN, 5000);
    boolean var14 = vmem.createArr(BOOLEAN, 5000);

    mid_integer var15 = vmem.createArr(MID_INTEGER, 5000);
    mid_integer var16 = vmem.createArr(MID_INTEGER, 5000);

    fun1(var1, var2);
    fun2(var3, var4);
    fun3(var5, var6);
    fun4(var7, var8);
    fun5(var9, var10);
    fun6(var11, var12);
    fun7(var13, var14);
    fun8(var15, var16);
    fun9(var1, var2);
    fun10(var3, var4);



    vmem.fn_end();
    return 0;
}