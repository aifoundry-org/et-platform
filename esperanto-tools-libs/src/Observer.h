/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "Utils.h"
#include <algorithm>
#include <mutex>
#include <vector>
namespace patterns {

template <typename T> class Subject;

template <typename T> class Observer {
public:
  virtual void update(T hint) = 0;
  virtual ~Observer() = default;
};

template <typename T> class Subject {
public:
  virtual ~Subject() = default;

  void attach(Observer<T>* o) {
    SpinLock lock(mutex_);
    if (std::find(observers_.begin(), observers_.end(), o) == observers_.end()) {
      observers_.emplace_back(o);
    }
  }
  void detach(Observer<T>* o) {
    SpinLock lock(mutex_);
    auto it = std::remove(begin(observers_), end(observers_), o);
    observers_.erase(it, end(observers_));
  }
  void notify(T hint) {
    SpinLock lock(mutex_);
    for (auto& o : observers_) {
      o->update(hint);
    }
  }

private:
  std::vector<Observer<T>*> observers_;
  std::mutex mutex_;
};
} // namespace patterns
