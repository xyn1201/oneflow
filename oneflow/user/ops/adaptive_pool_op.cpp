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
#include "oneflow/user/ops/nn_util.h"
#include "oneflow/core/framework/op_generated.h"

namespace oneflow {

namespace {

Maybe<void> InferFWTensorDesc(user_op::InferContext* ctx) {
  std::vector<int64_t> output_size = ctx->Attr<std::vector<int64_t>>("output_size");
  const Shape& x_shape = ctx->InputShape("x", 0);
  auto* y_shape = ctx->OutputShape("y", 0);
  y_shape->resize(x_shape.NumAxes());
  *y_shape = {x_shape[0], x_shape[1]};
  for (int i = 2; i < y_shape->size(); ++i) {
    (*y_shape)[i] = output_size.size() > i - 2 ? output_size[i - 2] : output_size[0];
  }

  return Maybe<void>::Ok();
}

Maybe<void> InferBWTensorDesc(user_op::InferContext* ctx) {
  *ctx->OutputShape("dx", 0) = ctx->InputShape("x", 0);
  *ctx->OutputIsDynamic("dx", 0) = ctx->InputIsDynamic("x", 0);
  return Maybe<void>::Ok();
}

Maybe<void> FwGetSbpFn(user_op::SbpContext* ctx) {
  const user_op::TensorDesc& tensor = ctx->LogicalTensorDesc4InputArgNameAndIndex("x", 0);
  // only for nchw
  FOR_RANGE(int64_t, i, 0, std::min(2, (int)tensor.shape().NumAxes())) {
    ctx->NewBuilder().Split(user_op::OpArg("x", 0), i).Split(user_op::OpArg("y", 0), i).Build();
  }
  return Maybe<void>::Ok();
}

Maybe<void> BwGetSbpFn(user_op::SbpContext* ctx) {
  const user_op::TensorDesc& tensor = ctx->LogicalTensorDesc4InputArgNameAndIndex("x", 0);
  FOR_RANGE(int64_t, i, 0, std::min(2, (int)tensor.shape().NumAxes())) {
    ctx->NewBuilder()
        .Split(user_op::OpArg("x", 0), i)
        .Split(user_op::OpArg("dy", 0), i)
        .Split(user_op::OpArg("dx", 0), i)
        .Build();
  }
  return Maybe<void>::Ok();
}

Maybe<void> InferFWDataType(user_op::InferContext* ctx) {
  *ctx->OutputDType("y", 0) = ctx->InputDType("x", 0);
  return Maybe<void>::Ok();
}

Maybe<void> InferBWDataType(user_op::InferContext* ctx) {
  *ctx->OutputDType("dx", 0) = ctx->InputDType("x", 0);
  return Maybe<void>::Ok();
}

}  // namespace

#define DEF_ADAPTIVE_AVG_POOL_OP(op_class_name_prefix)                                           \
  /* static */ Maybe<void> op_class_name_prefix##Op::InferLogicalTensorDesc(                     \
      user_op::InferContext* ctx) {                                                              \
    return InferFWTensorDesc(ctx);                                                               \
  }                                                                                              \
                                                                                                 \
  /*static*/ Maybe<void> op_class_name_prefix##Op::InferPhysicalTensorDesc(                      \
      user_op::InferContext* ctx) {                                                              \
    return InferLogicalTensorDesc(ctx);                                                          \
  }                                                                                              \
                                                                                                 \
  /* static */ Maybe<void> op_class_name_prefix##Op::GetSbp(user_op::SbpContext* ctx) {          \
    return FwGetSbpFn(ctx);                                                                      \
  }                                                                                              \
                                                                                                 \
  /* static */ Maybe<void> op_class_name_prefix##Op::InferDataType(user_op::InferContext* ctx) { \
    return InferFWDataType(ctx);                                                                 \
  }                                                                                              \
                                                                                                 \
  /* static */ Maybe<void> op_class_name_prefix##GradOp::InferLogicalTensorDesc(                 \
      user_op::InferContext* ctx) {                                                              \
    return InferBWTensorDesc(ctx);                                                               \
  }                                                                                              \
                                                                                                 \
  /*static*/ Maybe<void> op_class_name_prefix##GradOp::InferPhysicalTensorDesc(                  \
      user_op::InferContext* ctx) {                                                              \
    return InferLogicalTensorDesc(ctx);                                                          \
  }                                                                                              \
                                                                                                 \
  /* static */ Maybe<void> op_class_name_prefix##GradOp::GetSbp(user_op::SbpContext* ctx) {      \
    return BwGetSbpFn(ctx);                                                                      \
  }                                                                                              \
                                                                                                 \
  /* static */ Maybe<void> op_class_name_prefix##GradOp::InferDataType(                          \
      user_op::InferContext* ctx) {                                                              \
    return InferBWDataType(ctx);                                                                 \
  }

DEF_ADAPTIVE_AVG_POOL_OP(AdaptiveAvgPool1D)
DEF_ADAPTIVE_AVG_POOL_OP(AdaptiveAvgPool2D)
DEF_ADAPTIVE_AVG_POOL_OP(AdaptiveAvgPool3D)

#undef DEF_ADAPTIVE_AVG_POOL_OP

REGISTER_USER_OP_GRAD("adaptive_avg_pool1d")
    .SetBackwardOpConfGenFn([](user_op::BackwardOpConfContext* ctx) -> Maybe<void> {
      const auto adaptive_avg_pool1d_grad_op_name = ctx->FwOp().op_name() + "_grad";
      ctx->DefineOp(adaptive_avg_pool1d_grad_op_name, [&ctx](user_op::BackwardOpBuilder& builder) {
        return builder.OpTypeName("adaptive_avg_pool1d_grad")
            .InputBind("x", ctx->FwOp().input("x", 0))
            .InputBind("dy", ctx->FwOp().output_grad("y", 0))
            .Output("dx")
            .Build();
      });
      ctx->FwOp().InputGradBind(
          user_op::OpArg("x", 0),
          [&ctx, &adaptive_avg_pool1d_grad_op_name]() -> const std::string& {
            return ctx->GetOp(adaptive_avg_pool1d_grad_op_name).output("dx", 0);
          });
      return Maybe<void>::Ok();
    });

REGISTER_USER_OP_GRAD("adaptive_avg_pool2d")
    .SetBackwardOpConfGenFn([](user_op::BackwardOpConfContext* ctx) -> Maybe<void> {
      const auto adaptive_avg_pool2d_grad_op_name = ctx->FwOp().op_name() + "_grad";
      ctx->DefineOp(adaptive_avg_pool2d_grad_op_name, [&ctx](user_op::BackwardOpBuilder& builder) {
        return builder.OpTypeName("adaptive_avg_pool2d_grad")
            .InputBind("x", ctx->FwOp().input("x", 0))
            .InputBind("dy", ctx->FwOp().output_grad("y", 0))
            .Output("dx")
            .Build();
      });
      ctx->FwOp().InputGradBind(
          user_op::OpArg("x", 0),
          [&ctx, &adaptive_avg_pool2d_grad_op_name]() -> const std::string& {
            return ctx->GetOp(adaptive_avg_pool2d_grad_op_name).output("dx", 0);
          });
      return Maybe<void>::Ok();
    });

REGISTER_USER_OP_GRAD("adaptive_avg_pool3d")
    .SetBackwardOpConfGenFn([](user_op::BackwardOpConfContext* ctx) -> Maybe<void> {
      const auto adaptive_avg_pool3d_grad_op_name = ctx->FwOp().op_name() + "_grad";
      ctx->DefineOp(adaptive_avg_pool3d_grad_op_name, [&ctx](user_op::BackwardOpBuilder& builder) {
        return builder.OpTypeName("adaptive_avg_pool3d_grad")
            .InputBind("x", ctx->FwOp().input("x", 0))
            .InputBind("dy", ctx->FwOp().output_grad("y", 0))
            .Output("dx")
            .Build();
      });
      ctx->FwOp().InputGradBind(
          user_op::OpArg("x", 0),
          [&ctx, &adaptive_avg_pool3d_grad_op_name]() -> const std::string& {
            return ctx->GetOp(adaptive_avg_pool3d_grad_op_name).output("dx", 0);
          });
      return Maybe<void>::Ok();
    });

}  // namespace oneflow
