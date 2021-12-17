import unittest
from akashi_core.pysl import compile_named_shader, CompileError
from akashi_core import gl, ak


class TestFunctionCompiler(unittest.TestCase):

    def test_func_decl(self):

        @gl.fn('any')
        def decl_func(a: int, b: int) -> int: ...

        expected = 'int decl_func(int a, int b);'

        self.assertEqual(compile_named_shader(decl_func), expected)

    def test_inline_func(self):

        @gl.fn('any')
        def add(a: int, b: int) -> int:
            return a + b

        expected = '\n'.join([
            'int add(int a, int b){return (a) + (b);}',
        ])

        self.assertEqual(compile_named_shader(add), expected)

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

    # def test_assign_and_call_func(self):

    #     @gl.test_func
    #     def add(a: int, b: int) -> int:
    #         return a + b

    #     @gl.test_func
    #     def assign_fn(a: int) -> int:
    #         c: int = add(a, 2)
    #         d: int = 90
    #         d = c = 89
    #         c += 190 + d
    #         return c

    #     expected = '\n'.join([
    #         'int assign_fn(int a){int c = add(a, 2);int d = 90;d = c = 89;c += (190) + (d);return c;}',
    #     ])

    #     self.assertEqual(compile_named_shader(assign_fn), expected)

    # def test_paren_arith_func(self):

    #     @gl.test_func
    #     def paren_arith(a: int, b: int) -> int:
    #         return (100 - int(((12 * 90) + a) / b))

    #     expected = '\n'.join([
    #         'int paren_arith(int a, int b){return (100) - (int((((12) * (90)) + (a)) / (b)));}',
    #     ])

    #     self.assertEqual(compile_named_shader(paren_arith), expected)

    # def test_vec_attr(self):

    #     @gl.test_func
    #     def vec_attr(_fragColor: gl.vec4) -> None:
    #         _fragColor.x = 12

    #     expected = '\n'.join([
    #         'void vec_attr(vec4 _fragColor){_fragColor.x = 12;}',
    #     ])

    #     self.assertEqual(compile_named_shader(vec_attr), expected)

    #     vec4 = gl.vec4

    #     @gl.test_func
    #     def vec_attr2(_fragColor: vec4) -> gl.vec4:
    #         _fragColor.x = 12
    #         return _fragColor

    #     expected2 = '\n'.join([
    #         'vec4 vec_attr2(vec4 _fragColor){_fragColor.x = 12;return _fragColor;}',
    #     ])

    #     self.assertEqual(compile_named_shader(vec_attr2), expected2)

    # def test_params_qualifier(self):

    #     @gl.test_func
    #     def add(a: int, b: int) -> int:
    #         return a + b

    #     @gl.test_func
    #     def vec_attr(_fragColor: gl.inout_p[gl.vec4]) -> None:
    #         _fragColor.value.x = add(12, int(_fragColor.value.x))

    #     expected = '\n'.join([
    #         'void vec_attr(inout vec4 _fragColor){_fragColor.x = add(12, int(_fragColor.x));}',
    #     ])

    #     self.assertEqual(compile_named_shader(vec_attr), expected)

    #     @gl.test_func
    #     def vec_attr2(_fragColor: gl.out_p[gl.vec4]) -> gl.out_p[gl.vec4]:
    #         return _fragColor

    #     with self.assertRaisesRegex(CompileError, 'Return type must not have its parameter qualifier') as _:
    #         compile_named_shader(vec_attr2)
