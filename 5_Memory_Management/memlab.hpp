#ifndef __MEMLAB
#define __MEMLAB

#include <bits/stdc++.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <ctime>

using namespace std;

#define MEMSIZE 256  // in MB
#define MAX_VAR \
    2097152  // 64 MB for bookkeeping, 16777216 is max number of variable of
             // largest size
#define MAX_HOLE 5
typedef int pointer;
typedef int integer;
typedef int boolean;
typedef int character;
typedef int mid_integer;

int inline floor(int a, int b) { return (a % b == 0) ? a / b : a / b + 1; }

enum TYPE {
    INTEGER,
    CHARACTER,
    MID_INTEGER,
    BOOLEAN,
};

struct Stack {
    int* arr;
    int top_pos = -1;
    Stack();

    void push(int data);

    int empty();

    void pop();

    int top();
    size_t size();
    ~Stack();
};

struct PageTableEntry {
    char* ptr;
    bool valid_bit;
    TYPE var_type;
    int len;
};

struct VMemory {
    PageTableEntry* pgTable;
    char* memory;
    size_t memsize;
    size_t count = 0;
    size_t offset = 0;
    size_t memory_holes = 0;
    sem_t mutex;

    pointer last_valid = -1;

    Stack var_stack;

    pthread_t tid;
    pthread_attr_t attr;

    pthread_t g_tid;
    pthread_attr_t g_attr;

    FILE* memory_footprint_fd;
    std::chrono::steady_clock::time_point start;

    static size_t getsize(TYPE type, int len);

    int createMem(size_t size = MEMSIZE, size_t max_var = MAX_VAR);

    pointer createVar(TYPE var_type);
    pointer createArr(TYPE var_type, size_t size);

    int assignVar(pointer adr, int val);
    int assignArr(pointer adr, int index, int val);

    int readVar(pointer adr);
    int readArr(pointer adr, int index);

    void freeElem(pointer adr);

    void fn_begin();
    void fn_end(int return_something = false, int return_value = 0,
                TYPE return_type = INTEGER);

    pointer returned_value();

    size_t get_memory_footprint();

    ~VMemory();
};
void* garbage_collector(void* param);
void* print_memory_footprint(void* param);

#endif