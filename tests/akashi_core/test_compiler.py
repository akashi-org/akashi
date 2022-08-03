import unittest
from akashi_core.pysl.compiler.items import CompileError, CompilerConfig
from akashi_core.pysl.compiler.compiler import compile_shaders
from akashi_core import gl, ak
from . import compiler_fixtures
import typing as tp
from typing import overload

TEST_CONFIG: CompilerConfig.Config = {
    'pretty_compile': False,
    'mangle_mode': 'soft'
}


@gl.lib('any')
def module_global_add(a: int, b: int) -> int:
    return a + b


class TestMixed(unittest.TestCase):

    def test_basic(self):

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(12)

        @gl.entry(ak.frag)
        def vec_attr(buffer: ak.frag, cl: gl.inout_p[gl.vec4]) -> None:
            cl.x = buffer.time * 12

        def named_gen(arg_value: int):
            @gl.entry(ak.frag)
            def vec_attr(buffer: ak.frag, cl: gl.inout_p[gl.vec4]) -> None:
                cl.x = buffer.time * 12 + gl.outer(arg_value)
            return vec_attr

        expected = ''.join([
            'void frag_main_3(inout vec4 color){color.x = 900;}',
            'void frag_main_2(inout vec4 color){color.x = ((time) * (12)) + (180);frag_main_3(color);}',
            'void frag_main_1(inout vec4 color){color.x = (time) * (12);frag_main_2(color);}',
            'void frag_main(inout vec4 color){12;frag_main_1(color);}'
        ])

        self.maxDiff = None
        self.assertEqual(compile_shaders((
            gen(),
            vec_attr,
            named_gen(180),
            lambda e, b, c: e(c.x) << e(900)
        ), ak.frag, TEST_CONFIG), expected)

    def test_import(self):

        outer_value = 102

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(module_global_add(1, 2) * gl.outer(outer_value))

        @gl.entry(ak.frag)
        def vec_attr(buffer: ak.frag, cl: gl.inout_p[gl.vec4]) -> None:
            cl.x = module_global_add(1, 2)

        expected = ''.join([
            'int test_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'void frag_main_1(inout vec4 color){color.x = test_compiler_module_global_add(1, 2);}',
            'void frag_main(inout vec4 color){(test_compiler_module_global_add(1, 2)) * (102);frag_main_1(color);}'
        ])

        self.maxDiff = None
        self.assertEqual(compile_shaders((
            gen(),
            vec_attr
        ), ak.frag, TEST_CONFIG), expected)


class TestOther(unittest.TestCase):

    def test_buffer_direct_assign(self):

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(c) << e(c)

        expected = ''.join([
            'void frag_main(inout vec4 color){color = color;}'
        ])

        self.maxDiff = None
        self.assertEqual(compile_shaders((
            gen(),
        ), ak.frag, TEST_CONFIG), expected)

    def test_complex_imports(self):

        @gl.lib('any')
        def common_func() -> float:
            return 41

        @gl.lib('any')
        def another_func() -> float:
            return common_func() * 12

        @gl.entry(ak.frag)
        def entry(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            r: float = common_func()
            color.x = another_func()

        expected = ''.join([
            'float test_compiler_common_func(){return 41;}',
            'float test_compiler_another_func(){return (test_compiler_common_func()) * (12);}',
            'void frag_main(inout vec4 color){float r = test_compiler_common_func();color.x = test_compiler_another_func();}'
        ])

        self.maxDiff = None
        self.assertEqual(compile_shaders((entry,), ak.frag, TEST_CONFIG), expected)
