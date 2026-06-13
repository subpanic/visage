$input v_coordinates, v_dimensions, v_shader_values, v_shader_values1, v_position, v_gradient_pos, v_gradient_pos2, v_gradient_texture_pos

#include <shader_include.sh>

SAMPLER2D(s_gradient, 0);

void main() {
  gl_FragColor = gradient(s_gradient, v_gradient_texture_pos, v_gradient_pos, v_gradient_pos2, v_position);

  // v_shader_values.y  = blur (device pixels)
  // v_shader_values.z  = value1 = top_left radius
  // v_shader_values.w  = value2 = top_right radius
  // v_shader_values1.x = value3 = bottom_left radius
  // v_shader_values1.y = value4 = bottom_right radius
  // v_shader_values1.z = value5 = dx (device pixels, doubled in shader for p-space)
  // v_shader_values1.w = value6 = dy (device pixels, doubled in shader for p-space)
  float blur = v_shader_values.y;
  vec4 radii = vec4(v_shader_values.z, v_shader_values.w, v_shader_values1.x, v_shader_values1.y);
  float dx   = v_shader_values1.z;
  float dy   = v_shader_values1.w;

  vec2 p = v_coordinates * v_dimensions;

  // Shape mask: 1 inside, 0 outside, 1px soft transition.
  float shape_sdf = sdRoundedRectangle4(p, v_dimensions, radii);
  float inside_mask = smoothed(1.0, -1.0, shape_sdf);

  // Shadow comes from outside the shape offset by -(dx, dy) in screen space.
  // dx/dy are stored as device pixels; multiply by 2 to convert to p-space.
  float outer_sdf = sdRoundedRectangle4(p - 2.0 * vec2(dx, dy), v_dimensions, radii);

  // Bleed inward from the offset-shape boundary: 1 at/outside the boundary, 0 blur-px inside.
  float shadow = smoothed(-2.0 * blur, 2.0 * blur, outer_sdf);

  gl_FragColor.a *= shadow * inside_mask;
}
