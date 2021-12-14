import unittest
from akashi_core.pysl import CompileError, compile_inline_shader
from akashi_core import gl, ak
from . import compiler_fixtures

global_speed = 1890


class PolyMod(ak.PolygonShader):

    @gl.func
    def boost(a: int) -> int:
        return a * 1000


class TestInlineExpr(unittest.TestCase):

    def test_easy(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(12)

        expected = 'void frag_main(inout vec4 _fragColor){12;}'

        self.assertEqual(compile_inline_shader(gen(), 'FragShader'), expected)

    def test_basic_resolution(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(gl.sin(12))

        expected = 'void frag_main(inout vec4 _fragColor){sin(12);}'

        self.assertEqual(compile_inline_shader(gen(), 'FragShader'), expected)

    def test_local_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(speed * gl.sin(12))

        expected = 'void frag_main(inout vec4 _fragColor){(999) * (sin(12));}'

        self.assertEqual(compile_inline_shader(gen1(), 'FragShader'), expected)

    def test_global_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(global_speed + (speed * gl.sin(12)))

        expected = 'void frag_main(inout vec4 _fragColor){(1890) + ((999) * (sin(12)));}'

        self.assertEqual(compile_inline_shader(gen1(), 'FragShader'), expected)

    def test_argument_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr((c.value.y * global_speed) + (b.time.value * speed * gl.sin(12)))

        expected1 = 'void frag_main(inout vec4 _fragColor){((_fragColor.y) * (1890)) + (((time) * (999)) * (sin(12)));}'

        self.assertEqual(compile_inline_shader(gen1(), 'FragShader'), expected1)

        def gen2() -> ak.EntryPolyFn:
            return lambda b, p: gl.expr((p.value.y * global_speed) + (b.time.value * speed * gl.sin(12)))

        expected2 = 'void poly_main(inout vec3 pos){((pos.y) * (1890)) + (((time) * (999)) * (sin(12)));}'

        self.assertEqual(compile_inline_shader(gen2(), 'PolygonShader'), expected2)

    def test_import_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(compiler_fixtures.OuterMod.boost_sub(1, global_speed) + (speed * gl.sin(12)))

        expected1 = ''.join([
            'int OuterMod_sub(int a, int b){return (a) - (b);}',
            'int OuterMod_boost_sub(int a, int b){return OuterMod_sub(a, (b) * (10));}',
            'void frag_main(inout vec4 _fragColor){(OuterMod_boost_sub(1, 1890)) + ((999) * (sin(12)));}'
        ])

        self.assertEqual(compile_inline_shader(gen1(), 'FragShader'), expected1)

        def gen2() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(PolyMod.boost(1) + (speed * gl.sin(12)))

        with self.assertRaisesRegex(CompileError, 'Forbidden import PolygonShader from FragShader') as _:
            compile_inline_shader(gen2(), 'FragShader')

    def test_merge(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(global_speed + (speed * gl.sin(12))) >> gl.expr(102)

        expected = 'void frag_main(inout vec4 _fragColor){(1890) + ((999) * (sin(12)));102;}'

        self.assertEqual(compile_inline_shader(gen1(), 'FragShader'), expected)


class TestInlineAssign(unittest.TestCase):

    def test_eq(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.assign(c.value.x).eq(gl.sin(12))

        expected = 'void frag_main(inout vec4 _fragColor){_fragColor.x = sin(12);}'

        self.assertEqual(compile_inline_shader(gen(), 'FragShader'), expected)

    def test_op(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.assign(c.value.x).op('+=', b.resolution.value.x + gl.sin(12))

        expected = 'void frag_main(inout vec4 _fragColor){_fragColor.x += (resolution.x) + (sin(12));}'

        self.assertEqual(compile_inline_shader(gen(), 'FragShader'), expected)

    def test_merge(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.assign(c.value.x).op('+=', b.resolution.value.x + gl.sin(12)) >> gl.expr(12)

        expected = 'void frag_main(inout vec4 _fragColor){_fragColor.x += (resolution.x) + (sin(12));12;}'

        self.assertEqual(compile_inline_shader(gen(), 'FragShader'), expected)