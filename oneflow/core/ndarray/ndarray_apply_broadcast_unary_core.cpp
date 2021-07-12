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
#include "oneflow/core/ndarray/ndarray_apply_broadcast_unary_core.h"

namespace oneflow {

template<typename T, int NDIMS, template<typename> class unary_func>
struct NdarrayApplyBroadcastUnaryCPUCore final {
  inline static void Apply(const XpuVarNdarray<T>& y, const XpuVarNdarray<const T>& x) {
    y.template Assign<NDIMS>(x.Broadcast(y.shape()).template UnaryFunc<unary_func>());
  }
};

template<typename T, int NDIMS, template<typename> class unary_func>
struct NdarrayApplyBroadcastUnaryCoreWrapper<DeviceType::kCPU, T, NDIMS, unary_func> final {
  static void Apply(DeviceCtx* ctx, const XpuVarNdarray<T>& y, const XpuVarNdarray<const T>& x) {
    NdarrayApplyBroadcastUnaryCPUCore<T, NDIMS, unary_func>::Apply(y, x);
  }
};

#define INSTANTIATE_BROADCAST_UNARY_FUNC_CPU(dtype_pair, NDIMS, unary_func) \
  template struct NdarrayApplyBroadcastUnaryCoreWrapper<                \
      DeviceType::kCPU, OF_PP_PAIR_FIRST(dtype_pair), NDIMS, unary_func>;
OF_PP_SEQ_PRODUCT_FOR_EACH_TUPLE(INSTANTIATE_BROADCAST_UNARY_FUNC_CPU,
                                 ARITHMETIC_DATA_TYPE_SEQ FLOAT16_DATA_TYPE_SEQ, DIM_SEQ,
                                 ARITHMETIC_UNARY_FUNC_SEQ)

#if defined(WITH_ROCM)

namespace {

template<typename T, int NDIMS, template<typename> class unary_func>
struct NdarrayApplyBroadcastUnaryGPUCore final {
  OF_DEVICE_FUNC static void Apply(const XpuVarNdarray<T>& y, const XpuVarNdarray<const T>& x) {
    y.template Assign<NDIMS>(x.Broadcast(y.shape()).template UnaryFunc<unary_func>());
  }
};

template<typename T, int NDIMS, template<typename> class unary_func>
__global__ void GpuBroadcastUnaryFunc(const XpuVarNdarray<T> y, const XpuVarNdarray<const T> x) {
  NdarrayApplyBroadcastUnaryGPUCore<T, NDIMS, unary_func>::Apply(y, x);
}

}  // namespace

template<typename T, int NDIMS, template<typename> class unary_func>
struct NdarrayApplyBroadcastUnaryCoreWrapper<DeviceType::kGPU, T, NDIMS, unary_func> final {
  static void Apply(DeviceCtx* ctx, const XpuVarNdarray<T>& y, const XpuVarNdarray<const T>& x) {
    size_t n = y.host_shape().HostElemNum();
    RUN_ROCM_KERNEL((GpuBroadcastUnaryFunc<T, NDIMS, unary_func>), ctx, n, 0, y, x);
  }
};

#define INSTANTIATE_BROADCAST_UNARY_FUNC_GPU(dtype_pair, NDIMS, unary_func) \
  template struct NdarrayApplyBroadcastUnaryCoreWrapper<                \
      DeviceType::kGPU, OF_PP_PAIR_FIRST(dtype_pair), NDIMS, unary_func>;
OF_PP_SEQ_PRODUCT_FOR_EACH_TUPLE(INSTANTIATE_BROADCAST_UNARY_FUNC_GPU,
                                 ARITHMETIC_DATA_TYPE_SEQ HALF_DATA_TYPE_SEQ, DIM_SEQ,
                                 ARITHMETIC_UNARY_FUNC_SEQ)

#endif

}  // namespace oneflow
