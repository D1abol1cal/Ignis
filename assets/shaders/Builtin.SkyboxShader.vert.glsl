#version 450

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
    mat4 view;
} global_ubo;

layout(location = 0) out vec3 out_tex_coord;

void main() {
    out_tex_coord = in_position;
    mat4 view_no_translation = mat4(mat3(global_ubo.view));
    vec4 pos = global_ubo.projection * view_no_translation * vec4(in_position, 1.0);
    gl_Position = pos.xyww;
}
