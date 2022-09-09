#include <bits/stdc++.h>

#include "memlab.hpp"

using namespace std;

VMemory vmem;

void fibonacci(integer k) {
    vmem.fn_begin();
    integer var = vmem.createArr(INTEGER, vmem.readVar(k));
    for (int i = 0; i < k; i++) {
        vmem.assignArr(var, i, 0);
    }
    integer var1 = vmem.createVar(INTEGER);
    vmem.assignVar(var1, 1);

    if (vmem.readVar(k) == vmem.readVar((var1))) {
        vmem.fn_end(true, vmem.readArr(var, vmem.readVar(k) - 1), INTEGER);
        return;
    }
    vmem.assignArr(var, 1, 1);
    for (int i = 2; i < vmem.readVar(k); i++) {
        vmem.assignArr(var, i,
                       vmem.readArr(var, i - 1) + vmem.readArr(var, i - 2));
    }
    vmem.fn_end(true, vmem.readArr(var, vmem.readVar(k) - 1), INTEGER);
    return;
}

int main() {
    vmem.createMem(256);
    vmem.fn_begin();
    integer var = vmem.createVar(INTEGER);
    vmem.assignVar(var, 25);
    fibonacci(var);
    integer res = vmem.returned_value();
    cout << "The " << vmem.readVar(var)
         << "th Fibonacci number is : " << vmem.readVar(res) << endl;

    vmem.fn_end();
    return 0;
}