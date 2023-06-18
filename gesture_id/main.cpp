#include <iostream>
#include <csignal>

#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer.h>

enum class CameraState {
    AVAILABLE = 0,
    ACQUIRED,
    CONFIGURED,
    RUNNING
};

class Camera {
public:
    Camera() {
        ConfigureManager();
        ConfigureCamera();
        ConfigureBuffer();
        camera->start();
    }

    ~Camera() {
        StopAll();
    }

    void PrintAvailableCameras() {
        std::cout << "" << std::endl;
        std::cout << "Cameras Found: " << manager->cameras().size() << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Names of Available Cameras:" << std::endl;
        for (const std::shared_ptr<libcamera::Camera> cam : manager->cameras()) {
            std::cout << cam->id() << std::endl;
        }
        std::cout << "" << std::endl;
    }

    void StopAll() {
        // Stop LibCamera Objects
        camera->stop();
        camera->release();
        manager->stop();
        // Release Unique PTRs
        camera.reset();
        manager.release();
        allocator.release();
        return;
    }
    
    void Run() {

    }

private:
    static inline void RequestCallback(libcamera::Request* request) {
        std::cout << "Callback" << std::endl;
    }

    int ConfigureManager() {
        // Configure Manager - Must be manually defined before proceeding
        manager = std::make_unique<libcamera::CameraManager>();
        manager->start();
        if (manager->cameras().empty()) { return -1; }
        return 0;
    }

    int ConfigureCamera() {
        // Identify camera from manager and define camera object
        std::string id = manager->cameras()[0]->id();
        camera = manager->get(id);
        camera->acquire();

        // Define the camera configuration
        config = camera->generateConfiguration({libcamera::StreamRole::VideoRecording});
        config->at(0).size.width = 480;
        config->at(0).size.height = 480;
        config->at(0).pixelFormat = libcamera::formats::BGR888;
        config->validate();
        camera->configure(config.get());
        return 0;
    }

    int ConfigureBuffer() {
        // Configure FrameBufferAllocator - Must be manually defined before proceeding
        allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
        std::vector<std::unique_ptr<libcamera::Request>> requests;
        // Allocate buffers for each stream in camera config
        for (const libcamera::StreamConfiguration& cfg: *config) {
            if (allocator->allocate(cfg.stream()) < 0) { return -1; }
            size_t allocated = allocator->buffers(cfg.stream()).size();
        }
        libcamera::Stream* stream = config->at(0).stream();
        const std::vector<std::unique_ptr<libcamera::FrameBuffer>>& buffers = allocator->buffers(stream);
        if (buffers.empty()) { return -1; }
        // Allocate frame request for each stream
        for (uint16_t i = 0; i < buffers.size(); i++) {
            std::unique_ptr<libcamera::Request> request = camera->createRequest();
            if (!request) { return -1; }
            const std::unique_ptr<libcamera::FrameBuffer>& buffer = buffers[i];
            if (request->addBuffer(stream, buffer.get()) < 0) { return -1; }
            requests.push_back(std::move(request));
        }
        camera->requestCompleted.connect(this, &RequestCallback);
        return 0;
    }

private:
    // Define required libcamera objects globally (for example)
    // CameraManager - https://libcamera.org/api-html/classlibcamera_1_1CameraManager.html
    std::unique_ptr<libcamera::CameraManager> manager;
    // CameraConfiguration - https://libcamera.org/api-html/classlibcamera_1_1CameraConfiguration.html
    std::unique_ptr<libcamera::CameraConfiguration> config;
    // Camera - https://libcamera.org/api-html/classlibcamera_1_1Camera.html
    std::shared_ptr<libcamera::Camera> camera;
    // FrameBufferAllocator - https://libcamera.org/api-html/classlibcamera_1_1FrameBufferAllocator.html
    std::unique_ptr<libcamera::FrameBufferAllocator> allocator;
};


int main(int argc, const char** argv) {
    Camera camera;
    camera.PrintAvailableCameras();
    camera.Run();
    return 0;
}