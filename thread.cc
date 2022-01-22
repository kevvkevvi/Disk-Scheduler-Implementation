// thread.cc - implementation of a user-level thread library
// Kevin Li and Liam Juskevice 

#include "ucontext.h"
#include "thread.h"
#include "interrupt.h"
#include <string.h>
#include <queue>
#include <iostream>
#include <map>
#include <algorithm>
#include <vector>

using namespace std;

//thread_queue is the ready queue
static queue<ucontext_t*> thread_queue;
//the current thread pointer
static ucontext_t* curr_pointer;
//the previous thread pointer
static ucontext_t* prev_pointer;
//a map of the locks, key: lock, value[0]-> current owner of lock,
//value[1]-> lock queue for that lock
static map<unsigned, tuple<ucontext_t*, vector<ucontext_t*>>> locks;
//a map of cvs, key: condition varable & lock
static map<tuple<unsigned, unsigned>, tuple<ucontext_t*, vector<ucontext_t*>>> cvs;
//checks if the library is initialized or not
static bool initialize = false;

//static bool from_lib = false;

void delete_current_thread(){
    //delete stack and pointer
    curr_pointer->uc_stack.ss_sp = 0;
    curr_pointer->uc_stack.ss_size = 0;
    curr_pointer->uc_stack.ss_flags = 0;
    delete curr_pointer;
}

void caller(thread_startfunc_t func, void* arg) {
    //run function
    interrupt_enable();
    func(arg);
    interrupt_disable();
    //delete thread after finished running
    delete_current_thread();
    //context switch
    if (thread_queue.size() != 0) {
        ucontext_t* next_pointer = thread_queue.front();
        thread_queue.pop();
        prev_pointer = curr_pointer;
        curr_pointer = next_pointer;
        swapcontext(prev_pointer, curr_pointer);
    }
    //exit when ready queue is empty
    if (thread_queue.size() == 0) {
        cout << "Thread library exiting.\n";
        exit(0);
    }
    interrupt_enable();
    return;
}

int thread_libinit(thread_startfunc_t func, void* arg) {
    if (initialize) {
       return -1;
    } 
    interrupt_disable();
    initialize = true;
    ucontext_t* ucontext_ptr = new ucontext_t;
    curr_pointer = ucontext_ptr;
    getcontext(ucontext_ptr);
    char* stack = new char[STACK_SIZE];
    ucontext_ptr->uc_stack.ss_sp = stack;
    ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
    ucontext_ptr->uc_stack.ss_flags = 0;
    ucontext_ptr->uc_link = NULL;
    makecontext(ucontext_ptr,(void(*)()) caller, 2, func, arg);
    setcontext(ucontext_ptr);
    cout << "enabled interrupts" << endl;
    return 0;
}

int thread_create(thread_startfunc_t func, void* arg) {
    if (!initialize) {
       return -1;
    } 
    interrupt_disable();
    ucontext_t* ucontext_ptr = new ucontext_t;
    getcontext(ucontext_ptr);
    char* stack = new char[STACK_SIZE];
    ucontext_ptr->uc_stack.ss_sp = stack;
    ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
    ucontext_ptr->uc_stack.ss_flags = 0;
    ucontext_ptr->uc_link = NULL;
    makecontext(ucontext_ptr,(void(*)()) caller, 2, func, arg);
    thread_queue.push(ucontext_ptr);
    interrupt_enable();
    return 0;
}

int thread_yield() {
    if (!initialize) {
       return -1;
    }
    interrupt_disable(); 
    if (thread_queue.size() > 0) { 
        ucontext_t* next_pointer = thread_queue.front();
        thread_queue.pop();
        thread_queue.push(curr_pointer);
        prev_pointer = curr_pointer;
        curr_pointer = next_pointer;
        swapcontext(prev_pointer, curr_pointer);
    }
    interrupt_enable();
    return 0;
}

int thread_lock(unsigned lock) {
    if (!initialize) {
       return -1;
    }
    interrupt_disable(); 
    vector<ucontext_t*> lock_queue;
    //checks if lock is in lock map
    if (locks.find(lock) == locks.end()) {
        locks.emplace(lock, make_tuple(curr_pointer, lock_queue));
        interrupt_enable();
        return 0;
    }
    else {
        lock_queue = get<1>(locks.at(lock));
        if (get<0>(locks.at(lock)) == curr_pointer) {
            interrupt_enable();
            return -1;
        }
        if (find(lock_queue.begin(), lock_queue.end(), curr_pointer) != lock_queue.end()) {
            interrupt_enable();
            return -1;
        }
        else {
            get<1>(locks.at(lock)).push_back(curr_pointer);
            prev_pointer = curr_pointer;
            if (thread_queue.size() == 0) {
                cout << "Thread library exiting.\n";
                exit(0);
            }
            else {
                curr_pointer = thread_queue.front();
                thread_queue.pop();
                swapcontext(prev_pointer, curr_pointer);
            }
            interrupt_enable();
            return 0;
        }
    }
    interrupt_enable();
    return -1;
}

int thread_unlock(unsigned lock) {
    if (!initialize) {
       return -1;
    }
    interrupt_disable();
    if (locks.find(lock) == locks.end()){
        interrupt_enable();
        return -1;
    } 
    if (get<0>(locks.at(lock)) != curr_pointer) {
        interrupt_enable();
        return -1;
    }
    if (get<1>(locks.at(lock)).size() == 0) {
        locks.erase(lock);
        interrupt_enable();
        return 0;
    }
    else {
        //we transition the control of the lock to the next in line of lock queue
        get<0>(locks.at(lock)) = get<1>(locks.at(lock)).front();
        thread_queue.push(get<0>(locks.at(lock)));
        get<1>(locks.at(lock)).erase(get<1>(locks.at(lock)).begin());
        interrupt_enable();
        return 0;
    }
}


int no_interrupt_lock(unsigned lock) {
    if (!initialize) {
       return -1;
    }
    vector<ucontext_t*> lock_queue;
    //checks if lock is in lock map
    if (locks.find(lock) == locks.end()) {
        locks.emplace(lock, make_tuple(curr_pointer, lock_queue));
        return 0;
    }
    else {
        lock_queue = get<1>(locks.at(lock));
        if (get<0>(locks.at(lock)) == curr_pointer) {
            return -1;
        }
        if (find(lock_queue.begin(), lock_queue.end(), curr_pointer) != lock_queue.end()) {
            return -1;
        }
        else {
            get<1>(locks.at(lock)).push_back(curr_pointer);
            prev_pointer = curr_pointer;
            if (thread_queue.size() == 0) {
                cout << "Thread library exiting.\n";
                exit(0);
            }
            else {
                curr_pointer = thread_queue.front();
                thread_queue.pop();
                swapcontext(prev_pointer, curr_pointer);
            }
            return 0;
        }
    }
    return -1;
}

int no_interrupt_unlock(unsigned lock) {
    if (!initialize) {
       return -1;
    }
    if (locks.find(lock) == locks.end()){
        return -1;
    } 
    if (get<0>(locks.at(lock)) != curr_pointer) {
        return -1;
    }
    if (get<1>(locks.at(lock)).size() == 0) {
        locks.erase(lock);
        return 0;
    }
    else {
        //we transition the control of the lock to the next in line of lock queue
        get<0>(locks.at(lock)) = get<1>(locks.at(lock)).front();
        thread_queue.push(get<0>(locks.at(lock)));
        get<1>(locks.at(lock)).erase(get<1>(locks.at(lock)).begin());
        return 0;
    }
}

int thread_wait(unsigned lock, unsigned cond) {
    if (!initialize) {
       return -1;
    }
    interrupt_disable();
    if (locks.find(lock) == locks.end()) {
        interrupt_enable();
        return -1;
    }
    else {
        if (curr_pointer != get<0>(locks.at(lock))) {
            interrupt_enable();
            return -1;
        }
        vector<ucontext_t*> cv_queue;
        if (no_interrupt_unlock(lock)) {
            interrupt_enable();
            return -1;
        }
        if (cvs.find(make_tuple(cond, lock)) == cvs.end()) {
            cv_queue.push_back(curr_pointer);
            cvs.emplace(make_tuple(cond, lock), make_tuple(curr_pointer, cv_queue));
        }
        else {
            get<1>(cvs.at(make_tuple(cond, lock))).push_back(curr_pointer);
        }
        prev_pointer = curr_pointer;
        if (thread_queue.size() != 0) {
            curr_pointer = thread_queue.front();
            thread_queue.pop();
            swapcontext(prev_pointer, curr_pointer);
            if (no_interrupt_lock(lock)) {
                interrupt_enable();
                return -1;
            }
        }
        interrupt_enable();
        return 0;
    }
}

int thread_signal(unsigned lock, unsigned cond) {
    if (!initialize) {
       return -1;
    }
    interrupt_disable();
    if (cvs.find(make_tuple(cond, lock)) == cvs.end()){
        interrupt_enable();
        return 0;
    }
    if (get<1>(cvs.at(make_tuple(cond,lock))).size() == 0) {
        //cvs.erase(cond);
        interrupt_enable();
        return 0;
    }
    else {
        get<0>(cvs.at(make_tuple(cond, lock))) = get<1>(cvs.at(make_tuple(cond, lock))).front();
        thread_queue.push(get<0>(cvs.at(make_tuple(cond, lock))));
        get<1>(cvs.at(make_tuple(cond, lock))).erase(get<1>(cvs.at(make_tuple(cond, lock))).begin());
        interrupt_enable();
        return 0;
    }
    interrupt_enable();
    return -1;
}

int thread_broadcast(unsigned lock, unsigned cond) {
    if (!initialize) {
       return -1;
    }
    interrupt_disable();
    if (cvs.find(make_tuple(cond, lock)) == cvs.end()){
        interrupt_enable();
        return 0;
    }
    if (curr_pointer != get<0>(locks.at(lock))) {
        interrupt_enable();
        return -1;
    }
    if (get<1>(cvs.at(make_tuple(cond, lock))).size() == 0) {
        //cvs.erase(cond);
        interrupt_enable();
        return 0;
    }
    else {
        while (get<1>(cvs.at(make_tuple(cond, lock))).size() != 0) {
            get<0>(cvs.at(make_tuple(cond, lock))) = get<1>(cvs.at(make_tuple(cond, lock))).front();
            thread_queue.push(get<0>(cvs.at(make_tuple(cond, lock))));
            get<1>(cvs.at(make_tuple(cond, lock))).erase(get<1>(cvs.at(make_tuple(cond, lock))).begin());
        }
        thread_queue.push(get<0>(cvs.at(make_tuple(cond, lock))));
        interrupt_enable();
        return 0;
    }
    interrupt_enable();
    return -1;
}

