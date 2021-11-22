from akashi_core import ak, gl


@gl.module
class OuterMod(ak.FragShader):
    # For TestShaderModuleCompiler

    @gl.func
    def sub(a: int, b: int) -> int:
        return a - b

    @gl.func
    def boost_sub(a: int, b: int) -> int:
        return OuterMod.sub(a, b * 10)

    @gl.method
    def frag_main(self, color: gl.inout_p[gl.vec4]) -> None:
        return None
