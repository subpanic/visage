$input v_coordinates, v_dimensions, v_shader_values, v_shader_values1, v_position, v_gradient_pos, v_gradient_pos2, v_gradient_texture_pos

#include <shader_include.sh>

SAMPLER2D(s_gradient, 0);

void main() {
  gl_FragColor = gradient(s_gradient, v_gradient_texture_pos, v_gradient_pos, v_gradient_pos2, v_position);

  // v_shader_values.y  = blur (device pixels, stored as fade)
  // v_shader_values.z  = value1 = top_left radius
  // v_shader_values.w  = value2 = top_right radius
  // v_shader_values1.x = value3 = bottom_left radius
  // v_shader_values1.y = value4 = bottom_right radius
  float blur = v_shader_values.y;
  vec4 radii = vec4(v_shader_values.z, v_shader_values.w, v_shader_values1.x, v_shader_values1.y);

  // The shadow quad is expanded by blur device pixels on each side around the shadow shape.
  // v_dimensions = (shape_width + 2*blur + 1, shape_height + 2*blur + 1)
  // So the original shape half-dims in p-space = v_dimensions - 2*blur.
  vec2 half_shape = v_dimensions - vec2(2.0 * blur, 2.0 * blur);
  vec2 p = v_coordinates * v_dimensions;

  float sdf = sdRoundedRectangle4(p, half_shape, radii);

  // Full alpha inside/at shape edge, fades to 0 at blur device pixels outside.
  gl_FragColor.a *= smoothed(2.0 * blur, 0.0, sdf);
}
