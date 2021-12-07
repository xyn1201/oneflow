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
#include "oneflow/core/framework/op_expr_grad_function.h"
#include "oneflow/core/framework/op_interp_ctx_generated.h"
#include "oneflow/core/functional/functional.h"

namespace oneflow {
namespace one {

struct TrilCaptureState : public AutoGradCaptureState {
  bool requires_grad = false;
  int64_t diagonal = 0;
};

class Tril : public OpExprGradFunction<TrilCaptureState> {
 public:
  Maybe<void> Capture(TrilCaptureState* state, const TensorTuple& inputs,
                      const TensorTuple& outputs, const OpInterpCtx* ctx) const override;
  Maybe<void> Apply(const TrilCaptureState* state, const TensorTuple& out_grads,
                    TensorTuple* in_grads) const override;
};

Maybe<void> Tril::Capture(TrilCaptureState* state, const TensorTuple& inputs,
                          const TensorTuple& outputs, const OpInterpCtx* ctx) const {
  state->requires_grad = inputs.at(0)->requires_grad();
  if (!state->requires_grad) { return Maybe<void>::Ok(); }
  auto* interp_ctx = dynamic_cast<const TrilOpInterpCtx*>(ctx);
  state->diagonal = interp_ctx->diagonal();
  return Maybe<void>::Ok();
}

Maybe<void> Tril::Apply(const TrilCaptureState* state, const TensorTuple& out_grads,
                        TensorTuple* in_grads) const {
  CHECK_EQ_OR_RETURN(out_grads.size(), 1);
  in_grads->resize(1);
  if (state->requires_grad) {
    in_grads->at(0) = JUST(functional::Tril(out_grads.at(0), state->diagonal));
  }
  return Maybe<void>::Ok();
}

REGISTER_OP_EXPR_GRAD_FUNCTION("tril", Tril);

}  // namespace one
}  // namespace oneflow
