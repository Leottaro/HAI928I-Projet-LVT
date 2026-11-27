#include "glScene.hpp"

#include "Grid.hpp"
#include <qvector3d.h>

QVector3D glm_to_QVector(const glm::vec3 & p_vector) {
    return QVector3D(p_vector.x, p_vector.y, p_vector.z);
}

GLScene::GLScene(QWidget *parent) : QOpenGLWidget(parent), m_grid({4, 4, 4}) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_core = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;

    m_update_timer.setInterval(100);
    m_update_timer.setTimerType(Qt::PreciseTimer);
    m_update_timer.setSingleShot(false);
    connect(&m_update_timer, &QTimer::timeout, this, &GLScene::updateObjects);
    // connect(&m_update_timer, &QTimer::timeout, this, []() { std::cout << "salut" << std::endl; });

    m_update_timer.start();
    m_update_clock.restart();

    // Initialize each cell with random density

    m_grid.setWorldPos(glm::vec3(-1.f, -1.f, -1.f));
    m_grid.setWorldCellSize(2.f / glm::vec3(m_grid.getSize()));
    m_grid.randomizeDensity();
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

    // A core profile refuses to draw without a VAO bound, even when the vertices come from gl_VertexID.
    m_vao.create();

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
    glGenBuffers(1, &m_grid_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_grid_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_grid.getDensity().getDataVector().size() * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_grid_ssbo);

    m_program->release();
}

void GLScene::updateObjects() {
    const float delta_time = static_cast<float>(m_update_clock.nsecsElapsed()) * 1e-9f;

    if (m_camera.m_type != Camera::ControlType::FreeCam) {
        m_camera.onKeysHeld(m_pressed_keys, delta_time);
        emit camPosChanged(m_camera.m_cam_pos);
    }

    m_grid.update(delta_time);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_grid_ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_grid.getDensity().getCellCount() * sizeof(float), m_grid.getDensity().getDataVector().data());

    update();
    m_update_clock.restart();
}

void GLScene::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program->bind();
    QOpenGLVertexArrayObject::Binder vao(&m_vao);

    // Display
    glUniform3ui(m_program->uniformLocation("grid_size"), m_grid.getSize()[0], m_grid.getSize()[1], m_grid.getSize()[2]);
    m_program->setUniformValue("grid_pos", glm_to_QVector(m_grid.getWorldPos()));
    m_program->setUniformValue("grid_cell_dims", glm_to_QVector(m_grid.getWorldCellSize()));
    m_program->setUniformValue("n_samples", m_samples);

    // Camera
    m_program->setUniformValue("near_plane", m_camera.m_near_far.x());
    m_program->setUniformValue("cam_pos", m_camera.getCamPos());
    m_program->setUniformValue("viewproj_inverse", (m_camera.getProjection() * m_camera.getView()).inverted());

    // gl_FragCoord is in framebuffer pixels, which differ from widget pixels on HiDPI screens.
    const QSizeF framebuffer = size() * devicePixelRatioF();
    m_program->setUniformValue("inv_screen", QVector2D{static_cast<float>(1. / framebuffer.width()), static_cast<float>(1. / framebuffer.height())});

    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_program->release();
}

void GLScene::resizeGL(int w, int h) {
    m_camera.setAspectRatio(h > 0 ? static_cast<float>(w) / h : 1.f);
}

void GLScene::cleanupGL() {
    if (m_program == nullptr)
        return;
    makeCurrent();
    m_program.reset(nullptr);
    glDeleteBuffers(1, &m_grid_ssbo);
    m_grid_ssbo = 0;
    m_vao.destroy();
    doneCurrent();
}

void GLScene::keyPressEvent(QKeyEvent *_event) {
    if (_event->isAutoRepeat())
        return;
    m_pressed_keys.insert(_event->key());
}

void GLScene::keyReleaseEvent(QKeyEvent *_event) {
    if (_event->isAutoRepeat())
        return;
    m_pressed_keys.remove(_event->key());
}

void GLScene::focusOutEvent(QFocusEvent *_event) {
    m_pressed_keys.clear();
    QOpenGLWidget::focusOutEvent(_event);
}

void GLScene::wheelEvent(QWheelEvent *_event) {
    m_camera.onWheel(_event);
    emit camPosChanged(m_camera.m_cam_pos);
    update();
}

void GLScene::mousePressEvent(QMouseEvent *_event) {
    if (_event->button() == Qt::LeftButton) {
        setCursor(Qt::BlankCursor);
        m_has_last_mouse_pos = false;
    }
}
void GLScene::mouseReleaseEvent(QMouseEvent *_event) {
    if (_event->button() == Qt::LeftButton) {
        unsetCursor();
        m_has_last_mouse_pos = false;
    }
}
void GLScene::mouseMoveEvent(QMouseEvent *_event) {
    QPointF pos = _event->localPos();
    if (m_has_last_mouse_pos && _event->buttons() & Qt::MouseButton::LeftButton) {
        QVector2D delta(pos - m_last_mouse_pos);
        m_camera.onMouseMove(delta);
        emit camPosChanged(m_camera.m_cam_pos);
        update();
    }
    m_last_mouse_pos = pos;
    m_has_last_mouse_pos = true;
}
void GLScene::leaveEvent(QEvent *) {
    m_has_last_mouse_pos = false;
}

void GLScene::setSamples(int v) {
    m_samples = v;
    emit samplesChanged(m_samples);
    update();
}
void GLScene::setCamType(Camera::ControlType _type) {
    m_camera.setType(_type);
    emit camTypeChanged(m_camera.m_type);
    update();
}
void GLScene::cycleCamType() {
    m_camera.CycleType();
    emit camTypeChanged(m_camera.m_type);
    update();
}
void GLScene::setCamPos(const QVector3D &_cam_pos) {
    m_camera.m_cam_pos = _cam_pos;
    m_camera.refresh();
    emit camPosChanged(m_camera.m_cam_pos);
    update();
}
void GLScene::setCamPosAxis(uint8_t axis, double v) {
    m_camera.m_cam_pos[axis] = v;
    m_camera.refresh();
    emit camPosChanged(m_camera.m_cam_pos);
    update();
}
void GLScene::setCamFovyDegrees(float _fovy) {
    m_camera.m_fovy = glm::radians(_fovy);
    m_camera.refresh();
    emit camFovyChanged(m_camera.m_fovy);
    update();
}
void GLScene::setCamNearFar(const QVector2D &_near_far) {
    m_camera.m_near_far = _near_far;
    m_camera.refresh();
    emit camNearFarChanged(m_camera.m_near_far);
    update();
}
void GLScene::setCamRotationSpeed(float _rotation_speed) {
    m_camera.m_rotation_speed = _rotation_speed;
    m_camera.refresh();
    emit camRotationSpeedChanged(m_camera.m_rotation_speed);
    update();
}
void GLScene::setCamFreeCamSpeed(float _free_cam_speed) {
    m_camera.m_free_cam_speed = _free_cam_speed;
    m_camera.refresh();
    emit camFreeSpeedChanged(m_camera.m_free_cam_speed);
    update();
}
void GLScene::setCamDistanceToCenter(float _distance_to_center) {
    m_camera.m_distance_to_center = _distance_to_center;
    m_camera.refresh();
    emit camDistanceToCenterChanged(m_camera.m_distance_to_center);
    update();
}
void GLScene::setCamZoomRate(float _zoom_rate) {
    m_camera.m_zoom_rate = _zoom_rate;
    m_camera.refresh();
    emit camZoomRateChanged(m_camera.m_zoom_rate);
    update();
}
void GLScene::setCamZoomRange(const QVector2D &_zoom_range) {
    m_camera.m_zoom_range = _zoom_range;
    m_camera.refresh();
    emit camZoomRangeChanged(m_camera.m_zoom_range);
    update();
}
void GLScene::setCamElasticity(float _elasticity) {
    m_camera.m_elasticity = _elasticity;
    m_camera.refresh();
    emit camElasticityChanged(m_camera.m_elasticity);
    update();
}