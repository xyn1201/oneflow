#ifndef ONEFLOW_CORE_THREAD_CPU_THREAD_H_
#define ONEFLOW_CORE_THREAD_CPU_THREAD_H_

#include "oneflow/core/device/cpu_stream.h"
#include "oneflow/core/thread/thread.h"

namespace oneflow {

class CpuThread final : public Thread {
 public:
  OF_DISALLOW_COPY_AND_MOVE(CpuThread);
  CpuThread(int64_t thrd_loc_id);
  ~CpuThread();

 private:
  std::unique_ptr<std::thread> cpu_device_;
  std::unique_ptr<CpuStream> cpu_stream_;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_THREAD_CPU_THREAD_H_
