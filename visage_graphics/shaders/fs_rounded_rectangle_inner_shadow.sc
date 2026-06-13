$input v_coordinates, v_dimensions, v_shader_values, v_shader_values1, v_position, v_gradient_pos, v_gradient_pos2, v_gradient_texture_pos

#include <shader_include.sh>

SAMPLER2D(s_gradient, 0);

void main() {
  gl_FragColor = gradient(s_gradient, v_gradient_texture_pos, v_gradient_pos, v_gradient_pos2, v_position);

  // v_shader_values.y  = blur (device pixels)
  // v_shader_values.z  = value1 = rounding (device pixels, doubled for p-space per existing convention)
  // v_shader_values.w  = value2 = dx (device pixels, doubled in shader for p-space)
  // v_shader_values1.x = value3 = dy (device pixels, doubled in shader for p-space)
  float blur    = v_shader_values.y;
  float rounding = v_shader_values.z;
  float dx      = v_shader_values.w;
  float dy      = v_shader_values1.x;

  vec2 p = v_coordinates * v_dimensions;

  // Shape mask: 1 inside, 0 outside, 1px soft transition.
  float shape_sdf = sdRoundedRectangle(p, v_dimensions, 2.0 * rounding);
  float inside_mask = smoothed(1.0, -1.0, shape_sdf);

  // Shadow comes from outside the shape offset by -(dx, dy) in screen space.
  // dx/dy are stored as device pixels; multiply by 2 to convert to p-space.
  float outer_sdf = sdRoundedRectangle(p - 2.0 * vec2(dx, dy), v_dimensions, 2.0 * rounding);

  // Bleed inward from the offset-shape boundary: 1 at/outside the boundary, 0 blur-px inside.
  float shadow = smoothed(-2.0 * blur, 2.0 * blur, outer_sdf);

  gl_FragColor.a *= shadow * inside_mask;
}
