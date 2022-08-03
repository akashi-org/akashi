import unittest
from akashi_core.pysl.compiler.items import CompileError
from akashi_core.pysl.compiler.compiler import compile_shaders
from akashi_core import gl, ak
from . import compiler_fixtures
import typing as tp

global_speed = 1890


@gl.lib('any')
def module_global_add(a: int, b: int) -> int:
    return a + b


@gl.lib('poly')
def module_global_poly(a: int, b: int) -> int:
    return a + b


class TestLambdaExpr(unittest.TestCase):

    def test_easy(self):

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(12)

        expected = 'void frag_main(inout vec4 color){12;}'

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected)

    def test_basic_resolution(self):

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(gl.sin(12))

        expected = 'void frag_main(inout vec4 color){sin(12);}'

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected)

    def test_local_resolution(self):

        speed = 999

        def gen1() -> ak.LEntryFragFn:
            return lambda e, b, c: e(gl.outer(speed) * gl.sin(12))

        expected = 'void frag_main(inout vec4 color){(999) * (sin(12));}'

        self.assertEqual(compile_shaders((gen1(),), ak.frag), expected)

    def test_global_resolution(self):

        speed = 999

        def gen1() -> ak.LEntryFragFn:
            return lambda e, b, c: e(gl.outer(global_speed) + (gl.outer(speed) * gl.sin(12)))

        expected = 'void frag_main(inout vec4 color){(1890) + ((999) * (sin(12)));}'

        self.assertEqual(compile_shaders((gen1(),), ak.frag), expected)

    def test_argument_resolution(self):

        speed = 999

        def gen1() -> ak.LEntryFragFn:
            return (
                lambda e, b, c: e((c.y * gl.outer(global_speed)) + (b.time * gl.outer(speed) * gl.sin(12)))
            )

        expected1 = 'void frag_main(inout vec4 color){((color.y) * (1890)) + (((time) * (999)) * (sin(12)));}'

        self.assertEqual(compile_shaders((gen1(),), ak.frag), expected1)

        def gen2() -> ak.LEntryPolyFn:
            return lambda e, b, p: (
                e((p.y * gl.outer(global_speed)) + (b.time * gl.outer(speed) * gl.sin(12)))
            )

        expected2 = 'void poly_main(inout vec4 pos){((pos.y) * (1890)) + (((time) * (999)) * (sin(12)));}'

        self.assertEqual(compile_shaders((gen2(),), ak.poly), expected2)

    def test_import_resolution(self):

        speed = 999

        def gen1() -> ak.LEntryFragFn:
            return lambda e, b, c: (
                e(compiler_fixtures.boost_add(1, gl.outer(global_speed)) + (gl.outer(speed) * gl.sin(12)))
            )

        expected1 = ''.join([
            'int compiler_fixtures_boost(int v);',
            'int compiler_fixtures_boost_add(int a, int b);',
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (compiler_fixtures_boost(b));}',
            'void frag_main(inout vec4 color){(compiler_fixtures_boost_add(1, 1890)) + ((999) * (sin(12)));}'
        ])

        self.assertEqual(compile_shaders((gen1(),), ak.frag), expected1)

        def gen2() -> ak.LEntryFragFn:
            return lambda e, b, c: e(module_global_poly(1, 2) + (gl.outer(speed) * gl.sin(12)))

        with self.assertRaisesRegex(CompileError, 'Forbidden import PolygonShader from FragShader') as _:
            compile_shaders((gen2(),), ak.frag)

    def test_import2(self):

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(module_global_add(1, 2))

        expected1 = ''.join([
            'int test_lambda_compiler_module_global_add(int a, int b);',
            'int test_lambda_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'void frag_main(inout vec4 color){test_lambda_compiler_module_global_add(1, 2);}'
        ])

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected1)

    def test_merge(self):

        speed = 999

        def gen1() -> ak.LEntryFragFn:
            return lambda e, b, c: e(gl.outer(global_speed) + (gl.outer(speed) * gl.sin(12))) | e(102)

        expected = 'void frag_main(inout vec4 color){(1890) + ((999) * (sin(12)));102;}'

        self.assertEqual(compile_shaders((gen1(),), ak.frag), expected)


class TestLambdaAssign(unittest.TestCase):

    def test_eq(self):

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(c.x) << e(gl.sin(12))

        expected = 'void frag_main(inout vec4 color){color.x = sin(12);}'

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected)

    def test_merge(self):

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(c.x) << e(c.x + b.resolution.x + gl.sin(12)) | e(12)

        expected = 'void frag_main(inout vec4 color){color.x = ((color.x) + (resolution.x)) + (sin(12));12;}'

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected)


class TestLambdaLet(unittest.TestCase):

    def test_easy(self):

        speed = 999

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(x := 102).tp(int)  # noqa: F841

        expected = 'void frag_main(inout vec4 color){int x = 102;}'

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected)

        def gen2() -> ak.LEntryFragFn:
            return lambda e, b, c: e((x := gl.outer(speed))).tp(int)  # noqa: F841

        expected2 = 'void frag_main(inout vec4 color){int x = 999;}'

        self.assertEqual(compile_shaders((gen2(),), ak.frag), expected2)

    def test_merge(self):

        speed = 999

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(c.x) << e(gl.outer(speed)) | e(y := 12.1).tp(float) | e(y)

        expected = 'void frag_main(inout vec4 color){color.x = 999;float y = 12.1;y;}'

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected)


class TestLambdaMultiple(unittest.TestCase):

    def test_easy(self):

        speed = 999

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, c: e(x := 102).tp(int)  # noqa: F841

        def gen2() -> ak.LEntryFragFn:
            return lambda e, b, c: e(c.x) << e(gl.outer(speed)) | e(y := gl.vec2(1, 2)).tp(gl.vec2) | e(y)

        def gen3() -> ak.LEntryFragFn:
            return lambda e, b, c: e(c.y) << e(1.0)

        expected = ''.join([
            'void frag_main_2(inout vec4 color){color.y = 1.0;}',
            'void frag_main_1(inout vec4 color){color.x = 999;vec2 y = vec2(1, 2);y;frag_main_2(color);}',
            'void frag_main(inout vec4 color){int x = 102;frag_main_1(color);}'
        ])

        self.assertEqual(compile_shaders((gen(), gen2(), gen3()), ak.frag), expected)

    def test_imports(self):

        speed = 999

        def gen1() -> ak.LEntryFragFn:
            return lambda e, b, c: (
                e(compiler_fixtures.boost_add(1, gl.outer(global_speed)) + (gl.outer(speed) * gl.sin(12)))
            )

        def gen2() -> ak.LEntryFragFn:
            return lambda e, b, c: (
                e(z := gl.outer(speed) * gl.sin(b.time)).tp(float) |
                e(compiler_fixtures.boost_add(1, gl.outer(global_speed)) + z)
            )

        expected = ''.join([
            'int compiler_fixtures_boost(int v);',
            'int compiler_fixtures_boost_add(int a, int b);',
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (compiler_fixtures_boost(b));}',
            'void frag_main_1(inout vec4 color){float z = (999) * (sin(time));(compiler_fixtures_boost_add(1, 1890)) + (z);}',
            'void frag_main(inout vec4 color){(compiler_fixtures_boost_add(1, 1890)) + ((999) * (sin(12)));frag_main_1(color);}'
        ])

        self.maxDiff = None
        self.assertEqual(compile_shaders((gen1(), gen2()), ak.frag), expected)


class TestLambdaWithBrace(unittest.TestCase):

    def test_easy(self):

        speed = 999

        def gen() -> ak.LEntryFragFn:
            return lambda e, b, color: ((
                (e(color.x) << e(gl.outer(speed))) |
                (e(y := 12.1).tp(float) | e(y))
            ))

        expected = 'void frag_main(inout vec4 color){color.x = 999;float y = 12.1;y;}'

        self.assertEqual(compile_shaders((gen(),), ak.frag), expected)


class TestLambdaOther(unittest.TestCase):

    def test_nested_lambdas(self):

        def gen() -> tp.Callable[[int], ak.LEntryFragFn]:
            return lambda h: lambda e, b, c: e(12)

        expected = 'void frag_main(inout vec4 color){12;}'

        self.assertEqual(compile_shaders((gen()(99),), ak.frag), expected)

    def test_dist_unary_sub(self):

        def gen() -> tp.Callable[[int], ak.LEntryFragFn]:
            return lambda h: lambda e, b, c: e(x := 12).tp(int) | e(y := 12).tp(int) | e(-(x + y))

        expected = 'void frag_main(inout vec4 color){int x = 12;int y = 12;-((x) + (y));}'

        self.assertEqual(compile_shaders((gen()(99),), ak.frag), expected)
