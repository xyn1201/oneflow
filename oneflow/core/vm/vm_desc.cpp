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
#include "oneflow/core/vm/vm_desc.h"
#include "oneflow/core/vm/stream_desc.h"
#include "oneflow/core/vm/stream_type.h"
#include "oneflow/core/vm/instruction_type.h"
#include "oneflow/core/common/util.h"

namespace oneflow {
namespace vm {

namespace {

void SetMachineIdRange(Range* range, int64_t machine_num, int64_t this_machine_id) {
  *range = Range(this_machine_id, this_machine_id + 1);
}

intrusive::shared_ptr<VmDesc> MakeVmDesc(
    const Resource& resource, int64_t this_machine_id,
    const std::function<void(const std::function<void(const InstrTypeId&)>&)>& ForEachInstrTypeId) {
  std::set<const StreamType*> stream_types;
  ForEachInstrTypeId(
      [&](const InstrTypeId& instr_type_id) { stream_types.insert(&instr_type_id.stream_type()); });
  auto vm_desc =
      intrusive::make_shared<VmDesc>(intrusive::make_shared<VmResourceDesc>(resource).Get());
  SetMachineIdRange(vm_desc->mut_machine_id_range(), resource.machine_num(), this_machine_id);
  int cnt = 0;
  for (const auto* stream_type : stream_types) {
    auto stream_desc = stream_type->MakeStreamDesc(resource, this_machine_id);
    if (stream_desc) {
      ++cnt;
      CHECK(vm_desc->mut_stream_type2desc()->Insert(stream_desc.Mutable()).second);
    }
  }
  CHECK_EQ(vm_desc->stream_type2desc().size(), cnt);
  return vm_desc;
}

}  // namespace

intrusive::shared_ptr<VmDesc> MakeVmDesc(const Resource& resource, int64_t this_machine_id) {
  return MakeVmDesc(resource, this_machine_id, &ForEachInstrTypeId);
}

intrusive::shared_ptr<VmDesc> MakeVmDesc(const Resource& resource, int64_t this_machine_id,
                                         const std::set<std::string>& instr_type_names) {
  const auto& ForEachInstrTypeId = [&](const std::function<void(const InstrTypeId&)>& Handler) {
    for (const auto& instr_type_name : instr_type_names) {
      Handler(LookupInstrTypeId(instr_type_name));
      Handler(LookupInstrTypeId(std::string("Infer-") + instr_type_name));
    }
  };
  return MakeVmDesc(resource, this_machine_id, ForEachInstrTypeId);
}

}  // namespace vm
}  // namespace oneflow
