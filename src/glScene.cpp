#include "glScene.hpp"

#include <cstdlib>
#include <qnamespace.h>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtGui/QMouseEvent>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtWidgets/QWidget>

GLScene::GLScene(QWidget *parent) : QOpenGLWidget(parent), m_program(nullptr), m_camera() {
    setParent(parent);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_core = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;

    m_camera.onAspectRatioChange(static_cast<float>(width()) / height());
    m_last_frame.start();
    // if (m_transparent) {
    //     QSurfaceFormat fmt = format();
    //     fmt.setAlphaBufferSize(8);
    //     setFormat(fmt);
    // }
}

GLScene::~GLScene() {
    disconnect(m_cleanup_connection);
    cleanupGL();
}

QSize GLScene::minimumSizeHint() const {
    return QSize(50, 50);
}

QSize GLScene::sizeHint() const {
    return QSize(1280, 720);
}

void GLScene::initializeGL() {
    m_cleanup_connection = connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLScene::cleanupGL);

    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 1);

    m_program.reset(new QOpenGLShaderProgram());
    // Compile vertex shader
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "./shaders/viewport.vert"))
        close();

    // Compile fragment shader
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "./shaders/ray_traversal.frag"))
        close();

    // Link shader pipeline
    if (!m_program->link())
        close();

    // Bind shader pipeline for use
    if (!m_program->bind())
        close();

    // Grid
    std::vector<float> truc(32 * 32 * 32);
    for (uint i = 0; i < truc.size(); i++)
        truc[i] = static_cast<float>(rand()) / RAND_MAX;
    glGenBuffers(1, &m_grid_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_grid_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, truc.size() * sizeof(float), truc.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_grid_ssbo);
    // glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, truc.size() * sizeof(float), truc.data());

    m_program->release();
}

void GLScene::paintGL() {
    m_last_frame.restart();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_program->bind();

    glUniform3ui(m_program->uniformLocation("grid_size"), 32, 32, 32);
    m_program->setUniformValue("grid_pos", -0.5f, -0.5f, -0.5f);
    m_program->setUniformValue("grid_cell_dims", 1. / 32.f, 1. / 32.f, 1. / 32.f);

    m_program->setUniformValue("near_plane", m_camera.m_near_far.x());
    m_program->setUniformValue("cam_pos", m_camera.m_cam_pos);
    m_program->setUniformValue("viewproj_inverse", (m_camera.getProjection() * m_camera.getView()).inverted());
    m_program->setUniformValue("n_samples", GLuint(1));
    m_program->setUniformValue("inv_screen", QVector2D{1.f / size().width(), 1.f / size().height()});

    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_program->release();
}

void GLScene::resizeGL(int w, int h) {
    m_camera.onAspectRatioChange(static_cast<float>(w) / h);
}

void GLScene::cleanupGL() {
    if (m_program == nullptr)
        return;
    makeCurrent();
    m_program.reset(nullptr);
    doneCurrent();
}

void GLScene::keyPressEvent(QKeyEvent *_event) {
    if (_event->isAutoRepeat())
        return;
    switch (_event->key()) {
    case Qt::Key::Key_F11:
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
        update();
        break;
    case Qt::Key::Key_C:
        m_camera.CycleType();
        update();
        break;
    default:
        break;
    }
}

void GLScene::wheelEvent(QWheelEvent *_event) {
    m_camera.onWheel(_event);
    update();
}

void GLScene::mousePressEvent(QMouseEvent *_event) {
    if (_event->button() == Qt::LeftButton) {
        // grabMouse();
        setCursor(Qt::BlankCursor);
        m_has_last_mouse_pos = false;
    }
}
void GLScene::mouseReleaseEvent(QMouseEvent *_event) {
    if (_event->button() == Qt::LeftButton) {
        // releaseMouse();
        unsetCursor();
        m_has_last_mouse_pos = false;
    }
}
void GLScene::mouseMoveEvent(QMouseEvent *_event) {
    QPointF pos = _event->position();
    if (m_has_last_mouse_pos && _event->buttons() & Qt::MouseButton::LeftButton) {
        QVector2D delta(pos - m_last_mouse_pos);
        m_camera.onMouseMove(delta, getElapsedSec());
        update();
    }
    m_last_mouse_pos = pos;
    m_has_last_mouse_pos = true;
}
void GLScene::leaveEvent(QEvent *) {
    m_has_last_mouse_pos = false;
}