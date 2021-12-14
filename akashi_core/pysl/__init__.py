# pyright: reportUnusedImport=false
from . import _gl as gl
from .compiler.items import CompilerConfig, CompileError
from .compiler.compiler import compile_shader_module, compile_shader_func
from .compiler.compiler_inline import compile_inline_shader

from .shader import FragShader, PolygonShader, GeomShader, AnyShader
from .shader import EntryFragFn, EntryPolyFn
from .shader import VideoFragShader, VideoGeomShader, VideoPolygonShader
