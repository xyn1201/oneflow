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

import unittest
from collections import OrderedDict

import numpy as np
from test_util import GenArgList

import oneflow as flow
import oneflow.unittest

from oneflow.test_utils.automated_test_util import *


@autotest(n=2, auto_backward=False, check_graph=False)
def test_greater_equal_impl(test_case, ndim, placement, sbp):
    dims = [random(1, 4) * 8 for i in range(ndim)]
    x1 = random_tensor(ndim, *dims)
    x1 = x1.to_consistent(placement=placement, sbp=sbp)
    x2 = random_tensor(ndim, *dims)
    x2 = x2.to_consistent(placement=placement, sbp=sbp)

    z = torch.ge(x1, x2)
    return z


class TestGreaterEqualConsistent(flow.unittest.TestCase):
    @consistent
    def test_greater_equal(test_case):
        # random ndim in range [1,4]
        ndim = random(1, 5).to(int).value()
        for placement in all_placement():
            for sbp in all_sbp(placement, max_dim=ndim):
                test_greater_equal_impl(test_case, ndim, placement, sbp)


if __name__ == "__main__":
    unittest.main()
