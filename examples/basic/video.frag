#version 400

uniform float time;
uniform vec2 resolution;

vec4 circle(float width, float r, float velocity) {
  vec2 m = vec2(0.5 * 2.0 - 1.0, -0.5 * 2.0 + 1.0);
  vec2 p =
      (gl_FragCoord.xy * 2.0 - resolution) / min(resolution.x, resolution.y);
  float t = width / abs(r * abs(sin(time * velocity)) - length(m - p));
  return vec4(vec3(t), 1.0);
}

vec4 point(float size, vec2 pos) {
  vec2 m = vec2(pos.x * 2.0 - 1.0, -pos.y * 2.0 + 1.0);
  vec2 p =
      (gl_FragCoord.xy * 2.0 - resolution) / min(resolution.x, resolution.y);
  float t = size / length(m - p);
  return vec4(vec3(t), 1.0);
}

vec4 invert_filter(in vec4 base) { return vec4(vec3(1.0 - base), base.a); }

vec4 edge_filter(in vec4 base) {
  return vec4(vec3(fwidth(length(base.rgb))), base.a);
}

void frag_main(inout vec4 _fragColor) {
  // _fragColor = invert_filter(_fragColor);
  // _fragColor = edge_filter(_fragColor);
  // _fragColor = circle(0.2, 2.0, 0.3) * _fragColor;
}
