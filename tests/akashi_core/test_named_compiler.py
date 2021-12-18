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


class TestBasic(unittest.TestCase):

    def test_func_decl(self):

        @gl.fn('any')
        def decl_func(a: int, b: int) -> int: ...

        expected = 'int test_named_compiler_decl_func(int a, int b);'

        self.assertEqual(compile_named_shader(decl_func, TEST_CONFIG), expected)

    def test_inline_func(self):

        @gl.fn('any')
        def add(a: int, b: int) -> int:
            return a + b

        expected = '\n'.join([
            'int test_named_compiler_add(int a, int b){return (a) + (b);}',
        ])

        self.assertEqual(compile_named_shader(add, TEST_CONFIG), expected)

    def test_undecorated_func(self):

        def add(a: int, b: int) -> int:
            return a + b

        with self.assertRaisesRegex(CompileError, 'Named shader function must be decorated properly') as _:
            compile_named_shader(add)

        @gl.fn('error kind')  # type: ignore
        def add2(a: int, b: int) -> int:
            return a + b

        with self.assertRaisesRegex(CompileError, 'Invalid shader kind') as _:
            compile_named_shader(add2)

    def test_import(self):

        @gl.fn('frag')
        def add(a: int, b: int) -> int:
            return a + b

        @gl.fn('frag')
        def assign_fn(a: int) -> int:
            c: int = add(a, 2)
            d: int = module_global_add(90, 2)
            d = c = 89
            c += 190 + d
            return c

        expected = ''.join([
            'int test_named_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'int test_named_compiler_add(int a, int b){return (a) + (b);}',
            'int test_named_compiler_assign_fn(int a){int c = add(a, 2);int d = module_global_add(90, 2);d = c = 89;c += (190) + (d);return c;}',
        ])

        self.assertEqual(compile_named_shader(assign_fn, TEST_CONFIG), expected)

        @gl.fn('any')
        def error_import(a: int) -> int:
            return add(a, 12)

        with self.assertRaisesRegex(CompileError, 'Forbidden import FragShader from AnyShader') as _:
            compile_named_shader(error_import, TEST_CONFIG)

    def test_module_import(self):

        @gl.fn('frag')
        def add(a: int, b: int) -> int:
            return compiler_fixtures.boost_add(a, b)

        expected = ''.join([
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (boost(b));}',
            'int test_named_compiler_add(int a, int b){return boost_add(a, b);}'
        ])

        self.assertEqual(compile_named_shader(add, TEST_CONFIG), expected)

    def test_paren_arith_func(self):

        @gl.fn('any')
        def paren_arith(a: int, b: int) -> int:
            return (100 - int(((12 * 90) + a) / b))

        expected = ''.join([
            'int test_named_compiler_paren_arith(int a, int b){return (100) - (int((((12) * (90)) + (a)) / (b)));}',
        ])

        self.assertEqual(compile_named_shader(paren_arith, TEST_CONFIG), expected)

    def test_vec_attr(self):

        @gl.fn('frag')
        def vec_attr(_fragColor: gl.vec4) -> None:
            _fragColor.x = 12

        expected = ''.join([
            'void test_named_compiler_vec_attr(vec4 _fragColor){_fragColor.x = 12;}',
        ])

        self.assertEqual(compile_named_shader(vec_attr, TEST_CONFIG), expected)

        vec4 = gl.vec4

        @gl.fn('frag')
        def vec_attr2(_fragColor: vec4) -> gl.vec4:
            _fragColor.x = 12
            return _fragColor

        expected2 = ''.join([
            'vec4 test_named_compiler_vec_attr2(vec4 _fragColor){_fragColor.x = 12;return _fragColor;}',
        ])

        self.assertEqual(compile_named_shader(vec_attr2, TEST_CONFIG), expected2)

    def test_params_qualifier(self):

        @gl.fn('any')
        def add(a: int, b: int) -> int:
            return a + b

        @gl.fn('frag')
        def vec_attr(_fragColor: gl.inout_p[gl.vec4]) -> None:
            _fragColor.value.x = add(12, int(_fragColor.value.x))

        expected = ''.join([
            'int test_named_compiler_add(int a, int b){return (a) + (b);}',
            'void test_named_compiler_vec_attr(inout vec4 _fragColor){_fragColor.x = add(12, int(_fragColor.x));}',
        ])

        self.assertEqual(compile_named_shader(vec_attr, TEST_CONFIG), expected)

        @gl.fn('frag')
        def vec_attr2(_fragColor: gl.out_p[gl.vec4]) -> gl.out_p[gl.vec4]:
            return _fragColor

        with self.assertRaisesRegex(CompileError, 'Return type must not have its parameter qualifier') as _:
            compile_named_shader(vec_attr2, TEST_CONFIG)


class TestControl(unittest.TestCase):

    def test_if(self):

        @gl.fn('any')
        def add(a: int, b: int) -> int:
            if bool(a):
                return -1
            return a + b

        expected = 'int test_named_compiler_add(int a, int b){if(bool(a)){return -1;}return (a) + (b);}'

        self.assertEqual(compile_named_shader(add, TEST_CONFIG), expected)

        @gl.fn('any')
        def add2(a: int, b: int) -> int:
            if bool(a):
                return -1
            else:
                return a + b

        expected2 = 'int test_named_compiler_add2(int a, int b){if(bool(a)){return -1;}else{return (a) + (b);}}'

        self.assertEqual(compile_named_shader(add2, TEST_CONFIG), expected2)

        @gl.fn('any')
        def add3(a: int, b: int) -> int:
            if bool(a):
                return -1
            elif bool(b):
                return -100
            else:
                return a + b

        expected3 = 'int test_named_compiler_add3(int a, int b){if(bool(a)){return -1;}else{if(bool(b)){return -100;}else{return (a) + (b);}}}'  # noqa: E501

        self.assertEqual(compile_named_shader(add3, TEST_CONFIG), expected3)

        @gl.fn('any')
        def add4(a: int, b: int) -> int:
            if bool(a):
                ...
            return a + b

        expected4 = 'int test_named_compiler_add4(int a, int b){if(bool(a)){;}return (a) + (b);}'

        self.assertEqual(compile_named_shader(add4, TEST_CONFIG), expected4)

    def test_ternary_op(self):

        @gl.fn('any')
        def add(a: int, b: int) -> int:
            return a + b if bool(a) else 12

        expected = 'int test_named_compiler_add(int a, int b){return bool(a) ? (a) + (b) : 12;}'

        self.assertEqual(compile_named_shader(add, TEST_CONFIG), expected)

        @gl.fn('any')
        def add2(a: int, b: int) -> int:
            return a + b if bool(a) else 900 if bool(b) else 12

        expected2 = 'int test_named_compiler_add2(int a, int b){return bool(a) ? (a) + (b) : bool(b) ? 900 : 12;}'

        self.assertEqual(compile_named_shader(add2, TEST_CONFIG), expected2)

    def test_compare_and_bool_op(self):

        @gl.fn('any')
        def add(a: int, b: int) -> int:
            if a > 1:
                return -1
            return a + b

        expected = 'int test_named_compiler_add(int a, int b){if((a) > (1)){return -1;}return (a) + (b);}'

        self.assertEqual(compile_named_shader(add, TEST_CONFIG), expected)

        @gl.fn('any')
        def add2(a: int, b: int) -> int:
            if 1 <= a < 12:
                return -1
            return a + b

        with self.assertRaisesRegex(CompileError, 'Multiple comparison operators in one expression') as _:
            compile_named_shader(add2)

        @gl.fn('any')
        def add3(a: int, b: int) -> int:
            if (1 <= a and a < 12) or b > 100 or a < -100:
                return -1
            return a + b

        expected3 = (
            'int test_named_compiler_add3(int a, int b){' +
            'if((((1) <= (a)) && ((a) < (12))) || ((b) > (100)) || ((a) < (-100))){return -1;}' +
            'return (a) + (b);' +
            '}')

        self.assertEqual(compile_named_shader(add3, TEST_CONFIG), expected3)

    def test_while(self):

        @gl.fn('any')
        def count() -> int:
            res: int = 0
            while res < 10:
                res += 1
            return res

        expected = (
            'int test_named_compiler_count(){' +
            'int res = 0;' +
            'while((res) < (10)){res += 1;}' +
            'return res;' +
            '}')

        self.assertEqual(compile_named_shader(count, TEST_CONFIG), expected)

        @gl.fn('any')
        def count2() -> int:
            res: int = 0
            while res < 10:
                res += 1
            else:
                res = 100
            return res

        with self.assertRaisesRegex(CompileError, 'Else with while is not supported') as _:
            compile_named_shader(count2)

        @gl.fn('any')
        def count3() -> int:
            res: int = 0
            while True:
                res += 1
                if res < 10:
                    continue
                else:
                    break
            return res

        expected3 = (
            'int test_named_compiler_count3(){' +
            'int res = 0;' +
            'while(true){res += 1;' +
            'if((res) < (10)){continue;}else{break;}}' +
            'return res;' +
            '}')

        self.assertEqual(compile_named_shader(count3, TEST_CONFIG), expected3)

    def test_for(self):

        @gl.fn('any')
        def count() -> int:
            res: int = 0
            for i in range(1, 10, 2):
                res += 1
            return res

        expected = (
            'int test_named_compiler_count(){' +
            'int res = 0;' +
            'for(int i=(1);i<(10);i+=(2)){res += 1;}' +
            'return res;' +
            '}')

        self.assertEqual(compile_named_shader(count, TEST_CONFIG), expected)

        @gl.fn('any')
        def count2() -> int:
            res: int = 0
            for arr in [1, 2, 3]:
                res += arr
            return res

        with self.assertRaisesRegex(CompileError, '^On For statement') as _:
            compile_named_shader(count2)

        @gl.fn('any')
        def count3() -> int:
            res: int = 0
            for i in range(10):
                res += i
            else:
                res = 100
            return res

        with self.assertRaisesRegex(CompileError, '^Else with For is not supported') as _:
            compile_named_shader(count3)


class TestBuffer(unittest.TestCase):

    def test_uniform(self):

        @gl.entry('frag')
        def vec_attr(buffer: ak.FragShader, color: gl.inout_p[gl.vec4]) -> None:
            color.value.x = buffer.time.value * 12

        expected = ''.join([
            'void test_named_compiler_vec_attr(inout vec4 color){color.x = (time) * (12);}',
        ])

        self.assertEqual(compile_named_shader(vec_attr, TEST_CONFIG), expected)

    def test_uniform_import(self):

        ddd = 12

        @gl.fn('poly')
        def local_add(a: int, b: int) -> int:
            return a + b

        @gl.entry('frag')
        def vec_attr(buffer: ak.FragShader, color: gl.inout_p[gl.vec4]) -> None:
            color.value.x = buffer.time.value * module_global_add(12, 1) * compiler_fixtures.boost_add(20, gl.eval(ddd))

        expected = ''.join([
            'int test_named_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (boost(b));}',
            'void test_named_compiler_vec_attr(inout vec4 color){color.x = ((time) * (module_global_add(12, 1))) * (boost_add(20, 12));}',
        ])

        self.assertEqual(compile_named_shader(vec_attr, TEST_CONFIG), expected)

    def test_forbidden_import(self):

        @gl.fn('poly')
        def local_add(a: int, b: int) -> int:
            return a + b

        @gl.entry('frag')
        def vec_attr(buffer: ak.FragShader, color: gl.inout_p[gl.vec4]) -> None:
            color.value.x = local_add(1, 2)

        with self.assertRaisesRegex(CompileError, 'Forbidden import PolygonShader from FragShader') as _:
            compile_named_shader(vec_attr, TEST_CONFIG)
