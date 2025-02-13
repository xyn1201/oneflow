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
#ifndef ONEFLOW_CORE_EP_COMMON_PRIMITIVE_UTIL_H_
#define ONEFLOW_CORE_EP_COMMON_PRIMITIVE_UTIL_H_

#include "oneflow/core/common/data_type.pb.h"
#include "oneflow/core/common/util.h"

namespace oneflow {

namespace ep {
namespace primitive {

inline size_t GetElementCount(size_t num_dims, const int64_t* dims) {
  size_t count = 1;
  for (size_t i = 0; i < num_dims; ++i) { count *= dims[i]; }
  return count;
}

template<typename T>
bool IsPackSizeSupported(const size_t pack_size, size_t num_dims, const int64_t* dims,
                         const void* ptr) {
  return (dims[num_dims - 1] % pack_size == 0)
         && (reinterpret_cast<std::uintptr_t>(ptr) % (pack_size * sizeof(T)) == 0);
}

inline void SimplifyBroadcastDims(size_t num_a_dims, const int64_t* a_dims, size_t num_b_dims,
                                  const int64_t* b_dims, size_t num_c_dims, const int64_t* c_dims,
                                  size_t* simplified_num_dims, int64_t* simplified_broadcast_dims,
                                  int64_t* simplified_a_dims, int64_t* simplified_b_dims,
                                  int64_t* simplified_c_dims) {
  const size_t num_max_dims = std::max(num_a_dims, num_b_dims);
  auto MakeGetDim = [num_max_dims](size_t num_dims, const int64_t* dims) {
    const int64_t num_padding_dims = num_max_dims - num_dims;
    return [num_padding_dims, dims](size_t index) {
      return index < num_padding_dims ? 1 : dims[index - num_padding_dims];
    };
  };
  auto GetADim = MakeGetDim(num_a_dims, a_dims);
  auto GetBDim = MakeGetDim(num_b_dims, b_dims);
  auto GetCDim = MakeGetDim(num_c_dims, c_dims);
  *simplified_num_dims = 0;
  bool prev_broadcast_a = false;
  bool prev_broadcast_b = false;
  bool prev_broadcast_c = false;
  for (int64_t i = 0; i < num_max_dims; ++i) {
    const int64_t a_dim = GetADim(i);
    const int64_t b_dim = GetBDim(i);
    const int64_t c_dim = GetCDim(i);
    const int64_t broadcast_dim = std::max(std::max(a_dim, b_dim), c_dim);
    CHECK_GT(broadcast_dim, 0);
    const bool broadcast_a = (a_dim == 1);
    const bool broadcast_b = (b_dim == 1);
    const bool broadcast_c = (c_dim == 1);
    CHECK((a_dim == broadcast_dim) || broadcast_a);
    CHECK((b_dim == broadcast_dim) || broadcast_b);
    CHECK((c_dim == broadcast_dim) || broadcast_c);
    if (broadcast_dim == 1) {
      continue;
    } else if (*simplified_num_dims != 0
               && (prev_broadcast_a == broadcast_a && prev_broadcast_b == broadcast_b
                   && prev_broadcast_c == broadcast_c)) {
      simplified_a_dims[*simplified_num_dims - 1] *= a_dim;
      simplified_b_dims[*simplified_num_dims - 1] *= b_dim;
      simplified_c_dims[*simplified_num_dims - 1] *= c_dim;
      simplified_broadcast_dims[*simplified_num_dims - 1] *= broadcast_dim;
    } else {
      simplified_a_dims[*simplified_num_dims] = a_dim;
      simplified_b_dims[*simplified_num_dims] = b_dim;
      simplified_c_dims[*simplified_num_dims] = c_dim;
      simplified_broadcast_dims[*simplified_num_dims] = broadcast_dim;
      *simplified_num_dims += 1;
      prev_broadcast_a = broadcast_a;
      prev_broadcast_b = broadcast_b;
      prev_broadcast_c = broadcast_c;
    }
  }
}

template<size_t max_num_dims>
inline void SimplifyBroadcastDims(size_t num_src0_dims, const int64_t* src0_dims,
                                  size_t num_src1_dims, const int64_t* src1_dims,
                                  size_t* simplified_num_dims, int64_t* simplified_src0_dims,
                                  int64_t* simplified_src1_dims, int64_t* simplified_dst_dims) {
  size_t src0_count = GetElementCount(num_src0_dims, src0_dims);
  size_t src1_count = GetElementCount(num_src1_dims, src1_dims);
  if (src0_count == 1 || src1_count == 1) {
    *simplified_num_dims = 1;
    simplified_src0_dims[0] = src0_count;
    simplified_src1_dims[0] = src1_count;
    simplified_dst_dims[0] = std::max(src0_count, src1_count);
    return;
  }
  int64_t dst_dims[max_num_dims];
  int64_t broadcast_dims[max_num_dims];
  const size_t num_dst_dims = std::max(num_src0_dims, num_src1_dims);
  for (int64_t i = 0; i < num_dst_dims; ++i) {
    const int64_t num_src0_padding_dims = num_dst_dims - num_src0_dims;
    const int64_t num_src1_padding_dims = num_dst_dims - num_src1_dims;
    size_t src0_dim = i < num_src0_padding_dims ? 1 : src0_dims[i - num_src0_padding_dims];
    size_t src1_dim = i < num_src1_padding_dims ? 1 : src1_dims[i - num_src1_padding_dims];
    dst_dims[i] = std::max(src0_dim, src1_dim);
  }
  SimplifyBroadcastDims(num_src0_dims, src0_dims, num_src1_dims, src1_dims, num_dst_dims, dst_dims,
                        simplified_num_dims, broadcast_dims, simplified_src0_dims,
                        simplified_src1_dims, simplified_dst_dims);
  for (int64_t i = 0; i < *simplified_num_dims; ++i) {
    CHECK_EQ(broadcast_dims[i], simplified_dst_dims[i]);
  }
}

template<typename T, typename D>
std::unique_ptr<T> NewPrimitiveFromHandlers(
    const std::map<D, std::function<std::unique_ptr<T>()>>& handlers, const D& key) {
  const auto iter = handlers.find(key);
  if (iter != handlers.end()) { return iter->second(); }
  return nullptr;
}

}  // namespace primitive
}  // namespace ep

}  // namespace oneflow

#endif  // ONEFLOW_CORE_EP_COMMON_PRIMITIVE_UTIL_H_
