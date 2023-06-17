#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer.h>

int main(int argc, const char** argv) {
    // Define and start the LibCamera Camera Manager then
    // check that cameras are detected
    libcamera::CameraManager cameraManager;
    cameraManager.start();
    if (cameraManager.cameras().empty()) { return -1; }

    // Find camera Id of first detected camera and use that to define
    // a pointer to a Camera object
    std::string cameraId = cameraManager.cameras()[0]->id();
    std::shared_ptr<libcamera::Camera> camera = cameraManager.get(cameraId);
    camera->acquire();

    // Generate a default camera configuration and auto populate the
    // minimum requirements for a camera config
    std::unique_ptr<libcamera::CameraConfiguration> config = camera->generateConfiguration(
                                                                     {libcamera::StreamRole::StillCapture}
                                                                     );
    config->at(0).size.width = 480;
    config->at(0).size.height = 480;
    config->at(0).pixelFormat = libcamera::formats::BGR888;
    config->validate();
    camera->configure(config.get());

    // Setup Frame Buffer for storing Camera Responses

    camera->stop();
    camera->release();
    cameraManager.stop();
    return 0;
}