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

#ifndef FPL_ASYNC_LOADER_H
#define FPL_ASYNC_LOADER_H

namespace fpl {

class AsyncLoader;

// Any resources that can be loaded async should inherit from this.
class AsyncResource {
 public:
  AsyncResource(const std::string &filename)
    : filename_(filename), data_(nullptr) {}
  virtual ~AsyncResource() {}

  // Load should perform the actual loading of filename_, and store the
  // result in data_, or nullptr upon failure. It is called on the loader
  // thread, so should not access any program state outside of this object.
  // Since there will be only one loader thread, any libraries called by Load
  // need not be MT-safe as long as they're not also called by the main thread.
  virtual void Load() = 0;

  // This should implement the behavior of turning data_ into the actual
  // desired resource. Called on the main thread only.
  virtual void Finalize() = 0;

  const std::string &filename() const { return filename_; }

 protected:
  std::string filename_;
  uint8_t *data_;

  friend class AsyncLoader;
};

class AsyncLoader {
 public:
  AsyncLoader();
  ~AsyncLoader();

  // Call this any number of times before StartLoading.
  void QueueJob(AsyncResource *res);

  // Launches the loading thread.
  void StartLoading();

  // Cleans-up the background loading thread once all jobs have been completed.
  // You can restart with StartLoading() if you like.
  void StopLoadingWhenComplete();

  // Call this once per frame after StartLoading. Will call Finalize on any
  // resources that have finished loading. One it returns true, that means
  // the queue is empty, all resources have been processed, and the loading
  // thread has terminated.
  bool TryFinalize();

 private:
  void Lock(const std::function<void ()> &body) {
    auto err = SDL_LockMutex(mutex_);
    (void)err;
    assert(err == 0);
    body();
    SDL_UnlockMutex(mutex_);
  }
  template<typename T> T LockReturn(const std::function<T ()> &body) {
    T ret;
    Lock([&ret, &body]() { ret = body(); });
    return ret;
  }

  void LoaderWorker();
  static int LoaderThread(void *user_data);

  std::vector<AsyncResource *> queue_, done_;

  // This lock protects ALL state in this class, i.e. the two vectors.
  SDL_mutex *mutex_;

  // Kick-off the worker thread when a new job arrives.
  SDL_semaphore *job_semaphore_;
};

}  // namespace fpl

#endif  // FPL_ASYNC_LOADER_H
