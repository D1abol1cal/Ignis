#pragma once

#include <defines.h>
#include <game_types.h>
#include <math/math_types.h>
#include <systems/camera_system.h>

// TODO: temp
#include <resources/skybox.h>
#include <resources/ui_text.h>

typedef struct game_state {
    f32 delta_time;
    camera* world_camera;

    u16 width, height;

    frustum camera_frustum;

    // TODO: temp
    skybox sb;

    mesh meshes[120];
    mesh* car_mesh;
    mesh* sponza_mesh;
    b8 models_loaded;

    mesh ui_meshes[10];
    ui_text test_text;
    ui_text test_sys_text;

    // The unique identifier of the currently hovered-over object.
    u32 hovered_object_id;

    // Frustum culling visualisation (F3 / F4)
    b8 observer_mode;           // overhead observer camera is active
    vec3 saved_main_cam_pos;    // main camera state saved on F3 press
    vec3 saved_main_cam_rot;
    f32 main_cam_yaw;           // auto-rotating yaw used for the sweep frustum
    mesh* cam_marker_mesh;         // body of virtual camera indicator
    mesh* cam_nose_mesh;           // small cube at front tip (shows facing direction)
    mesh* frustum_edge_meshes[8];  // 8 thin boxes forming wireframe frustum pyramid
    b8 frustum_frozen;          // F4: pause the auto-rotation mid-sweep
    frustum frozen_frustum;
    // TODO: end temp
} game_state;

struct render_packet;

b8 game_boot(struct game* game_inst);

b8 game_initialize(game* game_inst);

b8 game_update(game* game_inst, f32 delta_time);

b8 game_render(game* game_inst, struct render_packet* packet, f32 delta_time);

void game_on_resize(game* game_inst, u32 width, u32 height);

void game_shutdown(game* game_inst);
