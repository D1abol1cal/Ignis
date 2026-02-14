#version 450

layout(location = 0) in vec3 in_tex_coord;

// Binding 0 is reserved for instance UBO (padding uniform)
layout(set = 1, binding = 1) uniform samplerCube cubemap_sampler;

layout(location = 0) out vec4 out_colour;

void main() {
    // Flip Y coordinate to correct upside-down images
    vec3 tex_coord = vec3(in_tex_coord.x, -in_tex_coord.y, in_tex_coord.z);
    out_colour = texture(cubemap_sampler, tex_coord);
}
