import unittest
from akashi_core.pysl import CompileError, compile_inline_shader
from akashi_core import gl, ak
from . import compiler_fixtures


class TestInlineExpr(unittest.TestCase):

    def test_easy(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(12)

        expected = '12;'

        self.assertEqual(compile_inline_shader(gen(), 'FragShader'), expected)

    def test_basic_resolution(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(gl.sin(12))

        expected = 'sin(12);'

        self.assertEqual(compile_inline_shader(gen(), 'FragShader'), expected)

    def test_local_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(speed * gl.sin(12))

        expected = '(999) * (sin(12));'

        self.assertEqual(compile_inline_shader(gen1(), 'FragShader'), expected)
