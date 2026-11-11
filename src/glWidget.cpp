#include "glWidget.hpp"

#include "Camera.hpp"

#include <QtCore/QCoreApplication>
#include <QtGui/QMouseEvent>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtWidgets/QWidget>

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent), m_program(nullptr), m_camera() {
    setParent(parent);

    m_core = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;

    m_camera.onAspectRatioChange(static_cast<float>(width()) / height());

    // if (m_transparent) {
    //     QSurfaceFormat fmt = format();
    //     fmt.setAlphaBufferSize(8);
    //     setFormat(fmt);
    // }
}

GLWidget::~GLWidget() {
    disconnect(m_cleanup_connection);
    cleanupGL();
}

QSize GLWidget::minimumSizeHint() const {
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const {
    return QSize(400, 400);
}

void GLWidget::initializeGL() {
    m_cleanup_connection = connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanupGL);

    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 1);

    m_program.reset(new QOpenGLShaderProgram());
    // Compile vertex shader
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "./shaders/viewport.vert"))
        close();

    // Compile fragment shader
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "./shaders/viewport_texture.frag"))
        close();

    // Link shader pipeline
    if (!m_program->link())
        close();

    // Bind shader pipeline for use
    if (!m_program->bind())
        close();

    // TODO: Initialize object GL

    m_program->release();
}

void GLWidget::paintGL() {
    quint64 current_frame = time(nullptr);
    m_last_frame = current_frame;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_program->bind();

    // TODO:

    m_program->release();
}

void GLWidget::resizeGL(int w, int h) {
    m_camera.onAspectRatioChange(static_cast<float>(w) / h);
}

void GLWidget::cleanupGL() {
    if (m_program == nullptr)
        return;
    makeCurrent();
    m_program.reset(nullptr);
    doneCurrent();
}

void GLWidget::keyPressEvent(QKeyEvent *_event) {
    float delta_time = static_cast<float>(time(nullptr) - m_last_frame) * 1.e-4f;
    m_camera.onkeyPress(_event, delta_time);
}
void GLWidget::wheelEvent(QWheelEvent *_event) {
    m_camera.onWheel(_event);
}
void GLWidget::mouseMoveEvent(QMouseEvent *_event) {
    float delta_time = static_cast<float>(time(nullptr) - m_last_frame) * 1.e-4f;
    m_camera.onMouseMove(_event, delta_time);
}