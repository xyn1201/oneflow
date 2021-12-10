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

#ifndef ONEFLOW_API_CPP_GRAPH_H_
#define ONEFLOW_API_CPP_GRAPH_H_

#include <memory>
#include <string>
#include <vector>
#include "oneflow/api/cpp/framework/device.h"
#include "oneflow/api/cpp/framework/shape.h"
#include "oneflow/api/cpp/framework/tensor.h"
#include "oneflow/core/common/hash_container.h"
#include "oneflow/core/framework/tensor.h"
#include "oneflow/core/framework/tensor_tuple.h"
#include "oneflow/core/job/job.pb.h"
#include "oneflow/core/operator/op_conf.pb.h"

namespace oneflow {

class NNGraph;

}  // namespace oneflow

namespace oneflow_api {

class Graph final {
 public:
  explicit Graph(const std::string& model_path, const Device& device);
  explicit Graph(const std::string& model_path);
  std::vector<Tensor> Forward(const std::vector<Tensor>& inputs);
  void set_batch_size(int batch_size);

  // not must, better if provided
  // void To(const Device& device);

 private:
  oneflow::Maybe<void> Compile(const std::vector<Tensor>& inputs);
  oneflow::Maybe<std::vector<Tensor>> Run(const std::vector<Tensor>& inputs) const;
  oneflow::Maybe<void> AddOp(oneflow::OperatorConf op_conf);
  oneflow::Maybe<void> BuildGraph(const std::vector<Tensor>& inputs);
  oneflow::Maybe<void> LoadCheckpoint();
  oneflow::Maybe<void> RegisterTensors();

  std::shared_ptr<oneflow::NNGraph> graph_ = nullptr;
  bool is_compiled_ = false;
  int batch_size_ = 0;
  Device device_;
  oneflow::Job job_;

  const std::string model_path_;

  oneflow::HashMap<std::string, std::shared_ptr<oneflow::one::Tensor>> input_name_to_tensor_;
  oneflow::HashMap<std::string, std::shared_ptr<oneflow::one::Tensor>> output_name_to_tensor_;
  oneflow::HashMap<std::string, std::shared_ptr<oneflow::one::Tensor>> variable_op_name_to_tensor_;
  std::shared_ptr<oneflow::one::TensorTuple> output_tensor_tuple_;
  std::shared_ptr<oneflow::one::TensorTuple> parameter_tensor_tuple_;
};

Graph Load(const std::string& model_path, const Device& device);

Graph Load(const std::string& model_path);

// TODO(zzk0): only for debug, remove this
inline void PrintTensor(const Tensor& tensor) {
  std::cout << tensor.shape().elem_cnt() << " ";
  for (int i = 0; i < tensor.shape().NumAxes(); ++i) { std::cout << tensor.shape().At(i) << " "; }
  std::cout << std::endl;
  float* data = new float[tensor.shape().elem_cnt() * 4];
  tensor.copy_to(data);
  for (int i = 0; i < tensor.shape().elem_cnt(); ++i) { std::cout << data[i] << " "; }
  std::cout << std::endl;
  delete[] data;
}

}  // namespace oneflow_api

#endif  // ONEFLOW_API_CPP_GRAPH_H_
