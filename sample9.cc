// A sample test program for the thread library.

#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#define lock 12
#define lock2 4
#define cv1 42
#define cv2 89

using namespace std;

void bloop(void* a) {
  thread_lock(lock);
  cout << "I'm taking this lock with me kid" << endl;
  thread_signal(lock,cv1);
  return;
}

void floop(void* a) {
  thread_lock(lock2);
  cout << "I can probably say this" << endl;
  thread_lock(lock);
  cout << "but not this one" << endl;
  thread_unlock(lock);
  return;
}

void woop(void* a) {
  thread_lock(25);
  if(thread_lock(lock2)) {
      cout << "I tried" << endl;
  }
  cout << "welp" << endl;
  thread_wait(25,cv1);
  thread_unlock(25);
}

void parent(void* a) {
    int arg = (intptr_t) a;
    cout << "parent called with arg " << arg << endl;

    if (thread_create((thread_startfunc_t) bloop, (void*) "child thread")) {
      cout << "thread_create failed\n";
      exit(1);
    }
    if (thread_create((thread_startfunc_t) floop, (void*) "child thread2")) {
      cout << "thread_create failed\n";
      exit(1);
    }
    if (thread_create((thread_startfunc_t) woop, (void*) "child thread3")) {
      cout << "thread_create failed\n";
      exit(1);
    }
    thread_lock(lock);
    cout << "I'm hip I'm with it" << endl;
    thread_wait(lock,cv1);
    cout << "back to you son" << endl;
    thread_signal(lock,cv1);
    thread_yield();
    thread_unlock(lock);
    return;
}

int main() {
    if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
      cout << "thread_libinit failed\n";
      exit(1);
    }
}
