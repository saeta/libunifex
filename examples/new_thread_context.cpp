/*
 * Copyright 2020-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unifex/new_thread_context.hpp>

#include <unifex/scheduler_concepts.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/transform.hpp>
#include <unifex/when_all.hpp>

#include <atomic>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <thread>

struct trace_construction_destruction {
    static std::atomic<int> instanceCount; 

    trace_construction_destruction() {
        ++instanceCount;
        std::stringstream s;
        s << "thread_local at address " << (void*)this
          << " constructing on thread " << std::this_thread::get_id();
        std::puts(s.str().c_str());
    }
    ~trace_construction_destruction() {
        --instanceCount;
        std::stringstream s;
        s << "thread_local at address " << (void*)this
          << " destructing on thread " << std::this_thread::get_id();
        std::puts(s.str().c_str());
    }
};

std::atomic<int> trace_construction_destruction::instanceCount = 0;

int main() {
  {
    unifex::new_thread_context ctx;

    auto makeThreadTask = [&](int i) {
        return unifex::transform(
            unifex::schedule(ctx.get_scheduler()),
            [i] {
                std::stringstream s;
                s << "Task " << i << " running on thread " << std::this_thread::get_id();
                std::puts(s.str().c_str());

                thread_local trace_construction_destruction t;
            });
    };

    unifex::sync_wait(
        unifex::when_all(
            makeThreadTask(1),
            makeThreadTask(2),
            makeThreadTask(3),
            makeThreadTask(4)));

    std::puts("shutting down new_thread_context");
  }

  std::puts("new_thread_contxt finished shutting down");

  // new_thread_context destructor should have waited for all threads to finish
  // destroying thread-locals.

  assert(trace_construction_destruction::instanceCount.load() == 0);
}