// A sample test program for the thread library.

#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#define lock 12
#define cv1 42

using namespace std;

void bloop(void* a) {
  thread_lock(lock);
  cout << "meep" << endl;
  thread_signal(lock,cv1);
  cout << "and back to you mom" << endl;
  thread_wait(lock,2);
  cout << "woops" << endl;
  thread_unlock(lock);
}

void parent(void* a) {
    int arg = (intptr_t) a;
    cout << "parent called with arg " << arg << endl;

    if (thread_create((thread_startfunc_t) bloop, (void*) "child thread")) {
      cout << "thread_create failed\n";
      exit(1);
    }

    thread_lock(lock);
    if(thread_lock(lock)) {
      cout << "dafuq bro" << endl;
    }
    cout << "I'm hip I'm with it" << endl;
    thread_wait(lock,cv1);
    cout << "back to you son" << endl;
    thread_unlock(lock);


}

int main() {
    if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
      cout << "thread_libinit failed\n";
      exit(1);
    }
}
