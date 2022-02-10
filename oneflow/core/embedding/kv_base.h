/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef ONEFLOW_EMBEDDING_KV_BASE_H_
#define ONEFLOW_EMBEDDING_KV_BASE_H_

#include "oneflow/core/common/util.h"
#include "oneflow/core/ep/include/stream.h"

namespace oneflow {

namespace embedding {

class KVBaseIterator {
 public:
  OF_DISALLOW_COPY_AND_MOVE(KVBaseIterator);
  KVBaseIterator() = default;
  virtual ~KVBaseIterator() = default;

  virtual void NextN(ep::Stream* stream, uint32_t n_request, uint32_t* n_result, void* keys,
                     void* values) = 0;
  virtual void Reset() = 0;
};

class KVBase {
 public:
  OF_DISALLOW_COPY_AND_MOVE(KVBase);
  KVBase() = default;
  virtual ~KVBase() = default;

  virtual void WithIterator(const std::function<void(KVBaseIterator*)>& fn) = 0;
};

}  // namespace embedding

}  // namespace oneflow

#endif  // ONEFLOW_EMBEDDING_KV_BASE_H_
