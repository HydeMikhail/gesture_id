#include <hardware/camera/camera.h>

int main(int argc, const char** argv) {
    GestureId::Hardware::Camera::Camera camera;
    camera.Run();
    return 0;
}