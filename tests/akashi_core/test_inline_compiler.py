import unittest
from akashi_core.pysl import CompileError, compile_inline_shaders
from akashi_core import gl, ak
from . import compiler_fixtures
import typing as tp

global_speed = 1890


@gl.fn('any')
def module_global_add(a: int, b: int) -> int:
    return a + b


@gl.fn('poly')
def module_global_poly(a: int, b: int) -> int:
    return a + b


class TestInlineExpr(unittest.TestCase):

    def test_easy(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(12)

        expected = 'void frag_main(inout vec4 color){12;}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)

    def test_basic_resolution(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(gl.sin(12))

        expected = 'void frag_main(inout vec4 color){sin(12);}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)

    def test_local_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(gl.eval(speed) * gl.sin(12))

        expected = 'void frag_main(inout vec4 color){(999) * (sin(12));}'

        self.assertEqual(compile_inline_shaders((gen1(),), lambda: ak.FragShader()), expected)

    def test_global_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(gl.eval(global_speed) + (gl.eval(speed) * gl.sin(12)))

        expected = 'void frag_main(inout vec4 color){(1890) + ((999) * (sin(12)));}'

        self.assertEqual(compile_inline_shaders((gen1(),), lambda: ak.FragShader()), expected)

    def test_argument_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return (
                lambda b, c: gl.expr((c.value.y * gl.eval(global_speed)) + (b.time.value * gl.eval(speed) * gl.sin(12)))
            )

        expected1 = 'void frag_main(inout vec4 color){((color.y) * (1890)) + (((time) * (999)) * (sin(12)));}'

        self.assertEqual(compile_inline_shaders((gen1(),), lambda: ak.FragShader()), expected1)

        def gen2() -> ak.EntryPolyFn:
            return lambda b, p: (
                gl.expr((p.value.y * gl.eval(global_speed)) + (b.time.value * gl.eval(speed) * gl.sin(12)))
            )

        expected2 = 'void poly_main(inout vec3 pos){((pos.y) * (1890)) + (((time) * (999)) * (sin(12)));}'

        self.assertEqual(compile_inline_shaders((gen2(),), lambda: ak.PolygonShader()), expected2)

    def test_import_resolution(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: (
                gl.expr(compiler_fixtures.boost_add(1, gl.eval(global_speed)) + (gl.eval(speed) * gl.sin(12)))
            )

        expected1 = ''.join([
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (compiler_fixtures_boost(b));}',
            'void frag_main(inout vec4 color){(compiler_fixtures_boost_add(1, 1890)) + ((999) * (sin(12)));}'
        ])

        self.assertEqual(compile_inline_shaders((gen1(),), lambda: ak.FragShader()), expected1)

        def gen2() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(module_global_poly(1, 2) + (gl.eval(speed) * gl.sin(12)))

        with self.assertRaisesRegex(CompileError, 'Forbidden import PolygonShader from FragShader') as _:
            compile_inline_shaders((gen2(),), lambda: ak.FragShader())

    def test_import2(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.expr(module_global_add(1, 2))

        expected1 = ''.join([
            'int test_inline_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'void frag_main(inout vec4 color){test_inline_compiler_module_global_add(1, 2);}'
        ])

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected1)

    def test_merge(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return (
                lambda b, c: gl.expr(gl.eval(global_speed) + (gl.eval(speed) * gl.sin(12))) >> gl.expr(102)
            )

        expected = 'void frag_main(inout vec4 color){(1890) + ((999) * (sin(12)));102;}'

        self.assertEqual(compile_inline_shaders((gen1(),), lambda: ak.FragShader()), expected)


class TestInlineAssign(unittest.TestCase):

    def test_eq(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.assign(c.value.x).eq(gl.sin(12))

        expected = 'void frag_main(inout vec4 color){color.x = sin(12);}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)

    def test_op(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.assign(c.value.x).op('+=', b.resolution.value.x + gl.sin(12))

        expected = 'void frag_main(inout vec4 color){color.x += (resolution.x) + (sin(12));}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)

    def test_merge(self):

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.assign(c.value.x).op('+=', b.resolution.value.x + gl.sin(12)) >> gl.expr(12)

        expected = 'void frag_main(inout vec4 color){color.x += (resolution.x) + (sin(12));12;}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)


class TestInlineLet(unittest.TestCase):

    def test_easy(self):

        speed = 999

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.let(x := 102).tp(int)  # noqa: F841

        expected = 'void frag_main(inout vec4 color){int x = 102;}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)

        def gen2() -> ak.EntryFragFn:
            return lambda b, c: gl.let((x := gl.eval(speed))).tp(int)  # noqa: F841

        expected2 = 'void frag_main(inout vec4 color){int x = 999;}'

        self.assertEqual(compile_inline_shaders((gen2(),), lambda: ak.FragShader()), expected2)

    def test_merge(self):

        speed = 999

        def gen() -> ak.EntryFragFn:
            return lambda b, c: (
                gl.assign(c.value.x).eq(gl.eval(speed)) >>
                gl.let(y := 12.1).tp(float) >> gl.expr(y)
            )

        expected = 'void frag_main(inout vec4 color){color.x = 999;float y = 12.1;y;}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)


class TestInlineMultiple(unittest.TestCase):

    def test_easy(self):

        speed = 999

        def gen() -> ak.EntryFragFn:
            return lambda b, c: gl.let(x := 102).tp(int)  # noqa: F841

        def gen2() -> ak.EntryFragFn:
            return lambda b, c: (
                gl.assign(c.value.x).eq(gl.eval(speed)) >> gl.let(y := gl.vec2(1, 2)).tp(gl.vec2) >> gl.expr(y)
            )

        def gen3() -> ak.EntryFragFn:
            return lambda b, c: (
                gl.assign(c.value.y).eq(1.0)
            )

        expected = ''.join([
            'void frag_main_2(inout vec4 color){color.y = 1.0;}',
            'void frag_main_1(inout vec4 color){color.x = 999;vec2 y = vec2(1, 2);y;frag_main_2(color);}',
            'void frag_main(inout vec4 color){int x = 102;frag_main_1(color);}'
        ])

        self.assertEqual(compile_inline_shaders((gen(), gen2(), gen3()), lambda: ak.FragShader()), expected)

    def test_imports(self):

        speed = 999

        def gen1() -> ak.EntryFragFn:
            return lambda b, c: (
                gl.expr(compiler_fixtures.boost_add(1, gl.eval(global_speed)) + (gl.eval(speed) * gl.sin(12)))
            )

        def gen2() -> ak.EntryFragFn:
            return lambda b, c: (
                gl.let(z := gl.eval(speed) * gl.sin(b.time.value)).tp(float) >>
                gl.expr(compiler_fixtures.boost_add(1, gl.eval(global_speed)) + z)
            )

        expected = ''.join([
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (compiler_fixtures_boost(b));}',
            'void frag_main_1(inout vec4 color){float z = (999) * (sin(time));(compiler_fixtures_boost_add(1, 1890)) + (z);}',
            'void frag_main(inout vec4 color){(compiler_fixtures_boost_add(1, 1890)) + ((999) * (sin(12)));frag_main_1(color);}'
        ])

        self.maxDiff = None
        self.assertEqual(compile_inline_shaders((gen1(), gen2()), lambda: ak.FragShader()), expected)


class TestInlineWithBrace(unittest.TestCase):

    def test_easy(self):

        speed = 999

        def gen() -> ak.EntryFragFn:
            return lambda b, color: ((
                (gl.assign(color.value.x).eq(gl.eval(speed))) >>
                (gl.let(y := 12.1).tp(float) >> gl.expr(y))
            ))

        expected = 'void frag_main(inout vec4 color){color.x = 999;float y = 12.1;y;}'

        self.assertEqual(compile_inline_shaders((gen(),), lambda: ak.FragShader()), expected)


# class TestInlineOther(unittest.TestCase):
#
#     def test_nested_lambdas2(self):
#
#         def gen() -> tp.Callable[[int], ak.EntryFragFn]:
#             return lambda h: lambda b, c: gl.expr(12)
#
#         expected = 'void frag_main(inout vec4 color){12;}'
#
#         self.assertEqual(compile_inline_shader((gen()(99),), lambda: ak.FragShader()), expected)
