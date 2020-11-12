include(ExternalProject)

set(LLVM_PROJECT_INSTALL ${THIRD_PARTY_DIR}/llvm-project)
set(LLVM_PROJECT_INCLUDE_DIR ${LLVM_PROJECT_INSTALL}/include)
set(LLVM_PROJECT_LIBRARY_DIR ${LLVM_PROJECT_INSTALL}/lib)

set(LLVM_PROJECT_TAR_URL https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/llvm-project-11.0.0.tar.xz)
use_mirror(VARIABLE LLVM_PROJECT_TAR_URL URL ${LLVM_PROJECT_TAR_URL})
set(LLVM_PROJECT_URL_HASH c6410be66b3d68d241a9c0f9b374d628)

set(LLVM_PROJECT_BUILD_LIBRARY_DIR ${LLVM_PROJECT_LIBRARY_DIR})
set(LLVM_PROJECT_LIBRARY_NAMES libLLVMCore.a)

foreach(LIBRARY_NAME ${LLVM_PROJECT_LIBRARY_NAMES})
  list(APPEND LLVM_PROJECT_STATIC_LIBRARIES ${LLVM_PROJECT_LIBRARY_DIR}/${LIBRARY_NAME})
endforeach()

if(THIRD_PARTY)

    include(ProcessorCount)
    ProcessorCount(PROC_NUM)
    ExternalProject_Add(llvm-project
        PREFIX llvm-project
        URL ${LLVM_PROJECT_TAR_URL}
        URL_HASH MD5=${LLVM_PROJECT_URL_HASH}
        UPDATE_COMMAND ""
        SOURCE_SUBDIR llvm
        CMAKE_ARGS
            -DLLVM_TARGETS_TO_BUILD:STRING=X86\;NVPTX
            -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
            -DLLVM_ENABLE_PROJECTS:STRING=mlir
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCMAKE_INSTALL_PREFIX:STRING=${LLVM_PROJECT_INSTALL}
            -DLLVM_BUILD_EXAMPLES:BOOL=OFF
            -DBUILD_SHARED_LIBS:BOOL=OFF
            -DLLVM_ENABLE_ASSERTIONS:BOOL=ON
            -DLLVM_INCLUDE_BENCHMARKS:BOOL=OFF
            -DLLVM_INCLUDE_EXAMPLES:BOOL=OFF
            -DLLVM_INCLUDE_TESTS:BOOL=OFF
            -DMLIR_INCLUDE_TESTS:BOOL=OFF
            -DMLIR_INCLUDE_INTEGRATION_TESTS:BOOL=OFF
            -DMLIR_CUDA_RUNNER_ENABLED:BOOL=OFF
            -DMLIR_INCLUDE_INTEGRATION_TESTS:BOOL=OFF
        BUILD_COMMAND make -j${PROC_NUM}
    )
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/llvm-project/src/llvm-project/llvm/cmake/modules")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/llvm-project/src/llvm-project/mlir/cmake/modules")
else()
    list(APPEND CMAKE_MODULE_PATH "${LLVM_PROJECT_LIBRARY_DIR}/cmake/llvm")
    list(APPEND CMAKE_MODULE_PATH "${LLVM_PROJECT_LIBRARY_DIR}/cmake/mlir")
endif(THIRD_PARTY)

include(AddLLVM)
include(TableGen)
