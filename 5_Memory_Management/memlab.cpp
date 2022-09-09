#include "memlab.hpp"

#include <bits/stdc++.h>
#include <fcntl.h>

using namespace std;

Stack::Stack() { arr = new int[MAX_VAR]; }

void Stack::push(int data) {
    top_pos++;
    arr[top_pos] = data;
}

int Stack::empty() { return (top_pos == -1); }

void Stack::pop() {
    if (empty()) return;
    top_pos--;
}

int Stack::top() {
    if (empty()) return INT_MIN;
    return arr[top_pos];
}
size_t Stack::size() { return top_pos + 1; }
Stack::~Stack() { delete[] arr; }

size_t VMemory::getsize(TYPE type, int len) {
    int sz = 4;
    if (len > 1) {
        if (type == INTEGER) {
            sz = floor(4 * len, 4) * 4;
        } else if (type == CHARACTER) {
            sz = floor(1 * len, 4) * 4;
        } else if (type == BOOLEAN) {
            sz = floor(floor(len, 32), 4) * 4;
        } else if (type == MID_INTEGER) {
            sz = floor(3 * len, 4) * 4;
        }
    }
    return sz;
}

int VMemory::createMem(size_t size, size_t max_var) {
    start = std::chrono::steady_clock::now();

    int sema = sem_init(&mutex, 1, 1);

    memsize = size;
    memory = (char*)calloc(size * 1024 * 1024, sizeof(char));
    // memset(memory, -1, size * 1024 * 1024);
    pgTable = (PageTableEntry*)calloc(max_var, sizeof(PageTableEntry));

    // memory_footprint_fd = fopen("memory_demo2_without.csv", "w+");
    // if (memory_footprint_fd == 0) {
    //     perror("Error in opening file : ");
    // }

    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, garbage_collector, this);

    // pthread_attr_init(&g_attr);
    // pthread_create(&g_tid, &g_attr, print_memory_footprint, this);

    return 1;
}

int VMemory::createVar(TYPE var_type) {
    sem_wait(&mutex);

    pgTable[count].ptr = memory + offset;
    pgTable[count].valid_bit = true;
    pgTable[count].var_type = var_type;
    pgTable[count].len = 1;

    offset += 4;
    var_stack.push(count);

    count++;

    cerr << "Var Created of type : " << var_type << endl;

    sem_post(&mutex);
    return count - 1;
}
int VMemory::createArr(TYPE var_type, size_t len) {
    sem_wait(&mutex);
    pgTable[count].ptr = memory + offset;
    pgTable[count].valid_bit = true;
    pgTable[count].var_type = var_type;
    pgTable[count].len = len;

    offset += getsize(var_type, len);
    var_stack.push(count);

    count++;

    cerr << "Arr created of type : " << var_type << " and length " << len
         << endl;

    sem_post(&mutex);
    return count - 1;
}

int VMemory::assignVar(pointer adr, int val) {
    try {
        sem_wait(&mutex);
        if (pgTable[adr].len > 1) {
            throw "Input is Array and not variable";
        } else if (!pgTable[adr].valid_bit) {
            throw "The variable is not valid";
        }
        if (pgTable[adr].var_type == INTEGER) {
            if (val < -2147483648 || val > 2147483647) {
                throw "Not a valid type";
            }
            unsigned int mask = 0b00000000000000000000000000000000;
            int* ptr = (int*)pgTable[adr].ptr;
            *ptr &= mask;
            *ptr |= (val << 0);
        } else if (pgTable[adr].var_type == CHARACTER) {
            if (val < -128 || val > 127) {
                throw "Not a valid type";
            }
            unsigned int mask = 0b00000000000000000000000000000000;
            int* ptr = (int*)pgTable[adr].ptr;
            *ptr &= mask;
            *ptr |= (val << 0);
        } else if (pgTable[adr].var_type == BOOLEAN) {
            if (val < 0 || val > 1) {
                throw "Not a valid type";
            }
            unsigned int mask = 0b00000000000000000000000000000000;
            int* ptr = (int*)pgTable[adr].ptr;
            *ptr &= mask;
            *ptr |= (((int)(val != 0)) << 0);
        } else if (pgTable[adr].var_type == MID_INTEGER) {
            if (val < -8388608 || val > 8388607) {
                cout << val << endl;
                throw "Not a valid type";
            }
            unsigned int mask = 0b00000000000000000000000000000000;
            int* ptr = (int*)pgTable[adr].ptr;
            *ptr &= mask;
            *ptr |= (val << 0);
        }
        // cerr << "Variable : " << adr << " assigned value " << val << endl;
        sem_post(&mutex);
    } catch (const char* msg) {
        cerr << msg << endl;
        sem_post(&mutex);
        exit(0);
    }

    return 1;
}
int VMemory::readVar(pointer adr) {
    try {
        sem_wait(&mutex);
        if (pgTable[adr].len > 1) {
            throw "Input is Array and not variable";
        } else if (!pgTable[adr].valid_bit) {
            throw "The variable is not valid";
        }
        unsigned int mask = 0b00000000000000000000000000000000;
        if (pgTable[adr].var_type == INTEGER) {
            int* ptr = (int*)pgTable[adr].ptr;
            mask |= *ptr;
        } else if (pgTable[adr].var_type == CHARACTER) {
            int* ptr = (int*)pgTable[adr].ptr;
            mask |= *ptr;
        } else if (pgTable[adr].var_type == BOOLEAN) {
            int* ptr = (int*)pgTable[adr].ptr;
            mask |= *ptr;
        } else if (pgTable[adr].var_type == MID_INTEGER) {
            int* ptr = (int*)pgTable[adr].ptr;
            mask |= *ptr;
        }
        cerr << "Variable read : " << adr << endl;

        sem_post(&mutex);
        return mask;
    } catch (const char* msg) {
        cerr << msg << endl;
        sem_post(&mutex);
        exit(0);
    }

    return 1;
}
int VMemory::assignArr(pointer adr, int index, int val) {
    try {
        sem_wait(&mutex);
        if (index >= pgTable[adr].len || index < 0) {
            throw "Index Out of Bound";
        } else if (!pgTable[adr].valid_bit) {
            throw "The array is not valid";
        }
        if (pgTable[adr].var_type == INTEGER) {
            if (val < -2147483648 || val > 2147483647) {
                throw "Not a valid type";
            }
            int* ptr = (int*)(pgTable[adr].ptr + index * 4);
            unsigned int mask = 0b00000000000000000000000000000000;
            *ptr &= mask;
            *ptr |= (val << 0);
        } else if (pgTable[adr].var_type == CHARACTER) {
            if (val < -128 || val > 127) {
                throw "Not a valid type";
            }
            int* ptr = (int*)(pgTable[adr].ptr + index * 1);
            unsigned int mask = 0b11111111111111111111111100000000;
            *ptr &= mask;
            *ptr |= (val << 0);
        } else if (pgTable[adr].var_type == BOOLEAN) {
            if (val < 0 || val > 1) {
                throw "Not a valid type";
            }
            int* ptr = (int*)(pgTable[adr].ptr + index / 32);
            int t = index % 32;
            unsigned int mask = 0b11111111111111111111111111111111;
            mask &= ~((unsigned int)1 << t);
            *ptr &= mask;
            *ptr |= ((unsigned int)val << t);
        } else if (pgTable[adr].var_type == MID_INTEGER) {
            if (val < -8388608 || val > 8388607) {
                throw "Not a valid type";
            }
            int* ptr = (int*)(pgTable[adr].ptr + index * 3);
            unsigned int mask = 0b11111111000000000000000000000000;
            *ptr &= mask;
            *ptr |= (val << 0);
        }
        // cerr << "Arr : " << adr << " assigned " << val << " at index " << val
        //      << endl;
        sem_post(&mutex);
    } catch (const char* msg) {
        cerr << msg << endl;
        sem_post(&mutex);
        exit(0);
    }
    return 1;
}
int VMemory::readArr(pointer adr, int index) {
    try {
        sem_wait(&mutex);
        if (index >= pgTable[adr].len || index < 0) {
            throw "Index Out of Bound";
        } else if (!pgTable[adr].valid_bit) {
            throw "The array is not valid";
        }
        int data;
        if (pgTable[adr].var_type == INTEGER) {
            int* ptr = (int*)(pgTable[adr].ptr + index * 4);
            unsigned int mask = 0b00000000000000000000000000000000;
            mask |= *ptr;
            data = *ptr;
        } else if (pgTable[adr].var_type == CHARACTER) {
            int* ptr = (int*)(pgTable[adr].ptr + index * 1);
            unsigned int mask = 0b00000000000000000000000000000000;
            mask |= *ptr;
            data = (*ptr << 24) >> 24;
        } else if (pgTable[adr].var_type == BOOLEAN) {
            int* ptr = (int*)(pgTable[adr].ptr + index / 32);
            int t = index % 32;
            unsigned int mask = 0b00000000000000000000000000000000;
            mask |= *ptr;
            mask &= ((unsigned int)1 << t);
            data = (mask != 0);
        } else if (pgTable[adr].var_type == MID_INTEGER) {
            int* ptr = (int*)(pgTable[adr].ptr + index * 3);
            unsigned int mask = 0b00000000000000000000000000000000;
            mask |= *ptr;
            data = (*ptr << 8) >> 8;
        }

        sem_post(&mutex);
        return data;
    } catch (const char* msg) {
        cerr << msg << endl;
        sem_post(&mutex);
        exit(0);
    }
    return 1;
}

void VMemory::freeElem(pointer adr) {
    try {
        if (adr >= count || adr < 0) {
            throw "Not a valid pointer";
        }
        pgTable[adr].valid_bit = false;
        last_valid = min(last_valid, adr - 1);
        size_t var_size = getsize(pgTable[adr].var_type, pgTable[adr].len);
        memory_holes += var_size;
        cerr << "Memory Holes : " << memory_holes << endl;
        cerr << "pointer deleted : " << adr << " size : " << var_size << endl;

    } catch (const char* msg) {
        cerr << msg << endl;
        sem_post(&mutex);
        exit(0);
    }
}

void VMemory::fn_begin() {
    sem_wait(&mutex);
    var_stack.push(-1);
    cerr << "function begins " << endl;
    sem_post(&mutex);
}
void VMemory::fn_end(int return_something, int return_value, TYPE return_type) {
    sem_wait(&mutex);
    while (!var_stack.empty() && var_stack.top() != -1) {
        pointer t = var_stack.top();
        freeElem(t);

        offset -= getsize(pgTable[t].var_type, pgTable[t].len);
        memory_holes -= getsize(pgTable[t].var_type, pgTable[t].len);
        cerr << "Memory used by pointer " << t << " freed" << endl;
        count--;

        var_stack.pop();
    }
    if (!var_stack.empty()) {
        var_stack.pop();
    }

    cerr << "function ends" << endl;
    sem_post(&mutex);
    if (return_something) {
        pointer p = createVar(return_type);
        assignVar(p, return_value);
    }
}
pointer VMemory::returned_value() {
    sem_wait(&mutex);
    pointer t = var_stack.top();
    sem_post(&mutex);
    return t;
}
size_t VMemory::get_memory_footprint() {
    return count * sizeof(PageTableEntry) + offset * sizeof(char);
}

VMemory::~VMemory() {
    // while (pthread_cancel(g_tid) != 0)
    //     ;
    // pthread_join(g_tid, NULL);
    while (pthread_cancel(tid) != 0)
        ;
    pthread_join(tid, NULL);
    // sleep(1);
    // fclose(memory_footprint_fd);
    free(memory);
    free(pgTable);
    sem_destroy(&mutex);
}

void* garbage_collector(void* param) {
    VMemory* vmem = (VMemory*)param;

    while (true) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
        if (vmem->memory_holes >= MAX_HOLE) {
            sem_wait(&(vmem->mutex));

            if (vmem->memory_holes < MAX_HOLE) {
                sem_post(&(vmem->mutex));
                continue;
            }
            cerr << "Garbage collector started, current memory holes : "
                 << vmem->memory_holes << endl;
            vmem->memory_holes = 0;
            char* mem = vmem->memory;

            mem = vmem->pgTable[vmem->last_valid + 1].ptr;
            for (int i = 0; i < vmem->count; i++) {
                if (vmem->pgTable[i].valid_bit == true) {
                    if (mem != vmem->pgTable[i].ptr) {
                        memcpy(mem, vmem->pgTable[i].ptr,
                               vmem->getsize(vmem->pgTable[i].var_type,
                                             vmem->pgTable[i].len));
                        vmem->pgTable[i].ptr = mem;
                    }
                    mem += vmem->getsize(vmem->pgTable[i].var_type,
                                         vmem->pgTable[i].len);
                }
            }
            vmem->offset = mem - vmem->memory;
            cerr << "Memory Compacted, current memory holes : "
                 << vmem->memory_holes << endl;
            sem_post(&(vmem->mutex));
        } else {
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
            usleep(100);
        }
    }
    return NULL;
}
void* print_memory_footprint(void* param) {
    VMemory* vmem = (VMemory*)param;

    while (true) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
        std::chrono::steady_clock::time_point end =
            std::chrono::steady_clock::now();
        int tm = std::chrono::duration_cast<std::chrono::microseconds>(
                     end - vmem->start)
                     .count();
        fprintf(vmem->memory_footprint_fd, "%d,%ld\n", tm,
                vmem->get_memory_footprint());
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
        usleep(1);
    }
    return NULL;
}