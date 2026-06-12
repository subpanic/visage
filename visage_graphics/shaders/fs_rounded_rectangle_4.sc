$input v_coordinates, v_dimensions, v_shader_values, v_shader_values1, v_position, v_gradient_pos, v_gradient_pos2, v_gradient_texture_pos

#include <shader_include.sh>

SAMPLER2D(s_gradient, 0);

void main() {
  gl_FragColor = gradient(s_gradient, v_gradient_texture_pos, v_gradient_pos, v_gradient_pos2, v_position);
  vec4 radii = vec4(v_shader_values.z, v_shader_values.w, v_shader_values1.x, v_shader_values1.y);
  gl_FragColor.a = gl_FragColor.a * roundedRectangle4(v_coordinates, v_dimensions, radii, v_shader_values.x, v_shader_values.y);
}
