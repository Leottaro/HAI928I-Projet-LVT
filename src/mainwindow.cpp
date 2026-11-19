#include "mainWindow.hpp"

#include "Camera.hpp"
#include "glScene.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <memory>

#include <QAction>
#include <QActionGroup>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QSpinBox>
#include <QWidget>
#include <QWidgetAction>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Water voxel wowow");
    m_scene = new GLScene(this);
    m_scene->setFocusPolicy(Qt::StrongFocus);
    m_scene->setFocus();

    buildRenderMenu();
    buildCameraMenu();

    setCentralWidget(m_scene);
}

void MainWindow::buildRenderMenu() {
    QMenu* render_menu = menuBar()->addMenu(tr("&Rendering"));
    QFormLayout* form_layout = new QFormLayout(render_menu);

    auto* samples = new QSpinBox(render_menu);
    samples->setRange(1, 16);
    samples->setKeyboardTracking(false);
    connect(samples, qOverload<int>(&QSpinBox::valueChanged), m_scene, &GLScene::setSamples);
    connect(m_scene, &GLScene::samplesChanged, samples, &QSpinBox::setValue);
    form_layout->addRow(tr("&Samples per pixel:"), samples);

    render_menu->setLayout(form_layout);
}

void MainWindow::buildCameraMenu() {
    QMenu* render_menu = menuBar()->addMenu(tr("&Camera"));

    render_menu->addSection(tr("&Type"));

    QActionGroup* type_group = new QActionGroup(render_menu);
    type_group->setExclusive(true);
    std::array<QAction*, Camera::NB_CONTROL_TYPES> type_actions{};
    for (uint8_t i = 0; i < Camera::NB_CONTROL_TYPES; i++) {
        type_actions[i] = new QAction(type_group);
        type_actions[i]->setCheckable(true);
        type_actions[i]->setChecked(static_cast<uint8_t>(m_scene->camera().m_type) == i);
        type_group->addAction(type_actions[i]);
        connect(type_actions[i], &QAction::triggered, this, [this, i]() { m_scene->setCamType(static_cast<Camera::ControlType>(i)); });
        type_actions[i]->setText(tr(Camera::CONTROL_TYPES_STR[i]));
    }
    render_menu->addActions(type_group->actions());

    QAction* cycle = new QAction(tr("&Cycle type"));
    cycle->setShortcut(QKeySequence(Qt::Key_C));
    connect(cycle, &QAction::triggered, m_scene, &GLScene::cycleCamType);
    render_menu->addAction(cycle);

    render_menu->addSection(tr("&Position"));

    QWidget* pos_widget = new QWidget(render_menu);
    pos_widget->setDisabled(m_scene->camera().m_type != Camera::ControlType::FreeCam);
    QHBoxLayout* pos_layout = new QHBoxLayout(pos_widget);
    std::array<QDoubleSpinBox*, 3> pos_axis{};
    for (uint8_t axis = 0; axis < 3; axis++) {
        pos_axis[axis] = new QDoubleSpinBox(pos_widget);
        pos_axis[axis]->setRange(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
        pos_axis[axis]->setSingleStep(1.e-2);
        pos_axis[axis]->setValue(m_scene->camera().m_cam_pos[axis]);
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
    auto* pos_action = new QWidgetAction(render_menu);
    pos_action->setDefaultWidget(pos_widget);
    render_menu->addAction(pos_action);

    render_menu->addSection(tr("&Fov"));
    QWidget* fov_widget = new QWidget(render_menu);
    QFormLayout* fov_layout = new QFormLayout(fov_widget);
    QDoubleSpinBox* fov = new QDoubleSpinBox(render_menu);
    fov->setDecimals(2);
    fov->setSuffix("°");
    fov->setSingleStep(1.e-2);
    fov->setRange(1., 179.);
    fov->setValue(glm::degrees(m_scene->camera().m_fovy));
    connect(fov, QOverload<double>::of(&QDoubleSpinBox::valueChanged), m_scene, &GLScene::setCamFovyDegrees);
    connect(m_scene, &GLScene::camFovyChanged, this, [fov](float _fovy) {
        fov->blockSignals(true);
        fov->setValue(glm::degrees(_fovy));
        fov->blockSignals(false);
    });
    fov_layout->addRow(tr("&Fov:"), fov);
    auto* fov_action = new QWidgetAction(render_menu);
    fov_action->setDefaultWidget(fov_widget);
    render_menu->addAction(fov_action);

    render_menu->addSection(tr("&Near/Far"));
    // TODO:

    render_menu->addSection(tr("&Rotation Speed"));
    // TODO:

    render_menu->addSection(tr("&Fov"));
    QWidget* mv_speed_widget = new QWidget(render_menu);
    QFormLayout* mv_speed_layout = new QFormLayout(mv_speed_widget);
    QDoubleSpinBox* mv_speed = new QDoubleSpinBox(render_menu);
    mv_speed->setDecimals(2);
    mv_speed->setSingleStep(1.e-2);
    mv_speed->setValue(m_scene->camera().m_free_cam_speed);
    connect(mv_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), m_scene, &GLScene::setCamFreeCamSpeed);
    connect(m_scene, &GLScene::camFreeSpeedChanged, this, [mv_speed](float _speed) {
        mv_speed->blockSignals(true);
        mv_speed->setValue(_speed);
        mv_speed->blockSignals(false);
    });
    mv_speed_layout->addRow(tr("&moving speed:"), mv_speed);
    auto* mv_speed_action = new QWidgetAction(render_menu);
    mv_speed_action->setDefaultWidget(mv_speed_widget);
    render_menu->addAction(mv_speed_action);

    render_menu->addSection(tr("&Distance To Center"));
    // TODO:

    render_menu->addSection(tr("&Zoom Rate"));
    // TODO:

    render_menu->addSection(tr("&Zoom Range"));
    // TODO:

    render_menu->addSection(tr("&Elasticity"));
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