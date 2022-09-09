#include <bits/stdc++.h>

#include "memlab.hpp"

using namespace std;

VMemory vmem;

int main() {
    vmem.createMem(256);
    vmem.fn_begin();
    integer var = vmem.createVar(INTEGER);
    vmem.assignVar(var, -1);

    integer var1 = vmem.createArr(MID_INTEGER, 10);
    integer var2 = vmem.createArr(INTEGER, 10);

    for (int i = 0; i < 10; i++) {
        vmem.assignArr(var1, i, 50 * i);
        vmem.assignArr(var2, i, 60 * i);
    }
    for (int i = 0; i < 10; i++) {
        cout << vmem.readArr(var1, i) << " " << vmem.readArr(var2, i) << endl;
    }
    vmem.freeElem(var1);
    vmem.freeElem(var2);
    usleep(100);

    integer var3 = vmem.createArr(INTEGER, 10);
    integer var4 = vmem.createArr(INTEGER, 10);

    for (int i = 0; i < 10; i++) {
        vmem.assignArr(var3, i, 0);
        vmem.assignArr(var4, i, 0);
    }
    vmem.freeElem(var1);
    vmem.freeElem(var2);
    usleep(100);
    cout << "Variable read : " << vmem.readVar(var) << endl;

    vmem.fn_end();
    return 0;
}