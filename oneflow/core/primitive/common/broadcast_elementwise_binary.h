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
#ifndef ONEFLOW_CORE_PRIMITIVE_COMMON_BROADCAST_ELEMENTWISE_BINARY
#define ONEFLOW_CORE_PRIMITIVE_COMMON_BROADCAST_ELEMENTWISE_BINARY

#include "oneflow/core/primitive/include/primitive.h"
#include "oneflow/core/common/nd_index_offset_helper.h"

namespace oneflow {

namespace primitive {

namespace {

template<typename T, int N>
struct GetPackType {
  using type = typename std::aligned_storage<N * sizeof(T), N * sizeof(T)>::type;
};

template<typename T, int N>
using PackType = typename GetPackType<T, N>::type;

template<typename T, int N>
union Pack {
  static_assert(sizeof(PackType<T, N>) == sizeof(T) * N, "");
  OF_DEVICE_FUNC Pack() {
    // do nothing
  }
  PackType<T, N> storage;
  T elem[N];
};

template<size_t num_dims, typename IndexType>
struct BroadcastElementwiseBinaryParams {
  NdIndexOffsetHelper<IndexType, num_dims> src0_index_helper;
  NdIndexOffsetHelper<IndexType, num_dims> src1_index_helper;
  NdIndexOffsetHelper<IndexType, num_dims> dst_index_helper;
  IndexType src0_dims[num_dims];
  IndexType src1_dims[num_dims];
  IndexType count{};
  const void* src0{};
  const void* src1{};
  void* dst{};
};

template<BinaryOp op, typename T, typename R, size_t num_dims, size_t pack_size, typename IndexType>
void LaunchKernel(StreamContext* stream_ctx,
                  BroadcastElementwiseBinaryParams<num_dims, IndexType> params);

template<BinaryOp op, typename T, typename R, size_t num_dims, size_t pack_size, typename IndexType>
void LaunchKernel(StreamContext* stream_ctx, const int64_t* src0_dims, const void* src0,
                  const int64_t* src1_dims, const void* src1, const int64_t* dst_dims, void* dst,
                  size_t count) {
  BroadcastElementwiseBinaryParams<num_dims, IndexType> params;
  params.src0_index_helper = NdIndexOffsetHelper<IndexType, num_dims>(src0_dims);
  params.src1_index_helper = NdIndexOffsetHelper<IndexType, num_dims>(src1_dims);
  params.dst_index_helper = NdIndexOffsetHelper<IndexType, num_dims>(dst_dims);
  for (size_t i = 0; i < num_dims; ++i) {
    params.src0_dims[i] = src0_dims[i];
    params.src1_dims[i] = src1_dims[i];
  }
  params.src0 = src0;
  params.src1 = src1;
  params.dst = dst;
  params.count = static_cast<IndexType>(count);
  LaunchKernel<op, T, R, num_dims, pack_size, IndexType>(stream_ctx, params);
}

template<BinaryOp op, typename T, typename R, size_t num_dims, size_t pack_size>
void DispatchIndexType(StreamContext* stream_ctx, const int64_t* src0_dims, const void* src0,
                       const int64_t* src1_dims, const void* src1, const int64_t* dst_dims,
                       void* dst) {
  size_t count = 1;
  for (size_t i = 0; i < num_dims; ++i) { count *= dst_dims[i]; }
  if (count < GetMaxVal<int32_t>()) {
    LaunchKernel<op, T, R, num_dims, pack_size, int32_t>(stream_ctx, src0_dims, src0, src1_dims,
                                                         src1, dst_dims, dst, count);
  } else {
    LaunchKernel<op, T, R, num_dims, pack_size, int64_t>(stream_ctx, src0_dims, src0, src1_dims,
                                                         src1, dst_dims, dst, count);
  }
}

template<BinaryOp op, typename T, typename R, size_t num_dims>
void DispatchPackSize(StreamContext* stream_ctx, size_t pack_size, const int64_t* src0_dims,
                      const void* src0, const int64_t* src1_dims, const void* src1,
                      const int64_t* dst_dims, void* dst) {
  void (*func)(StreamContext* /*stream_ctx*/, const int64_t* /*src0_dims*/, const void* /*src0*/,
               const int64_t* /*src1_dims*/, const void* /*src1*/, const int64_t* /*dst_dims*/,
               void* /*dst*/) = nullptr;
  if (pack_size == 1) {
    func = DispatchIndexType<op, T, R, num_dims, 1>;
  } else if (pack_size == 2) {
    func = DispatchIndexType<op, T, R, num_dims, 2>;
  } else if (pack_size == 4) {
    func = DispatchIndexType<op, T, R, num_dims, 4>;
  } else {
    UNIMPLEMENTED();
  }
  func(stream_ctx, src0_dims, src0, src1_dims, src1, dst_dims, dst);
}

template<BinaryOp op, typename T, typename R>
void LaunchWithSimplified(StreamContext* stream_ctx, size_t pack_size, size_t num_dims,
                          const int64_t* src0_dims, const void* src0, const int64_t* src1_dims,
                          const void* src1, const int64_t* dst_dims, void* dst) {
  void (*func)(StreamContext* /*stream_ctx*/, size_t /*pack_size*/, const int64_t* /*src0_dims*/,
               const void* /*src0*/, const int64_t* /*src1_dims*/, const void* /*src1*/,
               const int64_t* /*dst_dims*/, void* /*dst*/) = nullptr;
  if (num_dims == 1) {
    func = DispatchPackSize<op, T, R, 1>;
  } else if (num_dims == 2) {
    func = DispatchPackSize<op, T, R, 2>;
  } else if (num_dims == 3) {
    func = DispatchPackSize<op, T, R, 3>;
  } else if (num_dims == 4) {
    func = DispatchPackSize<op, T, R, 4>;
  } else if (num_dims == 5) {
    func = DispatchPackSize<op, T, R, 5>;
  } else if (num_dims == 6) {
    func = DispatchPackSize<op, T, R, 6>;
  } else if (num_dims == 7) {
    func = DispatchPackSize<op, T, R, 7>;
  } else if (num_dims == 8) {
    func = DispatchPackSize<op, T, R, 8>;
  } else {
    UNIMPLEMENTED();
  }
  func(stream_ctx, pack_size, src0_dims, src0, src1_dims, src1, dst_dims, dst);
}

constexpr size_t kMaxPackSize = 4;
constexpr size_t kMaxNumDims = 8;

void SimplifyDims(size_t num_src0_dims, const int64_t* src0_dims, size_t num_src1_dims,
                  const int64_t* src1_dims, size_t* simplified_num_dims,
                  int64_t* simplified_src0_dims, int64_t* simplified_src1_dims) {
  const size_t num_max_dims = std::max(num_src0_dims, num_src1_dims);
  auto MakeGetDim = [num_max_dims](size_t num_dims, const int64_t* dims) {
    const int64_t num_padding_dims = num_max_dims - num_dims;
    return [num_padding_dims, dims](size_t index) {
      return index < num_padding_dims ? 1 : dims[index - num_padding_dims];
    };
  };
  *simplified_num_dims = 0;
  bool prev_broadcast_src0 = false;
  bool prev_broadcast_src1 = false;
  auto GetSrc0Dim = MakeGetDim(num_src0_dims, src0_dims);
  auto GetSrc1Dim = MakeGetDim(num_src1_dims, src1_dims);
  for (int64_t i = 0; i < num_max_dims; ++i) {
    const int64_t src0_dim = GetSrc0Dim(i);
    const int64_t src1_dim = GetSrc1Dim(i);
    const int64_t broadcast_dim = std::max(src0_dim, src1_dim);
    CHECK_GT(broadcast_dim, 0);
    const bool broadcast_src0 = (src0_dim == 1);
    const bool broadcast_src1 = (src1_dim == 1);
    CHECK((src0_dim == broadcast_dim) || broadcast_src0);
    CHECK((src1_dim == broadcast_dim) || broadcast_src1);
    if (broadcast_dim == 1) {
      continue;
    } else if (*simplified_num_dims != 0
               && (prev_broadcast_src0 == broadcast_src0
                   && prev_broadcast_src1 == broadcast_src1)) {
      simplified_src0_dims[*simplified_num_dims - 1] *= src0_dim;
      simplified_src1_dims[*simplified_num_dims - 1] *= src1_dim;
    } else {
      simplified_src0_dims[*simplified_num_dims] = src0_dim;
      simplified_src1_dims[*simplified_num_dims] = src1_dim;
      *simplified_num_dims += 1;
      prev_broadcast_src0 = broadcast_src0;
      prev_broadcast_src1 = broadcast_src1;
    }
  }
}

template<size_t max_pack_size, typename T, typename R>
size_t GetPackSize(size_t num_src_dims, const int64_t* src0_dims, const void* src0,
                   const int64_t* src1_dims, const void* src1, void* dst) {
  static_assert(max_pack_size > 0 && (max_pack_size & (max_pack_size - 1)) == 0, "");
  auto src0_ptr = reinterpret_cast<std::uintptr_t>(src0);
  auto src1_ptr = reinterpret_cast<std::uintptr_t>(src1);
  auto dst_ptr = reinterpret_cast<std::uintptr_t>(dst);
  for (size_t pack_size = max_pack_size; pack_size > 1; pack_size /= 2) {
    if ((src0_dims[num_src_dims - 1] % pack_size == 0)
        && (src1_dims[num_src_dims - 1] % pack_size == 0)
        && (src0_ptr % (pack_size * sizeof(T)) == 0) && (src1_ptr % (pack_size * sizeof(T)) == 0)
        && (dst_ptr % (pack_size * sizeof(R))) == 0) {
      return pack_size;
    }
  }
  return 1;
}

template<BinaryOp op, typename T, typename R>
void SimplifyThenLaunch(StreamContext* stream_ctx, size_t num_src0_dims, const int64_t* src0_dims,
                        const void* src0, size_t num_src1_dims, const int64_t* src1_dims,
                        const void* src1, void* dst) {
  CHECK_LE(num_src0_dims, kMaxNumDims);
  CHECK_LE(num_src1_dims, kMaxNumDims);
  size_t simplified_num_dims = 0;
  int64_t simplified_src0_dims[kMaxNumDims];
  int64_t simplified_src1_dims[kMaxNumDims];
  SimplifyDims(num_src0_dims, src0_dims, num_src1_dims, src1_dims, &simplified_num_dims,
               simplified_src0_dims, simplified_src1_dims);
  size_t pack_size = GetPackSize<kMaxPackSize, T, R>(simplified_num_dims, simplified_src0_dims,
                                                     src0, simplified_src1_dims, src1, dst);
  int64_t simplified_dst_dims[kMaxNumDims];
  for (int64_t i = 0; i < simplified_num_dims; ++i) {
    simplified_dst_dims[i] = std::max(simplified_src0_dims[i], simplified_src1_dims[i]);
  }
  simplified_src0_dims[simplified_num_dims - 1] /= pack_size;
  simplified_src1_dims[simplified_num_dims - 1] /= pack_size;
  simplified_dst_dims[simplified_num_dims - 1] /= pack_size;
  LaunchWithSimplified<op, T, R>(stream_ctx, pack_size, simplified_num_dims, simplified_src0_dims,
                                 src0, simplified_src1_dims, src1, simplified_dst_dims, dst);
}

}  // namespace

#define BINARY_MATH_OP_SEQ                  \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kAdd)      \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kSub)      \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kMul)      \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kDiv)      \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kMax)      \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kMin)      \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kFloorDiv) \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kPow)
   // OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kFmod)

#define BINARY_LOGICAL_OP_SEQ                   \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kEqual)        \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kNotEqual)     \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kLessThan)     \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kLessEqual)    \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kGreaterThan)  \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kGreaterEqual) \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kLogicalAnd)   \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kLogicalOr)    \
  OF_PP_MAKE_TUPLE_SEQ(BinaryOp::kLogicalXor)

}  // namespace primitive

}  // namespace oneflow

#endif  // ONEFLOW_CORE_PRIMITIVE_COMMON_BROADCAST_ELEMENTWISE_BINARY
