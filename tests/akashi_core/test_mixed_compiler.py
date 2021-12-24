import unittest
from akashi_core.pysl import compile_named_shader, CompileError, CompilerConfig
from akashi_core import gl, ak
from . import compiler_fixtures
import typing as tp
from typing import overload

TEST_CONFIG: CompilerConfig.Config = {
    'pretty_compile': False,
    'mangle_mode': 'soft'
}


@gl.fn('any')
def module_global_add(a: int, b: int) -> int:
    return a + b


# class TestBasic(unittest.TestCase):
#
#     def test_basic(self):
#
#         @gl.fn('any')
#         def decl_func(a: int, b: int) -> int: ...
#
#         expected = 'int test_named_compiler_decl_func(int a, int b);'
#
#         self.assertEqual(compile_named_shader(decl_func, TEST_CONFIG), expected)
