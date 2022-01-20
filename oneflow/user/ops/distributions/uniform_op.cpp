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
#include "oneflow/core/framework/framework.h"
#include "oneflow/core/framework/op_generated.h"

namespace oneflow {

/* static */ Maybe<void> UniformOp::InferLogicalTensorDesc(user_op::InferContext* ctx) {
  Shape* out_shape = ctx->OutputShape("out", 0);
  const Shape& shape = ctx->Attr<Shape>("shape");
  DimVector dim_vec;
  if (shape.NumAxes() > 0) {
    dim_vec.insert(dim_vec.end(), shape.dim_vec().cbegin(), shape.dim_vec().cend());
  }
  *out_shape = Shape(dim_vec);
  return Maybe<void>::Ok();
}

/*static*/ Maybe<void> UniformOp::InferPhysicalTensorDesc(user_op::InferContext* ctx) {
  return InferLogicalTensorDesc(ctx);
}

/* static */ Maybe<void> UniformOp::GetSbp(user_op::SbpContext* ctx) {
  ctx->NewBuilder().Broadcast(ctx->inputs()).Broadcast(ctx->outputs()).Build();
  return Maybe<void>::Ok();
}

/* static */ Maybe<void> UniformOp::InferNdSbp(user_op::InferNdSbpFnContext* ctx) {
  cfg::SbpParallel default_sbp;
  default_sbp.mutable_broadcast_parallel();
  return user_op::InferNdSbp4SrcOp(ctx, default_sbp);
}

/* static */ Maybe<void> UniformOp::InferDataType(user_op::InferContext* ctx) {
  auto dtype = ctx->Attr<DataType>("dtype");
  *ctx->OutputDType("out", 0) = dtype;
  return Maybe<void>::Ok();
}

}  // namespace oneflow
