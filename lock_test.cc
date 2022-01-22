#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#define LOCK 0xFFFFFFFF


using namespace std;

int g = 0; // global (shared by all threads)

void loop(void* a) {
    if (thread_lock(LOCK)) {
        cout << "lock is not acquired requester" << endl;
    }
    char* id = (char*) a;
    cout << "loop called with id " << id << endl;
    
    for (int i = 0; i < 5; i++, g++) {
      cout << id << ":\t" << i << "\t" << g << endl;
      if (thread_yield()) {
          cout << "thread_yield failed\n";
          exit(1);
      }
    }
    if (thread_unlock(LOCK)){
        cout << "it's not released" << endl;
    }
}

void parent(void* a) {
    //if (thread_lock(LOCK)) {
    //    cout << "lock is not acquired requester" << endl;
    //}
    int arg = (intptr_t) a;
    cout << "parent called with arg " << arg << endl;

    if (thread_create((thread_startfunc_t) loop, (void*) "child thread")) {
      cout << "thread_create failed\n";
      exit(1);
    }

    loop((void*) "parent thread");
    //if (thread_unlock(LOCK)){
    //    cout << "it's not released" << endl;
    //}
}

int main() {
    if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
      cout << "thread_libinit failed\n";
      exit(1);
    }
}

