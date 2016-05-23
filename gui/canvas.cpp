//
// Created by 孙万捷 on 16/3/4.
//

#include "canvas.h"

Canvas::Canvas(const QGLFormat &format, QWidget *parent) : QGLWidget(format, parent)
{
    volumeReader.Read("../zx.mha");
    volumeReader.CreateDeviceVolume(&deviceVolume);
    setup_volume(deviceVolume);

    ZoomToExtent();
    camera.Setup(glm::vec3(0.f, 0.f, eyeDist), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), fov, WIDTH, HEIGHT);
    viewMat = glm::lookAt(glm::vec3(0.f, 0.f, eyeDist), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    UpdateCamera();

    // render params
    renderParams.SetupHDRBuffer(WIDTH, HEIGHT);
    renderParams.exposure = 3.f;
    renderParams.traceDepth = 2.f;
}

Canvas::~Canvas()
{
    renderParams.Clear();
}

void Canvas::initializeGL()
{
    makeCurrent();

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, WIDTH * HEIGHT * 4, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    
    checkCudaErrors(cudaGraphicsGLRegisterBuffer(&resource, pbo, cudaGraphicsMapFlagsNone));

    this->startTimer(0);
}

void Canvas::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void Canvas::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    size_t size;
    checkCudaErrors(cudaGraphicsMapResources(1, &resource, 0));
    checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void**)&img, &size, resource));

    rendering(img, renderParams);
    checkCudaErrors(cudaDeviceSynchronize());

    checkCudaErrors(cudaGraphicsUnmapResources(1, &resource, 0));

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    renderParams.frameNo++;
}

void Canvas::mousePressEvent(QMouseEvent *e)
{
    if((e->buttons() & Qt::LeftButton) || (e->buttons() & Qt::MidButton))
    {
        mouseStartPoint = PixelPosToViewPos(e->posF());
        e->accept();
    }
    e->ignore();
}

void Canvas::mouseReleaseEvent(QMouseEvent *e)
{
    e->ignore();
}

void Canvas::mouseMoveEvent(QMouseEvent *e)
{
    QPointF delta = PixelPosToViewPos(e->posF()) - mouseStartPoint;

    // rotation
    if(e->buttons() & Qt::LeftButton)
    {
        constexpr float baseDegree = 50.f;
        viewMat = glm::rotate(viewMat, static_cast<float>(glm::radians(delta.y() * baseDegree)), glm::vec3(1.f, 0.f, 0.f));
        viewMat = glm::rotate(viewMat, static_cast<float>(glm::radians(-delta.x() * baseDegree)), glm::vec3(0.f, 1.f, 0.f));

        UpdateCamera();
        updateGL();
        e->accept();
    }

    //todo: need fix
    // translation
    if(e->buttons() & Qt::MidButton)
    {
        constexpr float baseTranslate = 5.f;
        cameraTranslate.x += static_cast<float>(delta.x() * baseTranslate);
        cameraTranslate.y += static_cast<float>(delta.y() * baseTranslate);

        UpdateCamera();
        updateGL();
        e->accept();
    }

    mouseStartPoint = PixelPosToViewPos(e->posF());

    ReStartRender();
    e->ignore();
}

//todo: implement it
void Canvas::wheelEvent(QWheelEvent *e)
{

    ReStartRender();
    e->accept();
}

void Canvas::UpdateCamera()
{
    auto u = glm::vec3(viewMat[0][0], viewMat[0][1], viewMat[0][2]);
    auto v = glm::vec3(viewMat[1][0], viewMat[1][1], viewMat[1][2]);
    auto w = glm::vec3(viewMat[2][0], viewMat[2][1], viewMat[2][2]);
    auto pos = w * eyeDist - u * cameraTranslate.x - v * cameraTranslate.y;
    camera.Setup(pos, u, v, w, 45.f, WIDTH, HEIGHT);

    setup_camera(camera);
}

//todo: need fix
void Canvas::ZoomToExtent()
{
    glm::vec3 extent = volumeReader.GetVolumeSize();
    auto maxSpan = fmaxf(extent.x, fmaxf(extent.y, extent.z));
    maxSpan *= 1.5f;       // enlarge it slightly
    eyeDist = maxSpan / (2 * tan(glm::radians(fov * 0.5f)));
}