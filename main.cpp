#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <cstring>
#include "external/httplib.h"

#define DEVICE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480

struct Buffer {
    void* start;
    size_t length;
};

// 初始化摄像头并返回文件描述符和 mmap 缓冲区
int init_camera(int& fd, Buffer& buffer) {
    fd = open(DEVICE, O_RDWR);
    if (fd == -1) {
        perror("Failed to open camera device");
        return -1;
    }

    // 设置格式
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT failed");
        return -1;
    }

    // 请求缓冲区
    v4l2_requestbuffers req = {};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS failed");
        return -1;
    }

    // 查询缓冲区
    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        perror("VIDIOC_QUERYBUF failed");
        return -1;
    }

    buffer.length = buf.length;
    buffer.start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer.start == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    return 0;
}

// 捕获一帧图像数据
std::vector<unsigned char> capture_frame(int fd, Buffer& buffer) {
    // 队列缓冲区
    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        perror("VIDIOC_QBUF failed");
        return {};
    }

    // 开始采集
    if (ioctl(fd, VIDIOC_STREAMON, &buf.type) < 0) {
        perror("VIDIOC_STREAMON failed");
        return {};
    }

    // 等待采集完成
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        perror("VIDIOC_DQBUF failed");
        return {};
    }

    std::vector<unsigned char> frame((unsigned char*)buffer.start, (unsigned char*)buffer.start + buf.bytesused);

    // 关闭采集
    if (ioctl(fd, VIDIOC_STREAMOFF, &buf.type) < 0) {
        perror("VIDIOC_STREAMOFF failed");
    }

    return frame;
}

int main() {
    int fd;
    Buffer buffer;

    if (init_camera(fd, buffer) < 0) {
        return 1;
    }

    httplib::Server server;

    server.Get("/mjpeg", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content_provider(
            "multipart/x-mixed-replace; boundary=frame",
            [&](size_t offset, httplib::DataSink &sink) {
                while (true) {
                    auto frame = capture_frame(fd, buffer);
                    if (frame.empty()) {
                        std::cerr << "Empty frame, skip." << std::endl;
                        continue;
                    }

                    std::ostringstream out;
                    out << "--frame\r\n";
                    out << "Content-Type: image/jpeg\r\n\r\n";

                    sink.write(out.str().c_str(), out.str().size());
                    sink.write(reinterpret_cast<const char*>(frame.data()), frame.size());
                    sink.write("\r\n", 2);

                    usleep(33000); // 约 30fps
                }
                return true;
            }
        );
    });

    std::cout << "MJPEG streaming at http://localhost:8080/mjpeg" << std::endl;
    server.listen("0.0.0.0", 8080);

    close(fd);
    munmap(buffer.start, buffer.length);
    return 0;
}
