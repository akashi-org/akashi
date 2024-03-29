# pyright: reportPrivateUsage=false
import unittest
from akashi_core.pysl.compiler.items import CompileError, CompilerConfig, GLSLFunc, CompileCache
from akashi_core.pysl.compiler.compiler import compile_entry_shaders, compile_lib_shader
from akashi_core.pysl.compiler.evaluator import eval_glsl_fns, eval_entry_glsl_fns
from akashi_core import gl, ak
from . import compiler_fixtures
import typing as tp
from typing import overload
import time

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import _TEntryFnOpaque

TEST_CONFIG: CompilerConfig.Config = {
    'pretty_compile': False,
    'mangle_mode': 'soft'
}


def exec_compile_lib_shader(
        fn: tp.Callable,
        config: CompilerConfig.Config,
        cache: CompileCache | None = None) -> str:
    return ''.join(eval_glsl_fns(compile_lib_shader(fn, config, cache)))


def exec_compile_entry_shaders(
        fns: tuple['_TEntryFnOpaque', ...],
        buffer_type: tp.Type[gl._buffer_type],
        config: CompilerConfig.Config,
        cache: CompileCache | None = None) -> str:
    return ''.join(eval_entry_glsl_fns(compile_entry_shaders(fns, buffer_type, config, cache)))


@gl.lib('any')
def module_global_add(a: int, b: int) -> int:
    return a + b


class TestBasic(unittest.TestCase):

    def test_func_decl(self):

        @gl.lib('any')
        def decl_func(a: int, b: int) -> int: ...

        expected = 'int test_compiler_decl_func(int a, int b);'

        self.assertEqual(exec_compile_lib_shader(decl_func, TEST_CONFIG), expected)

    def test_comments(self):

        @gl.lib('any')
        def add_normal_comment(a: int, b: int) -> int:
            return a + b  # comment

        expected_normal = '\n'.join([
            'int test_compiler_add_normal_comment(int a, int b){return (a) + (b);}',
        ])
        self.assertEqual(exec_compile_lib_shader(add_normal_comment, TEST_CONFIG), expected_normal)

        @gl.lib('any')
        def add_triple_quotes(a: int, b: int) -> int:
            ''' In fact, this is not a comment. '''
            return a + b

        with self.assertRaisesRegex(CompileError, 'Strings are not allowed by default') as _:
            exec_compile_lib_shader(add_triple_quotes, TEST_CONFIG)

    def test_inline_func(self):

        @gl.lib('any')
        def add(a: int, b: int) -> int:
            return a + b

        expected = '\n'.join([
            'int test_compiler_add(int a, int b){return (a) + (b);}',
        ])

        self.assertEqual(exec_compile_lib_shader(add, TEST_CONFIG), expected)

    def test_undecorated_func(self):

        def add(a: int, b: int) -> int:
            return a + b

        with self.assertRaisesRegex(CompileError, 'Named shader function must be decorated properly') as _:
            exec_compile_lib_shader(add, TEST_CONFIG)

        @gl.lib('error kind')  # type: ignore
        def add2(a: int, b: int) -> int:
            return a + b

        with self.assertRaisesRegex(CompileError, 'Invalid shader kind') as _:
            exec_compile_lib_shader(add2, TEST_CONFIG)

    def test_import(self):

        @gl.lib('frag')
        def add(a: int, b: int) -> int:
            return a + b

        @gl.lib('frag')
        def assign_fn(a: int) -> int:
            c: int = add(a, 2)
            d: int = module_global_add(90, 2)
            d = c = 89
            c += 190 + d
            return c

        expected = ''.join([
            'int test_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'int test_compiler_add(int a, int b){return (a) + (b);}',
            'int test_compiler_assign_fn(int a){int c = test_compiler_add(a, 2);int d = test_compiler_module_global_add(90, 2);d = c = 89;c += (190) + (d);return c;}',
        ])

        self.maxDiff = None
        self.assertEqual(exec_compile_lib_shader(assign_fn, TEST_CONFIG), expected)

        @gl.lib('any')
        def error_import(a: int) -> int:
            return add(a, 12)

        with self.assertRaisesRegex(CompileError, 'Forbidden import FragShader from AnyShader') as _:
            exec_compile_lib_shader(error_import, TEST_CONFIG)

    def test_module_import(self):

        @gl.lib('frag')
        def add(a: int, b: int) -> int:
            return compiler_fixtures.boost_add(a, b)

        expected = ''.join([
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (compiler_fixtures_boost(b));}',
            'int test_compiler_add(int a, int b){return compiler_fixtures_boost_add(a, b);}'
        ])

        self.assertEqual(exec_compile_lib_shader(add, TEST_CONFIG), expected)

    def test_paren_arith_func(self):

        @gl.lib('any')
        def paren_arith(a: int, b: int) -> int:
            return (100 - int(((12 * 90) + a) / b))

        expected = ''.join([
            'int test_compiler_paren_arith(int a, int b){return (100) - (int((((12) * (90)) + (a)) / (b)));}',
        ])

        self.assertEqual(exec_compile_lib_shader(paren_arith, TEST_CONFIG), expected)

    def test_vec_attr(self):

        @gl.lib('frag')
        def vec_attr(_fragColor: gl.vec4) -> None:
            _fragColor.x = 12

        expected = ''.join([
            'void test_compiler_vec_attr(vec4 _fragColor){_fragColor.x = 12;}',
        ])

        self.assertEqual(exec_compile_lib_shader(vec_attr, TEST_CONFIG), expected)

        vec4 = gl.vec4

        @gl.lib('frag')
        def vec_attr2(_fragColor: vec4) -> gl.vec4:
            _fragColor.x = 12
            return _fragColor

        expected2 = ''.join([
            'vec4 test_compiler_vec_attr2(vec4 _fragColor){_fragColor.x = 12;return _fragColor;}',
        ])

        self.assertEqual(exec_compile_lib_shader(vec_attr2, TEST_CONFIG), expected2)

    def test_params_qualifier(self):

        @gl.lib('any')
        def add(a: int, b: int) -> int:
            return a + b

        @gl.lib('frag')
        def vec_attr(_fragColor: gl.inout_p[gl.vec4]) -> None:
            _fragColor.x = add(12, int(_fragColor.x))

        expected = ''.join([
            'int test_compiler_add(int a, int b){return (a) + (b);}',
            'void test_compiler_vec_attr(inout vec4 _fragColor){_fragColor.x = test_compiler_add(12, int(_fragColor.x));}',
        ])

        self.maxDiff = None
        self.assertEqual(exec_compile_lib_shader(vec_attr, TEST_CONFIG), expected)

        @gl.lib('frag')
        def vec_attr2(_fragColor: gl.out_p[gl.vec4]) -> gl.out_p[gl.vec4]:
            return _fragColor

        with self.assertRaisesRegex(CompileError, 'Return type must not have its parameter qualifier') as _:
            exec_compile_lib_shader(vec_attr2, TEST_CONFIG)

    def test_boolean_literal(self):

        out_true = True

        @gl.lib('any')
        def is_positive(a: int) -> bool:
            return gl.ceval(out_true) if a >= 0 else False

        expected = '\n'.join([
            'bool test_compiler_is_positive(int a){return (a) >= (0) ? true : false;}',
        ])

        self.assertEqual(exec_compile_lib_shader(is_positive, TEST_CONFIG), expected)

    def test_slice(self):

        @gl.lib('any')
        def slice_exp() -> gl.vec2:
            return gl.mat2(gl.vec2(1)[1], 1)[0]

        expected = '\n'.join([
            'vec2 test_compiler_slice_exp(){return mat2(vec2(1)[1], 1)[0];}',
        ])

        self.assertEqual(exec_compile_lib_shader(slice_exp, TEST_CONFIG), expected)

    def test_aliased_builtin_func(self):

        @gl.lib('any')
        def aliased_func() -> None:
            gl.any_(gl.bvec2(True, False))
            gl.all_(gl.bvec2(True, False))
            gl.not_(gl.bvec2(True, False))

        expected = '\n'.join([
            'void test_compiler_aliased_func(){any(bvec2(true, false));all(bvec2(true, false));not(bvec2(true, false));}',
        ])

        self.assertEqual(exec_compile_lib_shader(aliased_func, TEST_CONFIG), expected)


class TestControl(unittest.TestCase):

    def test_if(self):

        @gl.lib('any')
        def add(a: int, b: int) -> int:
            if bool(a):
                return -1
            return a + b

        expected = 'int test_compiler_add(int a, int b){if(bool(a)){return -1;}return (a) + (b);}'

        self.assertEqual(exec_compile_lib_shader(add, TEST_CONFIG), expected)

        @gl.lib('any')
        def add2(a: int, b: int) -> int:
            if bool(a):
                return -1
            else:
                return a + b

        expected2 = 'int test_compiler_add2(int a, int b){if(bool(a)){return -1;}else{return (a) + (b);}}'

        self.assertEqual(exec_compile_lib_shader(add2, TEST_CONFIG), expected2)

        @gl.lib('any')
        def add3(a: int, b: int) -> int:
            if bool(a):
                return -1
            elif bool(b):
                return -100
            else:
                return a + b

        expected3 = 'int test_compiler_add3(int a, int b){if(bool(a)){return -1;}else{if(bool(b)){return -100;}else{return (a) + (b);}}}'  # noqa: E501

        self.assertEqual(exec_compile_lib_shader(add3, TEST_CONFIG), expected3)

        @gl.lib('any')
        def add4(a: int, b: int) -> int:
            if bool(a):
                ...
            return a + b

        expected4 = 'int test_compiler_add4(int a, int b){if(bool(a)){;}return (a) + (b);}'

        self.assertEqual(exec_compile_lib_shader(add4, TEST_CONFIG), expected4)

    def test_ternary_op(self):

        @gl.lib('any')
        def add(a: int, b: int) -> int:
            return a + b if bool(a) else 12

        expected = 'int test_compiler_add(int a, int b){return bool(a) ? (a) + (b) : 12;}'

        self.assertEqual(exec_compile_lib_shader(add, TEST_CONFIG), expected)

        @gl.lib('any')
        def add2(a: int, b: int) -> int:
            return a + b if bool(a) else 900 if bool(b) else 12

        expected2 = 'int test_compiler_add2(int a, int b){return bool(a) ? (a) + (b) : bool(b) ? 900 : 12;}'

        self.assertEqual(exec_compile_lib_shader(add2, TEST_CONFIG), expected2)

    def test_compare_and_bool_op(self):

        @gl.lib('any')
        def add(a: int, b: int) -> int:
            if a > 1:
                return -1
            return a + b

        expected = 'int test_compiler_add(int a, int b){if((a) > (1)){return -1;}return (a) + (b);}'

        self.assertEqual(exec_compile_lib_shader(add, TEST_CONFIG), expected)

        @gl.lib('any')
        def add2(a: int, b: int) -> int:
            if 1 <= a < 12:
                return -1
            return a + b

        with self.assertRaisesRegex(CompileError, 'Multiple comparison operators in one expression') as _:
            exec_compile_lib_shader(add2, TEST_CONFIG)

        @gl.lib('any')
        def add3(a: int, b: int) -> int:
            if (1 <= a and a < 12) or b > 100 or a < -100:
                return -1
            return a + b

        expected3 = (
            'int test_compiler_add3(int a, int b){' +
            'if((((1) <= (a)) && ((a) < (12))) || ((b) > (100)) || ((a) < (-100))){return -1;}' +
            'return (a) + (b);' +
            '}')

        self.assertEqual(exec_compile_lib_shader(add3, TEST_CONFIG), expected3)

    def test_while(self):

        @gl.lib('any')
        def count() -> int:
            res: int = 0
            while res < 10:
                res += 1
            return res

        expected = (
            'int test_compiler_count(){' +
            'int res = 0;' +
            'while((res) < (10)){res += 1;}' +
            'return res;' +
            '}')

        self.assertEqual(exec_compile_lib_shader(count, TEST_CONFIG), expected)

        @gl.lib('any')
        def count2() -> int:
            res: int = 0
            while res < 10:
                res += 1
            else:
                res = 100
            return res

        with self.assertRaisesRegex(CompileError, 'Else with while is not supported') as _:
            exec_compile_lib_shader(count2, TEST_CONFIG)

        @gl.lib('any')
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
            'int test_compiler_count3(){' +
            'int res = 0;' +
            'while(true){res += 1;' +
            'if((res) < (10)){continue;}else{break;}}' +
            'return res;' +
            '}')

        self.assertEqual(exec_compile_lib_shader(count3, TEST_CONFIG), expected3)

    def test_for(self):

        @gl.lib('any')
        def count() -> int:
            res: int = 0
            for i in range(1, 10, 2):
                res += 1
            return res

        expected = (
            'int test_compiler_count(){' +
            'int res = 0;' +
            'for(int i=(1);i<(10);i+=(2)){res += 1;}' +
            'return res;' +
            '}')

        self.assertEqual(exec_compile_lib_shader(count, TEST_CONFIG), expected)

        @gl.lib('any')
        def count2() -> int:
            res: int = 0
            for arr in [1, 2, 3]:
                res += arr
            return res

        with self.assertRaisesRegex(CompileError, '^On For statement') as _:
            exec_compile_lib_shader(count2, TEST_CONFIG)

        @gl.lib('any')
        def count3() -> int:
            res: int = 0
            for i in range(10):
                res += i
            else:
                res = 100
            return res

        with self.assertRaisesRegex(CompileError, '^Else with For is not supported') as _:
            exec_compile_lib_shader(count3, TEST_CONFIG)

        to = 10

        @gl.lib('any')
        def count4() -> int:
            res: int = 0
            for i in range(1, gl.ceval(to), 2):
                res += 1
            return res

        expected = (
            'int test_compiler_count4(){' +
            'int res = 0;' +
            'for(int i=(1);i<(10);i+=(2)){res += 1;}' +
            'return res;' +
            '}')

        self.assertEqual(exec_compile_lib_shader(count4, TEST_CONFIG), expected)


class TestBuffer(unittest.TestCase):

    def test_uniform(self):

        @gl.entry(ak.frag)
        def vec_attr(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            color.x = buffer.time * 12

        expected = ''.join([
            'void frag_main(inout vec4 color){color.x = (time) * (12);}',
        ])

        self.assertEqual(exec_compile_entry_shaders((vec_attr,), ak.frag, TEST_CONFIG), expected)

    def test_uniform_import(self):

        ddd = 12

        @gl.lib('poly')
        def local_add(a: int, b: int) -> int:
            return a + b

        @gl.entry(ak.frag)
        def vec_attr(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            color.x = buffer.time * module_global_add(12, 1) * compiler_fixtures.boost_add(20, gl.eval(ddd))

        expected = ''.join([
            'int test_compiler_module_global_add(int a, int b);',
            'int compiler_fixtures_boost(int v);',
            'int compiler_fixtures_boost_add(int a, int b);',
            'int test_compiler_module_global_add(int a, int b){return (a) + (b);}',
            'int compiler_fixtures_boost(int v){return (v) * (1000);}',
            'int compiler_fixtures_boost_add(int a, int b){return (a) + (compiler_fixtures_boost(b));}',
            'void frag_main(inout vec4 color){color.x = ((time) * (test_compiler_module_global_add(12, 1))) * (compiler_fixtures_boost_add(20, 12));}',
        ])

        self.maxDiff = None
        self.assertEqual(exec_compile_entry_shaders((vec_attr, ), ak.frag, TEST_CONFIG), expected)

    def test_forbidden_import(self):

        @gl.lib('poly')
        def local_add(a: int, b: int) -> int:
            return a + b

        @gl.entry(ak.frag)
        def vec_attr(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            color.x = local_add(1, 2)

        with self.assertRaisesRegex(CompileError, 'Forbidden import PolygonShader from FragShader') as _:
            exec_compile_entry_shaders((vec_attr, ), ak.frag, TEST_CONFIG)


class TestEntry(unittest.TestCase):

    def test_basic(self):

        @gl.entry(ak.frag)
        def vec_attr(buffer: ak.frag, cl: gl.inout_p[gl.vec4]) -> None:
            cl.x = buffer.time * 12

        expected = ''.join([
            'void frag_main(inout vec4 color){color.x = (time) * (12);}',
        ])

        self.assertEqual(exec_compile_entry_shaders((vec_attr,), ak.frag, TEST_CONFIG), expected)

    def test_chain(self):

        @gl.entry(ak.frag)
        def vec_attr(buffer: ak.frag, cl: gl.inout_p[gl.vec4]) -> None:
            cl.x = buffer.time * 12

        @gl.entry(ak.frag)
        def vec_attr2(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            color.y = buffer.time * 12

        @gl.entry(ak.frag)
        def vec_attr3(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            color.z = buffer.time * 12

        expected = ''.join([
            'void frag_main_2(inout vec4 color){color.z = (time) * (12);}',
            'void frag_main_1(inout vec4 color){color.y = (time) * (12);frag_main_2(color);}',
            'void frag_main(inout vec4 color){color.x = (time) * (12);frag_main_1(color);}',
        ])

        self.maxDiff = None
        self.assertEqual(exec_compile_entry_shaders((vec_attr, vec_attr2, vec_attr3),
                         ak.frag, TEST_CONFIG), expected)


class TestClosure(unittest.TestCase):

    def test_basic(self):

        def gen(arg_value: int):
            @gl.entry(ak.frag)
            def vec_attr(buffer: ak.frag, cl: gl.inout_p[gl.vec4]) -> None:
                cl.x = buffer.time * 12 + gl.eval(arg_value)
            return vec_attr

        expected = ''.join([
            'void frag_main(inout vec4 color){color.x = ((time) * (12)) + (999);}',
        ])

        self.assertEqual(exec_compile_entry_shaders((gen(999),), ak.frag, TEST_CONFIG), expected)


class TestExtension(unittest.TestCase):

    def test_ceval(self):

        outer_value = 12

        @gl.lib('any')
        def func() -> None:
            gl.eval(outer_value)

        with self.assertRaisesRegex(CompileError, r'gl.eval\(\) is forbidden in lib shader') as _:
            exec_compile_lib_shader(func, TEST_CONFIG)

    def test_inline(self):

        @gl.lib('any')
        def inline_stmt_func() -> None:
            gl.inline_stmt('return 12;')

        expected = 'void test_compiler_inline_stmt_func(){return 12;}'

        self.assertEqual(exec_compile_lib_shader(inline_stmt_func, TEST_CONFIG), expected)


class TestOther(unittest.TestCase):

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
            'float test_compiler_common_func();',
            'float test_compiler_another_func();',
            'float test_compiler_common_func(){return 41;}',
            'float test_compiler_another_func(){return (test_compiler_common_func()) * (12);}',
            'void frag_main(inout vec4 color){float r = test_compiler_common_func();color.x = test_compiler_another_func();}'
        ])

        self.maxDiff = None
        self.assertEqual(exec_compile_entry_shaders((entry,), ak.frag, TEST_CONFIG), expected)


class TestCache(unittest.TestCase):

    def test_cached_lib_shader(self):

        @gl.lib('frag')
        def add(a: int, b: int) -> int:
            return compiler_fixtures.outer_func1(a, b)

        expected = ''.join([
            'int compiler_fixtures_outer_func1(int a, int b){return (a) + (12);}',
            'int test_compiler_add(int a, int b){return compiler_fixtures_outer_func1(a, b);}'
        ])

        def _check(_cache: CompileCache | None) -> float:
            st = time.time()
            for _ in range(50):
                self.assertEqual(exec_compile_lib_shader(add, TEST_CONFIG, _cache), expected)
            return time.time() - st

        cache = CompileCache(config=TEST_CONFIG)
        compile_lib_shader(add, TEST_CONFIG, cache)
        cache_time = _check(cache)
        no_cache_time = _check(None)

        print(f'test_cached_lib_shader: no_cache: {no_cache_time}, cache: {cache_time}')
        self.assertGreaterEqual(no_cache_time, cache_time)

    def test_cached_entry_shaders(self):

        @gl.entry(ak.frag)
        def entry_add(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            compiler_fixtures.outer_func1(1, 12)

        expected = ''.join([
            'int compiler_fixtures_outer_func1(int a, int b);',
            'int compiler_fixtures_outer_func1(int a, int b){return (a) + (12);}',
            'void frag_main(inout vec4 color){compiler_fixtures_outer_func1(1, 12);}'
        ])

        def _check(_cache: CompileCache | None) -> float:
            st = time.time()
            for _ in range(50):
                self.assertEqual(exec_compile_entry_shaders((entry_add,), ak.frag, TEST_CONFIG, _cache), expected)
            return time.time() - st

        cache = CompileCache(config=TEST_CONFIG)
        compile_entry_shaders((entry_add,), ak.frag, TEST_CONFIG, cache)
        cache_time = _check(cache)
        no_cache_time = _check(None)

        print(f'test_cached_entry_shaders: no_cache: {no_cache_time}, cache: {cache_time}')
        self.assertGreaterEqual(no_cache_time, cache_time)

    def test_cached_entry_shaders_with_outer(self):

        outer_value = 100

        @gl.entry(ak.frag)
        def entry_add(buffer: ak.frag, color: gl.inout_p[gl.vec4]) -> None:
            compiler_fixtures.outer_func1(1, gl.eval(100))

        expected = ''.join([
            'int compiler_fixtures_outer_func1(int a, int b);',
            'int compiler_fixtures_outer_func1(int a, int b){return (a) + (12);}',
            'void frag_main(inout vec4 color){compiler_fixtures_outer_func1(1, 100);}'
        ])

        def _check(_cache: CompileCache | None) -> float:
            st = time.time()
            for _ in range(50):
                self.assertEqual(exec_compile_entry_shaders((entry_add,), ak.frag, TEST_CONFIG, _cache), expected)
            return time.time() - st

        cache = CompileCache(config=TEST_CONFIG)
        compile_entry_shaders((entry_add,), ak.frag, TEST_CONFIG, cache)
        cache_time = _check(cache)
        no_cache_time = _check(None)

        print(f'test_cached_entry_shaders_with_outer: no_cache: {no_cache_time}, cache: {cache_time}')
        self.assertGreaterEqual(no_cache_time, cache_time)
