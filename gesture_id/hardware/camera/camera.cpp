#include <hardware/camera/camera.h>

namespace GestureId::Hardware::Camera {
Camera::Camera() {
    ConfigureManager();
    AcquireCamera();
    ConfigureCamera();
}

Camera::~Camera() {
    StopCamera();
    Stop();
}

void Camera::StartCamera() {
    if (cameraState == CameraState::CONFIGURED) {
        camera->start();
    }
}

void Camera::StopCamera() {
    if (cameraState == CameraState::CONFIGURED) { camera->stop(); }
    if (camera) {
        camera->requestCompleted.disconnect();
        requests.clear();
    }
    if (allocator) {
        for (libcamera::StreamConfiguration& cfg : *config) {
            allocator->free(cfg.stream());
        }
        allocator.release();
    }
    if (cameraState != CameraState::AVAILABLE) {
        camera->release();
        camera.reset();
        cameraState = CameraState::AVAILABLE;
    }
}

// TODO: Revisit data movement in this method
void Camera::Run() {
    StartCamera();
    {
        std::unique_lock lock{frameWaitMutex};
        QueueFrameRequest();
        
        if (frameWaitCV.wait_for(lock, std::chrono::milliseconds{1000}) == std::cv_status::timeout) {
            return;
        }
    }
    processFrameFunctionPointer();
}

bool Camera::ConfigureManager() {
    manager = std::make_unique<libcamera::CameraManager>();
    manager->start();
    if (manager->cameras().empty()) { return false; }
    return true;
}

bool Camera::AcquireCamera() {
    if (cameraState == CameraState::AVAILABLE) {
    if (manager->cameras().empty()) { return false; }
    auto cameraID = manager->cameras()[0]->id();
    camera = manager->get(cameraID);
    if (camera->acquire()) { return false; }
        else { cameraState = CameraState::ACQUIRED; }
    }
    return true;
}

bool Camera::ConfigureCamera() {
    if (cameraState == CameraState::ACQUIRED) {
        config = camera->generateConfiguration({libcamera::StreamRole::VideoRecording});
        config->at(0).size.width = 480;
        config->at(0).size.height = 480;
        config->at(0).pixelFormat = libcamera::formats::BGR888;
        config->validate();
        camera->configure(config.get());
        cameraState = CameraState::CONFIGURED;
    }
    return SetupFrameBuffer();
}

bool Camera::SetupFrameBuffer() {
    if (allocator) { return true; }
    allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
    for (const libcamera::StreamConfiguration& cfg: *config) {
        if (allocator->allocate(cfg.stream()) < 0) { return false; }
        size_t allocated = allocator->buffers(cfg.stream()).size();
    }
    libcamera::Stream* stream = config->at(0).stream();
    const std::vector<std::unique_ptr<libcamera::FrameBuffer>>& buffers = allocator->buffers(stream);
    if (buffers.empty()) { return false; }
    for (uint16_t i = 0; i < buffers.size(); i++) {
        std::unique_ptr<libcamera::Request> request = camera->createRequest();
        if (!request) { return false; }
        const std::unique_ptr<libcamera::FrameBuffer>& buffer = buffers[i];
        if (request->addBuffer(stream, buffer.get()) < 0) { return false; }
        requests.push_back(std::move(request));
    }
    camera->requestCompleted.connect(this, &Camera::RequestCallback);
    return true;
}

void Camera::RequestCallback(libcamera::Request* request) {
    if (request->status() == libcamera::Request::RequestCancelled) {
        frameWaitCV.notify_all();
        return;
    }
    processFrameFunctionPointer = std::bind(&Camera::ProcessFrame, this, request);
    frameWaitCV.notify_all(); 
}

void Camera::QueueFrameRequest() {
    for (std::unique_ptr<libcamera::Request>& request : requests) {
        camera->queueRequest(request.get());
    }
}

// TODO: Implement actual logic in this function
void Camera::ProcessFrame(libcamera::Request* request) {
    const libcamera::Request::BufferMap& buffers = request->buffers();
    for (auto [stream, buffer] : buffers) {
        const libcamera::FrameMetadata& metadata = buffer->metadata();
    }
    camera->stop();
}

void Camera::Stop() {
    config.reset();
    manager->stop();
}
} // namespace GestureId::Hardware::Camera