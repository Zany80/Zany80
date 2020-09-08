#include <stdio.h>
#include <stdlib.h>
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_imgui.h"
#include "sokol/sokol_time.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

#define STR_(x) #x
#define STR(x) STR_(x)

uint64_t last_time;

void init(void) {
    printf("Zany80 version " STR(PROJECT_VERSION));
    sg_setup(&(sg_desc){
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
    simgui_setup(&(simgui_desc_t){.sample_count = 2});
    stm_setup();
}

void frame(void) {
    int width = sapp_width();
    int height = sapp_height();
    const sg_pass_action action = (sg_pass_action) { 0 };
    sg_begin_default_pass(&action, width, height);
    double delta = stm_sec(stm_laptime(&last_time));
    simgui_new_frame(width, height, delta);
    simgui_render();
    sg_end_pass();
    sg_commit();
}

void deinit(void) {
    simgui_shutdown();
    sg_shutdown();
}

void event(const sapp_event *event) {
    simgui_handle_event(event);
}

sapp_desc sokol_main(int argc, char **argv) {
    (void)argv;
    (void)argc;
    return (sapp_desc) {
        .width = 640,
        .height = 480,
        .window_title = "Zany80 v" STR(PROJECT_VERSION),
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = deinit,
        .event_cb = event,
        .sample_count = 4,
        .fullscreen = true,
        .alpha = true,
    };
}
