#include <iostream>
#include <condition_variable>
#include <mutex>

#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer.h>

namespace GestureId::Hardware::Camera {

enum class CameraState {
    AVAILABLE = 0,
    ACQUIRED,
    CONFIGURED,
    RUNNING
};

class Camera {
public:
    Camera();
    ~Camera();
    
    void StartCamera();
    void StopCamera();
    void Run();

private:
    bool ConfigureManager();
    bool AcquireCamera();
    bool ConfigureCamera();
    bool SetupFrameBuffer();
    void RequestCallback(libcamera::Request* request);
    void QueueFrameRequest();
    void ProcessFrame(libcamera::Request* request);
    void Stop();
    
    std::unique_ptr<libcamera::CameraManager> manager;
    std::unique_ptr<libcamera::CameraConfiguration> config;
    std::shared_ptr<libcamera::Camera> camera;
    std::unique_ptr<libcamera::FrameBufferAllocator> allocator;
    std::vector<std::unique_ptr<libcamera::Request>> requests;

    std::function<void()> processFrameFunctionPointer;

    std::condition_variable frameWaitCV;
    std::mutex frameWaitMutex;

    CameraState cameraState = CameraState::AVAILABLE;
};
} // namespace GestureId::Hardware::Camera