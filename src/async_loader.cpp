// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "precompiled.h"
#include "async_loader.h"

namespace fpl {

AsyncLoader::AsyncLoader() {
  mutex_ = SDL_CreateMutex();
  assert(mutex_);
}

AsyncLoader::~AsyncLoader() {
  if (mutex_) SDL_DestroyMutex(mutex_);
}

void AsyncLoader::QueueJob(AsyncResource *res) {
  Lock([this,res]() {
    queue_.push_back(res);
  });
}

void AsyncLoader::LoaderWorker() {
  for (;;) {
    auto res = LockReturn<AsyncResource *>([this]() {
      return queue_.empty() ? nullptr : queue_[0];
    });
    if (!res) break;
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "async load: %s",
                 res->filename_.c_str());
    res->Load();
    Lock([this, res]() {
      queue_.erase(queue_.begin());
      done_.push_back(res);
    });
  }
}

int AsyncLoader::LoaderThread(void *user_data) {
  reinterpret_cast<AsyncLoader *>(user_data)->LoaderWorker();
  return 0;
}

void AsyncLoader::StartLoading() {
  auto thread = SDL_CreateThread(AsyncLoader::LoaderThread,
                                 "FPL Loader Thread", this);
  (void)thread;
  assert(thread);
}

bool AsyncLoader::TryFinalize() {
  for (;;) {
    auto res = LockReturn<AsyncResource *>([this]() {
      return done_.empty() ? nullptr : done_[0];
    });
    if (!res) break;
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "finalize: %s",
                 res->filename_.c_str());
    res->Finalize();
    Lock([this]() {
      done_.erase(done_.begin());
    });
  }
  return LockReturn<bool>([this]() {
    return queue_.empty();
  });
}


}  // namespace fpl

