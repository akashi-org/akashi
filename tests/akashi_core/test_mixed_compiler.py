import unittest
from akashi_core.pysl import compile_mixed_shaders, CompileError, CompilerConfig
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


class TestBasic(unittest.TestCase):

    def test_basic(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(12)

        @gl.entry_frag()
        def vec_attr(buffer: ak.FragShader, cl: gl.inout_p[gl.vec4]) -> None:
            cl.value.x = buffer.time.value * 12

        expected = ''.join([
            'void frag_main_2(inout vec4 color){color.x = 900;}',
            'void frag_main_1(inout vec4 color){color.x = (time) * (12);frag_main_2(color);}',
            'void frag_main(inout vec4 color){12;frag_main_1(color);}'
        ])

        self.assertEqual(compile_mixed_shaders((
            gen(),
            vec_attr,
            lambda b, c: gl.assign(c.value.x).eq(900)
        ), lambda: ak.FragShader(), TEST_CONFIG), expected)

    def test_import(self):

        outer_value = 102

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(module_global_add(1, 2) * gl.eval(outer_value))

        @gl.entry_frag()
        def vec_attr(buffer: ak.FragShader, cl: gl.inout_p[gl.vec4]) -> None:
            cl.value.x = module_global_add(1, 2)

        expected = ''.join([
            'int test_mixed_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'void frag_main_1(inout vec4 color){color.x = test_mixed_compiler_module_global_add(1, 2);}',
            'void frag_main(inout vec4 color){(test_mixed_compiler_module_global_add(1, 2)) * (102);frag_main_1(color);}'
        ])

        self.maxDiff = None
        self.assertEqual(compile_mixed_shaders((
            gen(),
            vec_attr
        ), lambda: ak.FragShader(), TEST_CONFIG), expected)
