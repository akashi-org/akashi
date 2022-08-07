from akashi_core.pysl._gl_vec import vec4, dvec4, dvec2, dvec3
from akashi_core import gl, ak
import typing as tp
from typing_extensions import assert_type

from .utils import TJ, TJ_TRUE, TJ_FALSE

# [TODO] impl later


def test_vectors(self):

    assert_type(TJ(dvec3(1.0).xy).eq(dvec2), TJ_TRUE)
