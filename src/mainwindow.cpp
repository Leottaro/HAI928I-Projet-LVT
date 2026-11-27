#include "mainWindow.hpp"

#include "Camera.hpp"
#include "glScene.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <qpushbutton.h>

#include <QAction>
#include <QActionGroup>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSpinBox>
#include <QVector3D>
#include <QWidget>
#include <QWidgetAction>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Water voxel wowow");
    m_scene = new GLScene(this);
    m_scene->setFocusPolicy(Qt::StrongFocus);
    m_scene->setFocus();

    buildRenderMenu();
    buildCameraMenu();
    buildGridMenu();

    setCentralWidget(m_scene);
}

void MainWindow::buildRenderMenu() {
    QMenu* render_menu = menuBar()->addMenu(tr("&Rendering"));

    // QFormLayout* form_layout = new QFormLayout(render_menu);
    render_menu->addSection(tr("&Samples per pixel"));
    QWidget* samples_widget = new QWidget(render_menu);
    QFormLayout* samples_layout = new QFormLayout(samples_widget);
    QSpinBox* samples = new QSpinBox(render_menu);
    samples->setValue(m_scene->getSamples());
    samples->setRange(1, 16);
    samples->setKeyboardTracking(false);
    connect(samples, qOverload<int>(&QSpinBox::valueChanged), m_scene, &GLScene::setSamples);
    connect(m_scene, &GLScene::samplesChanged, samples, &QSpinBox::setValue);
    samples_layout->addRow(tr("&samples:"), samples);
    auto* samples_action = new QWidgetAction(render_menu);
    samples_action->setDefaultWidget(samples_widget);
    render_menu->addAction(samples_action);

    render_menu->addSection(tr("&Sun Pos"));

    QWidget* sun_pos_widget = new QWidget(render_menu);
    QHBoxLayout* sun_pos_layout = new QHBoxLayout(sun_pos_widget);
    std::array<QDoubleSpinBox*, 3> sun_pos_axis{};
    for (uint8_t axis = 0; axis < 3; axis++) {
        sun_pos_axis[axis] = new QDoubleSpinBox(sun_pos_widget);
        sun_pos_axis[axis]->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
        sun_pos_axis[axis]->setSingleStep(1.e-1);
        sun_pos_axis[axis]->setValue(m_scene->getSunPos()[axis]);
        connect(sun_pos_axis[axis], QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, axis](double v) { m_scene->setSunPosAxis(axis, v); });
        sun_pos_layout->addWidget(sun_pos_axis[axis]);
    }
    connect(m_scene, &GLScene::sunPosChanged, this, [sun_pos_axis](const QVector3D& new_sun_pos) {
        for (uint8_t axis = 0; axis < 3; axis++) {
            sun_pos_axis[axis]->blockSignals(true);
            sun_pos_axis[axis]->setValue(new_sun_pos[axis]);
            sun_pos_axis[axis]->blockSignals(false);
        }
    });
    auto* sun_pos_action = new QWidgetAction(render_menu);
    sun_pos_action->setDefaultWidget(sun_pos_widget);
    render_menu->addAction(sun_pos_action);

    render_menu->addSection(tr("&Fluid Scattering Coefs"));

    QWidget* scattering_widget = new QWidget(render_menu);
    QHBoxLayout* scattering_layout = new QHBoxLayout(scattering_widget);
    std::array<QDoubleSpinBox*, 3> scattering_axis{};
    for (uint8_t axis = 0; axis < 3; axis++) {
        scattering_axis[axis] = new QDoubleSpinBox(scattering_widget);
        scattering_axis[axis]->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
        scattering_axis[axis]->setSingleStep(1.e-1);
        scattering_axis[axis]->setValue(m_scene->getScatteringCoefs()[axis]);
        connect(scattering_axis[axis], QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, axis](double v) { m_scene->setScatteringCoefsAxis(axis, v); });
        scattering_layout->addWidget(scattering_axis[axis]);
    }
    connect(m_scene, &GLScene::scatteringCoefsChanged, this, [scattering_axis](const QVector3D& new_scattering) {
        for (uint8_t axis = 0; axis < 3; axis++) {
            scattering_axis[axis]->blockSignals(true);
            scattering_axis[axis]->setValue(new_scattering[axis]);
            scattering_axis[axis]->blockSignals(false);
        }
    });
    auto* scattering_action = new QWidgetAction(render_menu);
    scattering_action->setDefaultWidget(scattering_widget);
    render_menu->addAction(scattering_action);
}

void MainWindow::buildCameraMenu() {
    QMenu* cam_menu = menuBar()->addMenu(tr("&Camera"));

    cam_menu->addSection(tr("&Type"));

    QActionGroup* type_group = new QActionGroup(cam_menu);
    type_group->setExclusive(true);
    std::array<QAction*, Camera::NB_CONTROL_TYPES> type_actions{};
    for (uint8_t i = 0; i < Camera::NB_CONTROL_TYPES; i++) {
        type_actions[i] = new QAction(type_group);
        type_actions[i]->setCheckable(true);
        type_actions[i]->setChecked(static_cast<uint8_t>(m_scene->getCamera().m_type) == i);
        type_group->addAction(type_actions[i]);
        connect(type_actions[i], &QAction::triggered, this, [this, i]() { m_scene->setCamType(static_cast<Camera::ControlType>(i)); });
        type_actions[i]->setText(tr(Camera::CONTROL_TYPES_STR[i]));
    }
    cam_menu->addActions(type_group->actions());

    QAction* cycle = new QAction(tr("&Cycle type"));
    cycle->setShortcut(QKeySequence(Qt::Key_C));
    connect(cycle, &QAction::triggered, m_scene, &GLScene::cycleCamType);
    cam_menu->addAction(cycle);

    cam_menu->addSection(tr("&Position"));

    QWidget* pos_widget = new QWidget(cam_menu);
    pos_widget->setDisabled(m_scene->getCamera().m_type != Camera::ControlType::FreeCam);
    QHBoxLayout* pos_layout = new QHBoxLayout(pos_widget);
    std::array<QDoubleSpinBox*, 3> pos_axis{};
    for (uint8_t axis = 0; axis < 3; axis++) {
        pos_axis[axis] = new QDoubleSpinBox(pos_widget);
        pos_axis[axis]->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
        pos_axis[axis]->setSingleStep(1.e-2);
        pos_axis[axis]->setValue(m_scene->getCamera().m_cam_pos[axis]);
        connect(pos_axis[axis], QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, axis](double v) { m_scene->setCamPosAxis(axis, v); });
        pos_layout->addWidget(pos_axis[axis]);
    }
    connect(m_scene, &GLScene::camPosChanged, this, [pos_axis](const QVector3D& new_pos) {
        for (uint8_t axis = 0; axis < 3; axis++) {
            pos_axis[axis]->blockSignals(true);
            pos_axis[axis]->setValue(new_pos[axis]);
            pos_axis[axis]->blockSignals(false);
        }
    });
    auto* pos_action = new QWidgetAction(cam_menu);
    pos_action->setDefaultWidget(pos_widget);
    cam_menu->addAction(pos_action);

    cam_menu->addSection(tr("&Fov"));
    QWidget* fov_widget = new QWidget(cam_menu);
    QFormLayout* fov_layout = new QFormLayout(fov_widget);
    QDoubleSpinBox* fov = new QDoubleSpinBox(cam_menu);
    fov->setDecimals(2);
    fov->setSuffix("°");
    fov->setSingleStep(1.e-1);
    fov->setRange(1., 179.);
    fov->setValue(glm::degrees(m_scene->getCamera().m_fovy));
    connect(fov, QOverload<double>::of(&QDoubleSpinBox::valueChanged), m_scene, &GLScene::setCamFovyDegrees);
    connect(m_scene, &GLScene::camFovyChanged, this, [fov](float _fovy) {
        fov->blockSignals(true);
        fov->setValue(glm::degrees(_fovy));
        fov->blockSignals(false);
    });
    fov_layout->addRow(tr("&Fov:"), fov);
    auto* fov_action = new QWidgetAction(cam_menu);
    fov_action->setDefaultWidget(fov_widget);
    cam_menu->addAction(fov_action);

    cam_menu->addSection(tr("&Near/Far"));
    // TODO:

    cam_menu->addSection(tr("&Rotation Speed"));
    // TODO:

    cam_menu->addSection(tr("&Fov"));
    QWidget* mv_speed_widget = new QWidget(cam_menu);
    QFormLayout* mv_speed_layout = new QFormLayout(mv_speed_widget);
    QDoubleSpinBox* mv_speed = new QDoubleSpinBox(cam_menu);
    mv_speed->setDecimals(2);
    mv_speed->setSingleStep(1.);
    mv_speed->setValue(m_scene->getCamera().m_free_cam_speed);
    connect(mv_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), m_scene, &GLScene::setCamFreeCamSpeed);
    connect(m_scene, &GLScene::camFreeSpeedChanged, this, [mv_speed](float _speed) {
        mv_speed->blockSignals(true);
        mv_speed->setValue(_speed);
        mv_speed->blockSignals(false);
    });
    mv_speed_layout->addRow(tr("&moving speed:"), mv_speed);
    auto* mv_speed_action = new QWidgetAction(cam_menu);
    mv_speed_action->setDefaultWidget(mv_speed_widget);
    cam_menu->addAction(mv_speed_action);

    cam_menu->addSection(tr("&Distance To Center"));
    // TODO:

    cam_menu->addSection(tr("&Zoom Rate"));
    // TODO:

    cam_menu->addSection(tr("&Zoom Range"));
    // TODO:

    cam_menu->addSection(tr("&Elasticity"));
    // TODO:

    // enable / disable things depending on the type
    connect(m_scene, &GLScene::camTypeChanged, this, [type_actions, pos_widget](Camera::ControlType type) {
        pos_widget->setDisabled(type != Camera::ControlType::FreeCam);
        for (uint8_t i = 0; i < Camera::NB_CONTROL_TYPES; i++) {
            type_actions[i]->blockSignals(true);
            type_actions[i]->setChecked(i == static_cast<uint8_t>(type));
            type_actions[i]->blockSignals(false);
        }
    });
}

void MainWindow::buildGridMenu() {
    QMenu* grid_menu = menuBar()->addMenu(tr("&Grid"));

    grid_menu->addSection(tr("&Size"));

    QWidget* size_coords_widget = new QWidget(grid_menu);
    QHBoxLayout* size_coords_layout = new QHBoxLayout(size_coords_widget);
    std::array<QSpinBox*, 3> size_axis{};
    for (uint8_t axis = 0; axis < 3; axis++) {
        size_axis[axis] = new QSpinBox(size_coords_widget);
        size_axis[axis]->setRange(1, std::numeric_limits<int>::max());
        size_axis[axis]->setValue(m_scene->getGrid().getSize()[axis]);
        size_coords_layout->addWidget(size_axis[axis]);
    }
    auto* size_coords_action = new QWidgetAction(grid_menu);
    size_coords_action->setDefaultWidget(size_coords_widget);
    grid_menu->addAction(size_coords_action);

    QWidget* validate_size_widget = new QWidget(grid_menu);
    QFormLayout* validate_size_layout = new QFormLayout(validate_size_widget);
    QPushButton* validate_size_button = new QPushButton("Resize Grid");
    connect(validate_size_button, &QPushButton::pressed, this, [this, size_axis]() { this->m_scene->resizeGrid(size_axis[0]->value(), size_axis[1]->value(), size_axis[2]->value()); });
    validate_size_layout->addRow(tr("&Resize Grid:"), validate_size_button);
    auto* validate_size_action = new QWidgetAction(grid_menu);
    validate_size_action->setDefaultWidget(validate_size_widget);
    grid_menu->addAction(validate_size_action);
}