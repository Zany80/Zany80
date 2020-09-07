#include <stdio.h>
#include "sokol_app.h"

#define STR_(x) #x
#define STR(x) STR_(x)

void init(void) {
    printf("Zany80 version " STR(PROJECT_VERSION));
}

void frame(void) {

}

void deinit(void) {

}

void event(const sapp_event *event) {

}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .width = 640,
        .height = 480,
        .window_title = "Zany80 v" STR(PROJECT_VERSION),
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = deinit,
        .event_cb = event,
    };
}
