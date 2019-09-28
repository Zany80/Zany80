#include <Zany80/API.h>
#include <emscripten.h>
#include <Zany80/Zany80.h>

void SetupIcon() {
    zany_log(ZL_DEBUG, "Relying on favicon in emsc backend\n");
}

void Zany80::ToggleFullscreen() {
    EM_ASM(
        var elem = document.getElementById("canvas");
        if (elem.requestFullscreen) {
            elem.requestFullscreen();
        } else if (elem.mozRequestFullScreen) { /* Firefox */
            elem.mozRequestFullScreen();
        } else if (elem.webkitRequestFullscreen) { /* Chrome, Safari and Opera */
            elem.webkitRequestFullscreen();
        } else if (elem.msRequestFullscreen) { /* IE/Edge */
            elem.msRequestFullscreen();
        }
    );
}
