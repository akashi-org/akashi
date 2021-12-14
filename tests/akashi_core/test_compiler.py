import unittest
from akashi_core.pysl import compile_shader_func, compile_shader_module, CompileError
from akashi_core import gl, ak
from . import compiler_fixtures


class TestFunctionCompiler(unittest.TestCase):

    def test_func_decl(self):

        @gl.test_func
        def decl_func(a: int, b: int) -> int: ...

        expected = 'int decl_func(int a, int b);'

        self.assertEqual(compile_shader_func(decl_func), expected)

    def test_inline_func(self):

        @gl.test_func
        def add(a: int, b: int) -> int:
            return a + b

        expected = '\n'.join([
            'int add(int a, int b){return (a) + (b);}',
        ])

        self.assertEqual(compile_shader_func(add), expected)

    def test_undecorated_func(self):

        def add(a: int, b: int) -> int:
            return a + b

        with self.assertRaisesRegex(CompileError, 'Shader function/method must be decorated properly') as _:
            compile_shader_func(add)

    def test_assign_and_call_func(self):

        @gl.test_func
        def add(a: int, b: int) -> int:
            return a + b

        @gl.test_func
        def assign_fn(a: int) -> int:
            c: int = add(a, 2)
            d: int = 90
            d = c = 89
            c += 190 + d
            return c

        expected = '\n'.join([
            'int assign_fn(int a){int c = add(a, 2);int d = 90;d = c = 89;c += (190) + (d);return c;}',
        ])

        self.assertEqual(compile_shader_func(assign_fn), expected)

    def test_paren_arith_func(self):

        @gl.test_func
        def paren_arith(a: int, b: int) -> int:
            return (100 - int(((12 * 90) + a) / b))

        expected = '\n'.join([
            'int paren_arith(int a, int b){return (100) - (int((((12) * (90)) + (a)) / (b)));}',
        ])

        self.assertEqual(compile_shader_func(paren_arith), expected)

    def test_vec_attr(self):

        @gl.test_func
        def vec_attr(_fragColor: gl.vec4) -> None:
            _fragColor.x = 12

        expected = '\n'.join([
            'void vec_attr(vec4 _fragColor){_fragColor.x = 12;}',
        ])

        self.assertEqual(compile_shader_func(vec_attr), expected)

        vec4 = gl.vec4

        @gl.test_func
        def vec_attr2(_fragColor: vec4) -> gl.vec4:
            _fragColor.x = 12
            return _fragColor

        expected2 = '\n'.join([
            'vec4 vec_attr2(vec4 _fragColor){_fragColor.x = 12;return _fragColor;}',
        ])

        self.assertEqual(compile_shader_func(vec_attr2), expected2)

    def test_params_qualifier(self):

        @gl.test_func
        def add(a: int, b: int) -> int:
            return a + b

        @gl.test_func
        def vec_attr(_fragColor: gl.inout_p[gl.vec4]) -> None:
            _fragColor.value.x = add(12, int(_fragColor.value.x))

        expected = '\n'.join([
            'void vec_attr(inout vec4 _fragColor){_fragColor.x = add(12, int(_fragColor.x));}',
        ])

        self.assertEqual(compile_shader_func(vec_attr), expected)

        @gl.test_func
        def vec_attr2(_fragColor: gl.out_p[gl.vec4]) -> gl.out_p[gl.vec4]:
            return _fragColor

        with self.assertRaisesRegex(CompileError, 'Return type must not have its parameter qualifier') as _:
            compile_shader_func(vec_attr2)


class Sub2Mod(ak.FragShader):
    # For TestShaderModuleCompiler

    @gl.func
    def boost(a: int) -> int:
        return a * 1000

    @gl.method
    def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
        return None


@gl.module
class SubMod(ak.FragShader):
    # For TestShaderModuleCompiler

    @gl.func
    def add(a: int, b: int) -> int:
        return a + b

    @gl.func
    def boost_add(a: int, b: int) -> int:
        return a + Sub2Mod.boost(b)

    @gl.func
    def boost_add2(a: int, b: int) -> int:
        return SubMod.add(a, Sub2Mod.boost(b))

    @gl.method
    def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
        return None


class AnyMod1(ak.AnyShader):
    # For TestShaderModuleCompiler

    @gl.func
    def forbidden_func() -> int:
        return SubMod.add(1, 2)

    @gl.func
    def rand(co: gl.vec2) -> float:
        return gl.fract(gl.sin(gl.dot(co.xy, gl.vec2(12.9898, 78.233))) * 43758.5453)


class TestShaderModuleCompiler(unittest.TestCase):

    def test_basic(self):

        @gl.module
        class SMod(ak.FragShader):

            @gl.func
            def add(a: int, b: int) -> int:
                return a + b

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                return None

        expected = 'int SMod_add(int a, int b){return (a) + (b);}void frag_main(inout vec4 color){return;}'

        self.assertEqual(compile_shader_module(SMod()), expected)

        @gl.module
        class UndecoMod(ak.FragShader):

            def add(a: int, b: int) -> int:  # type: ignore
                return a + b

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                return None

        self.assertEqual(compile_shader_module(UndecoMod()), 'void frag_main(inout vec4 color){return;}')

    def test_import(self):

        @gl.module
        class MainMod(ak.FragShader):

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: int = SubMod.add(1, 2)  # noqa: F841

        expected = ''.join([
            'int SubMod_add(int a, int b){return (a) + (b);}',
            'void frag_main(inout vec4 color){int c = SubMod_add(1, 2);}'
        ])

        self.assertEqual(compile_shader_module(MainMod()), expected)

    def test_recursive_import(self):

        @gl.module
        class MainMod(ak.FragShader):

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: int = SubMod.boost_add(1, 2)  # noqa: F841

        expected = ''.join([
            'int Sub2Mod_boost(int a){return (a) * (1000);}',
            'int SubMod_boost_add(int a, int b){return (a) + (Sub2Mod_boost(b));}',
            'void frag_main(inout vec4 color){int c = SubMod_boost_add(1, 2);}'
        ])

        self.assertEqual(compile_shader_module(MainMod()), expected)

    def test_recursive_and_self_import(self):

        @gl.module
        class MainMod(ak.FragShader):

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: int = SubMod.boost_add2(1, 2)  # noqa: F841

        expected = ''.join([
            'int SubMod_add(int a, int b){return (a) + (b);}'
            'int Sub2Mod_boost(int a){return (a) * (1000);}',
            'int SubMod_boost_add2(int a, int b){return SubMod_add(a, Sub2Mod_boost(b));}',
            'void frag_main(inout vec4 color){int c = SubMod_boost_add2(1, 2);}'
        ])

        self.maxDiff = None

        self.assertEqual(compile_shader_module(MainMod()), expected)

    def test_module_import(self):

        @gl.module
        class MainMod(ak.FragShader):

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: int = compiler_fixtures.OuterMod.boost_sub(1, 2)  # noqa: F841

        expected = ''.join([
            'int OuterMod_sub(int a, int b){return (a) - (b);}',
            'int OuterMod_boost_sub(int a, int b){return OuterMod_sub(a, (b) * (10));}',
            'void frag_main(inout vec4 color){int c = OuterMod_boost_sub(1, 2);}'
        ])

        self.assertEqual(compile_shader_module(MainMod()), expected)

    def test_dynamic(self):

        @gl.module
        class MainMod(ak.FragShader):

            dvalue: gl.dynamic[float]

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: float = 2.0 + self.dvalue.value  # noqa: F841

        expected = ''.join([
            'void frag_main(inout vec4 color){float c = (2.0) + (1.2);}'
        ])

        self.assertEqual(compile_shader_module(MainMod(gl.dynamic(1.2))), expected)

    def test_uniform(self):

        @gl.module
        class MainMod(ak.FragShader):

            dvalue: gl.dynamic[float]

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: float = 2.0 + self.global_time.value  # noqa: F841

        expected = ''.join([
            'void frag_main(inout vec4 color){float c = (2.0) + (global_time);}'
        ])

        self.assertEqual(compile_shader_module(MainMod(gl.dynamic(1.2))), expected)

    def test_anyshader(self):

        @gl.module
        class MainMod(ak.FragShader):

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: float = AnyMod1.rand(gl.vec2(1, 2))  # noqa: F841

        expected = ''.join([
            'float AnyMod1_rand(vec2 co){return fract((sin(dot(co.xy, vec2(12.9898, 78.233)))) * (43758.5453));}',
            'void frag_main(inout vec4 color){float c = AnyMod1_rand(vec2(1, 2));}'
        ])

        self.assertEqual(compile_shader_module(MainMod()), expected)

        @gl.module
        class MainMod2(ak.FragShader):

            @gl.method
            def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
                c: int = AnyMod1.forbidden_func()  # noqa: F841

        with self.assertRaisesRegex(CompileError, 'Forbidden import FragShader from AnyShader') as _:
            compile_shader_module(MainMod2())


class TestControlCompiler(unittest.TestCase):

    def test_if(self):

        @gl.test_func
        def add(a: int, b: int) -> int:
            if bool(a):
                return -1
            return a + b

        expected = 'int add(int a, int b){if(bool(a)){return -1;}return (a) + (b);}'

        self.assertEqual(compile_shader_func(add), expected)

        @gl.test_func
        def add2(a: int, b: int) -> int:
            if bool(a):
                return -1
            else:
                return a + b

        expected2 = 'int add2(int a, int b){if(bool(a)){return -1;}else{return (a) + (b);}}'

        self.assertEqual(compile_shader_func(add2), expected2)

        @gl.test_func
        def add3(a: int, b: int) -> int:
            if bool(a):
                return -1
            elif bool(b):
                return -100
            else:
                return a + b

        expected3 = 'int add3(int a, int b){if(bool(a)){return -1;}else{if(bool(b)){return -100;}else{return (a) + (b);}}}'  # noqa: E501

        self.assertEqual(compile_shader_func(add3), expected3)

        @gl.test_func
        def add4(a: int, b: int) -> int:
            if bool(a):
                ...
            return a + b

        expected4 = 'int add4(int a, int b){if(bool(a)){;}return (a) + (b);}'

        self.assertEqual(compile_shader_func(add4), expected4)

    def test_ternary_op(self):

        @gl.test_func
        def add(a: int, b: int) -> int:
            return a + b if bool(a) else 12

        expected = 'int add(int a, int b){return bool(a) ? (a) + (b) : 12;}'

        self.assertEqual(compile_shader_func(add), expected)

        @gl.test_func
        def add2(a: int, b: int) -> int:
            return a + b if bool(a) else 900 if bool(b) else 12

        expected2 = 'int add2(int a, int b){return bool(a) ? (a) + (b) : bool(b) ? 900 : 12;}'

        self.assertEqual(compile_shader_func(add2), expected2)

    def test_compare_and_bool_op(self):

        @gl.test_func
        def add(a: int, b: int) -> int:
            if a > 1:
                return -1
            return a + b

        expected = 'int add(int a, int b){if((a) > (1)){return -1;}return (a) + (b);}'

        self.assertEqual(compile_shader_func(add), expected)

        @gl.test_func
        def add2(a: int, b: int) -> int:
            if 1 <= a < 12:
                return -1
            return a + b

        with self.assertRaisesRegex(CompileError, 'Multiple comparison operators in one expression') as _:
            compile_shader_func(add2)

        @gl.test_func
        def add3(a: int, b: int) -> int:
            if (1 <= a and a < 12) or b > 100 or a < -100:
                return -1
            return a + b

        expected3 = (
            'int add3(int a, int b){' +
            'if((((1) <= (a)) && ((a) < (12))) || ((b) > (100)) || ((a) < (-100))){return -1;}' +
            'return (a) + (b);' +
            '}')

        self.assertEqual(compile_shader_func(add3), expected3)

    def test_while(self):

        @gl.test_func
        def count() -> int:
            res: int = 0
            while res < 10:
                res += 1
            return res

        expected = (
            'int count(){' +
            'int res = 0;' +
            'while((res) < (10)){res += 1;}' +
            'return res;' +
            '}')

        self.assertEqual(compile_shader_func(count), expected)

        @gl.test_func
        def count2() -> int:
            res: int = 0
            while res < 10:
                res += 1
            else:
                res = 100
            return res

        with self.assertRaisesRegex(CompileError, 'Else with while is not supported') as _:
            compile_shader_func(count2)

        @gl.test_func
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
            'int count3(){' +
            'int res = 0;' +
            'while(true){res += 1;' +
            'if((res) < (10)){continue;}else{break;}}' +
            'return res;' +
            '}')

        self.assertEqual(compile_shader_func(count3), expected3)

    def test_for(self):

        @gl.test_func
        def count() -> int:
            res: int = 0
            for i in range(1, 10, 2):
                res += 1
            return res

        expected = (
            'int count(){' +
            'int res = 0;' +
            'for(int i=(1);i<(10);i+=(2)){res += 1;}' +
            'return res;' +
            '}')

        self.assertEqual(compile_shader_func(count), expected)

        @gl.test_func
        def count2() -> int:
            res: int = 0
            for arr in [1, 2, 3]:
                res += arr
            return res

        with self.assertRaisesRegex(CompileError, '^On For statement') as _:
            compile_shader_func(count2)

        @gl.test_func
        def count3() -> int:
            res: int = 0
            for i in range(10):
                res += i
            else:
                res = 100
            return res

        with self.assertRaisesRegex(CompileError, '^Else with For is not supported') as _:
            compile_shader_func(count3)


if __name__ == "__main__":
    unittest.main()
