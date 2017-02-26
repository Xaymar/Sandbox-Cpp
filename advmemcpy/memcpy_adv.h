#pragma once

void* memcpy_thread_initialize(size_t threads);
void memcpy_thread_env(void* env);
void* memcpy_thread(void* to, void* from, size_t size);
void memcpy_thread_finalize(void* env);
