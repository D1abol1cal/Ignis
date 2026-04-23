#version 450

layout(location = 0) out vec4 out_colour;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
    mat4 view;
    vec4 ambient_colour;
    vec3 view_position;
    int mode;
    float time;
} global_ubo;

layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_colour;
    float shininess;
} object_ubo;

struct directional_light {
    vec3 direction;
    vec4 colour;
};

struct point_light {
    vec3 position;
    vec4 colour;
    // Usually 1, make sure denominator never gets smaller than 1
    float constant_f;
    // Reduces light intensity linearly
    float linear;
    // Makes the light fall off slower at longer distances.
    float quadratic;
};

// TODO: feed in from cpu
directional_light dir_light = {
    vec3(-0.57735, -0.57735, -0.57735),
    //vec4(0.6, 0.6, 0.6, 1.0)
    vec4(0.4, 0.4, 0.2, 1.0)
};

// TODO: feed in from cpu
point_light p_light_0 = {
    vec3(-5.5, 0.0, -5.5),
    vec4(0.0, 1.0, 0.0, 1.0),
    1.0, // constant_f
    0.35, // Linear
    0.44  // Quadratic
};

// TODO: feed in from cpu
point_light p_light_1 = {
    vec3(5.5, 0.0, -5.5),
    vec4(1.0, 0.0, 0.0, 1.0),
    1.0, // constant_f
    0.35, // Linear
    0.44  // Quadratic
};

// Samplers, diffuse, spec
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

layout(location = 0) flat in int in_mode;
layout(location = 9) flat in uint in_highlight;
// Data Transfer Object
layout(location = 1) in struct dto {
    vec4 ambient;
	vec2 tex_coord;
	vec3 normal;
	vec3 view_position;
	vec3 frag_position;
    vec4 colour;
	vec3 tangent;
} in_dto;

mat3 TBN;

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction);
vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction);

void main() {
    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent;
    tangent = (tangent - dot(tangent, normal) *  normal);
    vec3 bitangent = cross(in_dto.normal, in_dto.tangent);
    TBN = mat3(tangent, bitangent, normal);

    // Update the normal to use a sample from the normal map.
    vec3 localNormal = 2.0 * texture(samplers[SAMP_NORMAL], in_dto.tex_coord).rgb - 1.0;
    normal = normalize(TBN * localNormal);

    if(in_mode == 0 || in_mode == 1) {
        vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);

        out_colour = calculate_directional_light(dir_light, normal, view_direction);

        out_colour += calculate_point_light(p_light_0, normal, in_dto.frag_position, view_direction);
        out_colour += calculate_point_light(p_light_1, normal, in_dto.frag_position, view_direction);
    } else if(in_mode == 2) {
        out_colour = vec4(abs(normal), 1.0);
    } else {
        out_colour = vec4(0.0, 0.0, 0.0, 1.0);
    }

    // Observer mode: in-frustum cubes → solid bright blue.
    if (in_highlight == 2) {
        out_colour = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    // Observer mode: camera marker arrow → bright orange.
    if (in_highlight == 3) {
        out_colour = vec4(1.0, 0.55, 0.05, 1.0);
        return;
    }

    // Observer mode: out-of-frustum objects → dark silhouette.
    if (in_highlight == 4) {
        out_colour = vec4(0.08, 0.08, 0.10, 1.0);
        return;
    }

    // Doom Eternal glory-kill glow: pulsing red rim + energy scan lines.
    // Avoids body tinting so it works on any texture colour.
    if(in_highlight != 0) {
        vec3 view_dir = normalize(in_dto.view_position - in_dto.frag_position);
        float ndotv = max(dot(normal, view_dir), 0.0);

        // Multi-layer Fresnel
        float rim_sharp = pow(1.0 - ndotv, 5.0);   // razor silhouette edge
        float rim_wide  = pow(1.0 - ndotv, 2.2);   // mid aura
        float rim_body  = pow(1.0 - ndotv, 1.0);   // very wide, for subtle darkening

        // Organic double-beat pulse
        float pulse = 0.55 + 0.3 * sin(global_ubo.time * 6.0) + 0.15 * sin(global_ubo.time * 12.5);

        // Energy scan line sweeping upward through world space
        float scan_pos  = fract(in_dto.frag_position.y * 0.35 - global_ubo.time * 0.9);
        float scan      = smoothstep(0.0, 0.04, scan_pos) * (1.0 - smoothstep(0.07, 0.14, scan_pos));

        // Secondary thin faster scan line
        float scan2     = fract(in_dto.frag_position.y * 0.8  - global_ubo.time * 2.2);
        float thin_scan = smoothstep(0.0, 0.01, scan2) * (1.0 - smoothstep(0.025, 0.04, scan2));

        // Colour palette: Argent D'Nur teal — the energy of Doom's ancient realm
        vec3 aura_col = vec3(0.0,  0.5,  0.6);   // deep teal body aura
        vec3 rim_col  = vec3(0.05, 0.85, 0.9);   // bright cyan rim
        vec3 edge_col = vec3(0.55, 1.0,  1.0);   // white-cyan silhouette flash
        vec3 scan_col = vec3(0.1,  0.9,  0.85);  // pure Argent teal scan line

        // Slightly darken non-edge areas so the rim pops
        out_colour.rgb *= 1.0 - rim_body * 0.2;

        // Wide aura — breathes with the pulse (low intensity, no colour shift)
        out_colour.rgb += aura_col * rim_wide * 0.28 * pulse;

        // Bright cyan rim at silhouette
        out_colour.rgb += rim_col * rim_sharp * 0.7 * pulse;

        // Hot bright edge — always on (gives the "outlined" feel)
        out_colour.rgb += edge_col * pow(rim_sharp, 1.2) * 0.85;

        // Energy scan lines
        out_colour.rgb += scan_col * scan      * 0.4 * (0.3 + 0.7 * rim_wide);
        out_colour.rgb += scan_col * thin_scan * 0.25;
    }
}

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction) {
    float diffuse_factor = max(dot(normal, -light.direction), 0.0);

    vec3 half_direction = normalize(view_direction - light.direction);
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), object_ubo.shininess);

    vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
    vec4 ambient = vec4(vec3(in_dto.ambient * object_ubo.diffuse_colour), diff_samp.a);
    vec4 diffuse = vec4(vec3(light.colour * diffuse_factor), diff_samp.a);
    vec4 specular = vec4(vec3(light.colour * specular_factor), diff_samp.a);
    
    if(in_mode == 0) {
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    return (ambient + diffuse + specular);
}

vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction) {
    vec3 light_direction =  normalize(light.position - frag_position);
    float diff = max(dot(normal, light_direction), 0.0);

    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), object_ubo.shininess);

    // Calculate attenuation, or light falloff over distance.
    float distance = length(light.position - frag_position);
    float attenuation = 1.0 / (light.constant_f + light.linear * distance + light.quadratic * (distance * distance));

    vec4 ambient = in_dto.ambient;
    vec4 diffuse = light.colour * diff;
    vec4 specular = light.colour * spec;
    
    if(in_mode == 0) {
        vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}