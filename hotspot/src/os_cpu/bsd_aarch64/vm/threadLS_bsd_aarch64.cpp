/*
*
*/

#include "precompiled.hpp"
#include "runtime/threadLocalStorage.hpp"
#include "runtime/thread.inline.hpp"

void ThreadLocalStorage::generate_code_for_get_thread() {
    // nothing we can do here for user-level thread
}

void ThreadLocalStorage::pd_init() {
}

__thread Thread *aarch64_currentThread;

void ThreadLocalStorage::pd_set_thread(Thread* thread) {
  os::thread_local_storage_at_put(ThreadLocalStorage::thread_index(), thread);
  aarch64_currentThread = thread;
}
