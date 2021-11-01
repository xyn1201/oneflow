// /*
// Copyright 2020 The OneFlow Authors. All rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// */
// #include "oneflow/core/framework/op_expr_grad_function.h"
// #include "oneflow/core/framework/op_builder.h"
// #include "oneflow/core/framework/op_interpreter/op_interpreter_util.h"
// #include "oneflow/core/framework/op_expr.h"
// #include "oneflow/core/framework/op_expr_helper.h"
// #include "oneflow/core/framework/framework.h"
// #include "oneflow/core/functional/functional.h"

// namespace oneflow {
// namespace one {

// struct RNNTlossInterpState : public AutoGradCaptureState {
//   bool requires_grad = false;
// };

// class RNNTloss : public OpExprGradFunction<RNNTlossInterpState> {
//  public:
//   Maybe<void> Init(const OpExpr& op) override;
//   Maybe<void> Capture(RNNTlossInterpState* ctx, const TensorTuple& inputs,
//                       const TensorTuple& outputs, const AttrMap& attrs) const override;
//   Maybe<void> Apply(const RNNTlossInterpState* ctx, const TensorTuple& out_grads,
//                     TensorTuple* in_grads) const override;

//  private:
//   AttrMap base_attrs_;
//   // std::shared_ptr<OpExpr> grad_op_;
// };

// Maybe<void> RNNTloss::Init(const OpExpr& op) {
//   const auto* fw_op_expr = dynamic_cast<const UserOpExpr*>(&op);
//   CHECK_NOTNULL_OR_RETURN(fw_op_expr);
//   // const std::string& op_name = fw_op_expr->op_name();
//   base_attrs_ = MakeAttrMapFromUserOpConf(fw_op_expr->proto());
//   // grad_op_ = JUST(one::OpBuilder("RNNTloss", GradientOpName(op_name))
//   //                     .Input("grads")
//   //                     .Input("dy")
//   //                     .Output("dx")
//   //                     .Build());
//   return Maybe<void>::Ok();
// }

// Maybe<void> RNNTloss::Capture(RNNTlossInterpState* ctx, const TensorTuple& inputs,
//                                 const TensorTuple& outputs, const AttrMap& attrs) const {
//   ComposedAttrMap composed_attrs(attrs, base_attrs_);
//   CHECK_EQ_OR_RETURN(inputs.size(), 4);
//   ctx->requires_grad = inputs.at(0)->requires_grad();

//   if (!ctx->requires_grad) return Maybe<void>::Ok();

//   ctx->SaveTensorForBackward(outputs.at(1));
//   return Maybe<void>::Ok();
// }

// Maybe<void> RNNTloss::Apply(const RNNTlossInterpState* ctx, const TensorTuple& out_grads,
//                               TensorTuple* in_grads) const {
//   if (!ctx->requires_grad) return Maybe<void>::Ok();
//   CHECK_EQ_OR_RETURN(out_grads.size(), 2);
//   const auto& dy = out_grads.at(0);
//   const auto& grads = ctx->SavedTensors().at(0);
//   in_grads->resize(1);
  
//   DimVector dim_vec = {-1,1,1,1};
//   std::shared_ptr<one::Tensor> dy_trans = JUST(functional::Reshape(dy,Shape(dim_vec)));
//   in_grads->at(0) = JUST(functional::Mul(grads,dy_trans));

//   return Maybe<void>::Ok();
// }

// REGISTER_OP_EXPR_GRAD_FUNCTION("RNNTloss", RNNTloss);

// }  // namespace one
// }  // namespace oneflow