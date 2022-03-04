"""
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
"""
import oneflow
from oneflow.framework.docstr.utils import add_docstr

add_docstr(
    oneflow.decode_onerec,
    r"""
    (Tensor input, String key, DataType dtype, Shape shape, Bool is_dynamic=False, Shape reshape=None, Shape batch_padding=None)
    decode_onerec(input:Tensor, key:str, dtype, shape, is_dynamic=False, reshape=None, batch_padding=None) -> Tensor

    Decode a tensor from input which should be generated before by oneflow.nn.OneRecReader.

    Args:
        input: (Tensor): The tensor generated by oneflow.nn.OneRecReader before.
        key(str): The field name of the tensor to be decode
        shape(bool):  The shape of the tensor to be decode
        is_dynamic(bool): The tensor shape is dynamic or not
        reshape(tuple): Set it if you want to reshape the tensor
        batch_padding(tuple): Set it if batch padding is needed


    For example:

    .. code-block:: python

        import oneflow as flow
        files = ['file01.onerec', 'file02.onerec']
        # read onerec dataset form files
        reader = flow.nn.OneRecReader(files, 10, True, "batch")
        readdata = reader()

        # decode
        labels = flow.decode_onerec(readdata, key="labels", dtype=flow.int32, shape=(1,))
        dense_fields = flow.decode_onerec(readdata, key="dense_fields", dtype=flow.float, shape=(13,))

    """,
)
