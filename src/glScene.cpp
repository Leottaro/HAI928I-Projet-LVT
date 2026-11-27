#include "glScene.hpp"

#include "Grid.hpp"

constexpr const char *VERTEX_SHADER_FILE = "./shaders/viewport.vert";
constexpr const char *FRAGMENT_SHADER_FILE = "./shaders/ray_traversal.frag";

GLScene::GLScene(QWidget *parent) : QOpenGLWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_core = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;

    m_update_timer.setInterval(0);
    m_update_timer.setTimerType(Qt::PreciseTimer);
    m_update_timer.setSingleShot(false);
    connect(&m_update_timer, &QTimer::timeout, this, &GLScene::updateObjects);

    m_update_timer.start();
    m_update_clock.restart();
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

void GLScene::updateObjects() {
    const float delta_time = static_cast<float>(m_update_clock.nsecsElapsed()) * 1e-9f;
    m_update_clock.restart();

    if (m_camera.m_type == Camera::ControlType::FreeCam) {
        m_camera.onKeysHeld(m_pressed_keys, delta_time);
        emit camPosChanged(m_camera.m_cam_pos);
    }

    if (m_pressed_keys.contains(Qt::Key::Key_R)) {
        recompileShaders();
    }
    m_grid.update(delta_time);
    m_grid_updated = true;

    update();
}

void GLScene::initializeGL() {
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();

    m_cleanup_connection = connect(QOpenGLContext::currentContext(), &QOpenGLContext::aboutToBeDestroyed, this, &GLScene::cleanupGL);

    f->initializeOpenGLFunctions();
    f->glClearColor(0, 0, 0, 1);

    // A core profile refuses to draw without a VAO bound, even when the vertices come from gl_VertexID.
    m_vao.create();

    if (!recompileShaders())
        close();

    // Grid
    f->glActiveTexture(GL_TEXTURE0);
    f->glGenTextures(1, &m_grid_texture);
    f->glBindTexture(GL_TEXTURE_3D, m_grid_texture);
    f->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    resizeGrid(m_grid.getSize().x, m_grid.getSize().y, m_grid.getSize().z);

    m_program->release();
}

void GLScene::paintGL() {
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();

    if (m_grid_updated) {
        f->glBindTexture(GL_TEXTURE_3D, m_grid_texture);
        f->glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, m_grid.getSize().x, m_grid.getSize().y, m_grid.getSize().z, 0, GL_RED, GL_FLOAT, m_grid.data().data());
        m_grid_updated = false;
    }

    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program->bind();
    QOpenGLVertexArrayObject::Binder vao(&m_vao);

    // Display
    f->glUniform1ui(m_program->uniformLocation("n_samples"), m_samples);
    m_program->setUniformValue("scattering_coefs", m_scattering_coefs);
    m_program->setUniformValue("sun_pos", m_sun_pos);

    // Camera
    m_program->setUniformValue("near_plane", m_camera.m_near_far.x());
    m_program->setUniformValue("cam_pos", m_camera.getCamPos());
    m_program->setUniformValue("viewproj_inverse", (m_camera.getProjection() * m_camera.getView()).inverted());

    // Grid
    f->glUniform3ui(m_program->uniformLocation("grid_size"), m_grid.getSize()[0], m_grid.getSize()[1], m_grid.getSize()[2]);
    m_program->setUniformValue("grid_pos", m_grid.getPos());
    m_program->setUniformValue("grid_cell_dims", m_grid.getCellDims());

    // gl_FragCoord is in framebuffer pixels, which differ from widget pixels on HiDPI screens.
    const QSizeF framebuffer = size() * devicePixelRatioF();
    m_program->setUniformValue("inv_screen", QVector2D{static_cast<float>(1. / framebuffer.width()), static_cast<float>(1. / framebuffer.height())});

    f->glDrawArrays(GL_TRIANGLES, 0, 6);

    m_program->release();
}

void GLScene::resizeGL(int w, int h) {
    m_camera.setAspectRatio(h > 0 ? static_cast<float>(w) / h : 1.f);
}

void GLScene::cleanupGL() {
    if (m_program == nullptr)
        return;
    makeCurrent();
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();
    m_program.reset(nullptr);
    if (m_grid_texture != 0) {
        f->glDeleteTextures(1, &m_grid_texture);
        m_grid_texture = 0;
    }
    m_vao.destroy();
    doneCurrent();
}

bool GLScene::recompileShaders() {
    m_program.reset(new QOpenGLShaderProgram());
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, VERTEX_SHADER_FILE))
        return false;
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, FRAGMENT_SHADER_FILE))
        return false;
    if (!m_program->link())
        return false;
    return m_program->bind();
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

void GLScene::focusInEvent(QFocusEvent *_event) {
    recompileShaders();
    QOpenGLWidget::focusInEvent(_event);
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pos = _event->position();
#else
    QPointF pos = _event->localPos();
#endif
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
void GLScene::setSunPos(const QVector3D &_sun_pos) {
    m_sun_pos = _sun_pos;
    emit sunPosChanged(_sun_pos);
    update();
}
void GLScene::setSunPosAxis(uint8_t axis, double v) {
    m_sun_pos[axis] = v;
    emit camPosChanged(m_sun_pos);
    update();
}
void GLScene::setScatteringCoefs(const QVector3D &_scattering_coefs) {
    m_scattering_coefs = _scattering_coefs;
    emit scatteringCoefsChanged(_scattering_coefs);
    update();
}
void GLScene::setScatteringCoefsAxis(uint8_t axis, double v) {
    m_scattering_coefs[axis] = v;
    emit camPosChanged(m_scattering_coefs);
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
void GLScene::resizeGrid(uint x, uint y, uint z) {
    m_grid = FluidGrid({x, y, z});
    m_grid.forEachCell([](size_t, const glm::uvec3 &, float &_density) { _density = 1.f; });
    m_grid_updated = true;
    // m_grid.forEachCell([x,y,z](size_t, const glm::uvec3 &_pos, float &_density) { 
    //     float base_density = static_cast<float>((_pos.y * z + _pos.z) * _pos.z + _pos.x) / (x * y * z);
    //     float random_factor = static_cast<float>(rand()) / RAND_MAX;
    //     _density = std::clamp(base_density + 0.25f * (random_factor - 0.5f), 0.f, 1.f);
    //  });
}