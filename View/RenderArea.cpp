#include "RenderArea.h"

#include <iostream>
#include <assert.h>

#include <QtGui>
#include <QPrintDialog>
#include <QPrinter>
#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#endif
#include <QtSvg/QSvgGenerator>
#include <QFileDialog>

#include "View/MainWindow.h"
#include "Data/ClientManager.h"

#include <qmath.h>

void GraphicsView::wheelEvent(QWheelEvent *e)
{
  if (e->modifiers() & Qt::ControlModifier) {
      if (e->delta() > 0)
          view->zoomIn(6);
      else
          view->zoomOut(6);
      e->accept();
  } else {
      QGraphicsView::wheelEvent(e);
  }
}

bool GraphicsView::viewportEvent(QEvent* event) {
  return QGraphicsView::viewportEvent(event);
}



void GraphicsView::mouseReleaseEvent(QMouseEvent *e)
{
  if (dragMode() == QGraphicsView::RubberBandDrag && e->button() == Qt::LeftButton) {
    end_point_ = mapToScene(e->pos());
    std::cout << "setting end point to: " << end_point_.x() << std::endl;
//    view->marker_.setMarker(checkMaxXVal(start_point_.rx()));
//    view->marker_.setMarker(checkMaxXVal(end_point_.rx()));
    if (end_point_.rx() < start_point_.rx()) {
      QPointF tmp = end_point_;
      end_point_ = start_point_;
      start_point_ = tmp;
    }
    view->start_marker_.setMarker(checkMaxXVal(start_point_.rx()));
    view->end_marker_.setMarker(checkMaxXVal(end_point_.rx()));
    view->selected_area_in_ms_ = view->start_marker_.getTime() - view->end_marker_.getTime();
    emit view->updateSceneSignal();
  }

  return QGraphicsView::mouseReleaseEvent(e);
}

qreal GraphicsView::checkMaxXVal(qreal x)
{
  Procedure* proc = view->getOverallProcedure();
  if (proc) {
    qreal max_x = -proc->points_per_update_ * proc->workload_.size();
    if (max_x > x) {
      x = max_x;
    }
  }
  return x;
}

void GraphicsView::mousePressEvent(QMouseEvent *e)
{
  if(dragMode() == QGraphicsView::RubberBandDrag && e->button() == Qt::LeftButton) {
    start_point_ = mapToScene(e->pos());
    std::cout << "[GraphicsView::mousePressEvent] setting start point "<< start_point_.x() << std::endl;
  }

  return QGraphicsView::mousePressEvent(e);
}

RenderArea::RenderArea(const QString &name, QWidget *parent)
    : QFrame(parent), points_per_update_(3), marker_(points_per_update_),
                                             start_marker_(points_per_update_),
                                             end_marker_(points_per_update_),
                                             selected_area_in_ms_(0)
{
    setFrameStyle(Sunken | StyledPanel);
    graphicsView = new GraphicsView(this);
    graphicsView->setRenderHint(QPainter::Antialiasing, false);
    graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    graphicsView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    QSize iconSize(size, size);

    QToolButton *zoomInIcon = new QToolButton;
    zoomInIcon->setAutoRepeat(true);
    zoomInIcon->setAutoRepeatInterval(33);
    zoomInIcon->setAutoRepeatDelay(0);
    zoomInIcon->setIcon(QPixmap("://images/zoomin.png"));
    zoomInIcon->setIconSize(iconSize);
    QToolButton *zoomOutIcon = new QToolButton;
    zoomOutIcon->setAutoRepeat(true);
    zoomOutIcon->setAutoRepeatInterval(33);
    zoomOutIcon->setAutoRepeatDelay(0);
    zoomOutIcon->setIcon(QPixmap("://images/zoomout.png"));
    zoomOutIcon->setIconSize(iconSize);
    zoomSlider = new QSlider;
    zoomSlider->setMinimum(0);
    zoomSlider->setMaximum(200);
    zoomSlider->setValue(65);
    zoomSlider->setTickPosition(QSlider::TicksRight);

    // Zoom slider layout
    QVBoxLayout *zoomSliderLayout = new QVBoxLayout;
    zoomSliderLayout->addWidget(zoomInIcon);
    zoomSliderLayout->addWidget(zoomSlider);
    zoomSliderLayout->addWidget(zoomOutIcon);

    resetButton = new QToolButton;
    resetButton->setText(tr("0"));
    resetButton->setEnabled(false);

    // Label layout
    QHBoxLayout *labelLayout = new QHBoxLayout;
    label = new QLabel(name);
    label2 = new QLabel(tr("Pointer Mode"));
    selectModeButton = new QToolButton;
    selectModeButton->setText(tr("Set Marker"));
    selectModeButton->setCheckable(true);
    selectModeButton->setChecked(false);
    dragModeButton = new QToolButton;
    dragModeButton->setText(tr("Drag"));
    dragModeButton->setCheckable(true);
    dragModeButton->setChecked(true);
    pauseButton = new QToolButton;
    pauseButton->setText(tr("Pause"));
    pauseButton->setCheckable(true);
    pauseButton->setChecked(false);
//    antialiasButton = new QToolButton;
//    antialiasButton->setText(tr("Antialiasing"));
//    antialiasButton->setCheckable(true);
//    antialiasButton->setChecked(false);
//    openGlButton = new QToolButton;
//    openGlButton->setText(tr("OpenGL"));
//    openGlButton->setCheckable(true);
//#ifndef QT_NO_OPENGL
//    openGlButton->setEnabled(QGLFormat::hasOpenGL());
//#else
//    openGlButton->setEnabled(false);
//#endif
//    printButton = new QToolButton;
//    printButton->setIcon(QIcon(QPixmap(":/fileprint.png")));

    QButtonGroup *pointerModeGroup = new QButtonGroup;
    pointerModeGroup->setExclusive(true);
    pointerModeGroup->addButton(selectModeButton);
    pointerModeGroup->addButton(dragModeButton);

    labelLayout->addWidget(label);
    labelLayout->addStretch();
    labelLayout->addWidget(label2);
    labelLayout->addWidget(selectModeButton);
    labelLayout->addWidget(dragModeButton);
    labelLayout->addStretch();
    labelLayout->addWidget(pauseButton);
//    labelLayout->addWidget(antialiasButton);
//    labelLayout->addWidget(openGlButton);
//    labelLayout->addWidget(printButton);

    QGridLayout *topLayout = new QGridLayout;
    topLayout->addLayout(labelLayout, 0, 0);
    topLayout->addWidget(graphicsView, 1, 0);
    topLayout->addLayout(zoomSliderLayout, 1, 1);
    topLayout->addWidget(resetButton, 2, 1);
    setLayout(topLayout);

//    QTimer* timer = new QTimer();
//    connect(timer, SIGNAL(timeout()), this, SLOT(updateScene()));
//    timer->start(50);

    connect(resetButton, SIGNAL(clicked()), this, SLOT(resetView()));
    connect(zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setupMatrix()));

    connect(graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(setResetButtonEnabled()));
    connect(graphicsView->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(setResetButtonEnabled()));
    connect(selectModeButton, SIGNAL(toggled(bool)), this, SLOT(togglePointerMode()));
    connect(dragModeButton, SIGNAL(toggled(bool)), this, SLOT(togglePointerMode()));
    connect(pauseButton, SIGNAL(toggled(bool)), this, SLOT(togglePause()));
//    connect(antialiasButton, SIGNAL(toggled(bool)), this, SLOT(toggleAntialiasing()));
//    connect(openGlButton, SIGNAL(toggled(bool)), this, SLOT(toggleOpenGL()));
    connect(zoomInIcon, SIGNAL(clicked()), this, SLOT(zoomIn()));
    connect(zoomOutIcon, SIGNAL(clicked()), this, SLOT(zoomOut()));
//    connect(printButton, SIGNAL(clicked()), this, SLOT(print()));

    setupMatrix();
    scene_ = new QGraphicsScene;
    view()->setScene(scene_);
//overall_procedure_ = new Procedure(0xCAFEAFFE, "Overall Usage", false);
    overall_procedure_ = 0;
    connect(this, SIGNAL(updateSceneSignal()), this, SLOT(updateScene()));
    connect(this, SIGNAL(updateSceneCoordinatesSignal()), this, SLOT(updateSceneCoordinates()));

    drawCoordinates();
    togglePointerMode();
    std::cout << "RenderArea::RenderArea" << std::endl;
}

RenderArea::~RenderArea()
{
  {
    boost::unique_lock<boost::shared_mutex> lock(procedures_lock_);
    for (QVector<Procedure*>::iterator it = procedures_.begin();
         it != procedures_.end();
         it++) {
      delete (*it);
    }

    if (overall_procedure_) {
      delete overall_procedure_;
      overall_procedure_ = 0;
    }
  }

  if (scene_) {
    scene_->clear();
    delete scene_;
  }
  std::cout << "~RenderArea" << std::endl;
}

QGraphicsView *RenderArea::view() const
{
  return static_cast<QGraphicsView *>(graphicsView);
}

void RenderArea::resetView()
{
  zoomSlider->setValue(80);
  setupMatrix();
  graphicsView->ensureVisible(QRectF(0, 0, 0, 0));

  resetButton->setEnabled(false);
}

void RenderArea::setResetButtonEnabled()
{
  resetButton->setEnabled(true);
}

void RenderArea::setupMatrix()
{
  scale_ = qPow(qreal(2), (zoomSlider->value()) / qreal(50));
//  std::cout << "[RenderArea::setupMatrix] zoomSlider->value(): " << zoomSlider->value() << std::endl;
  QMatrix matrix;
  matrix.scale(scale_, scale_);

  graphicsView->setMatrix(matrix);
  setResetButtonEnabled();
  emit updateSceneSignal();
}

void RenderArea::togglePointerMode()
{
  graphicsView->setDragMode(selectModeButton->isChecked()
                            ? QGraphicsView::RubberBandDrag
                            : QGraphicsView::ScrollHandDrag);
  graphicsView->setInteractive(selectModeButton->isChecked());
}

void RenderArea::togglePause()
{
  ClientManager::instance_->setPause(pauseButton->isChecked());
}

void RenderArea::toggleOpenGL()
{
#ifndef QT_NO_OPENGL
    graphicsView->setViewport(openGlButton->isChecked() ? new QGLWidget(QGLFormat(QGL::SampleBuffers)) : new QWidget);
#endif
}

void RenderArea::toggleAntialiasing()
{
  graphicsView->setRenderHint(QPainter::Antialiasing, antialiasButton->isChecked());
}

void RenderArea::print()
{
#ifndef QT_NO_PRINTER
  QPrinter printer;
  QPrintDialog dialog(&printer, this);
  if (dialog.exec() == QDialog::Accepted) {
      QPainter painter(&printer);
      graphicsView->render(&painter);
  }
#endif
}

void RenderArea::zoomIn(int level)
{
    zoomSlider->setValue(zoomSlider->value() + level);
}

void RenderArea::zoomOut(int level)
{
    zoomSlider->setValue(zoomSlider->value() - level);
}

void RenderArea::updateScene()
{
  if(!scene_) return;
  scene_->clear();

  // draw overall workload
  if (overall_procedure_ && overall_procedure_->visible_) {
    QPen pen(overall_procedure_->color_, 2.0/scale_, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
    QPainterPath path;
    path.addPolygon(checkViewCoordinates(overall_procedure_->workload_));
    scene_->addPath(path,pen);
  }

  // draw procedure workloads
  {
    boost::shared_lock<boost::shared_mutex> lock(procedures_lock_);
    for (QVector<Procedure*>::iterator it = procedures_.begin();
         it != procedures_.end();
         it++) {
      if ((*it)->visible_) {
        QPainterPath path;
        QPen pen((*it)->color_, 1.0/scale_, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
        path.addPolygon(checkViewCoordinates((*it)->workload_));
        scene_->addPath(path, pen);
      }
    }
  }

  // set scene rect to size of content
  QRectF rect = scene_->sceneRect();
  int scene_size;
  if (overall_procedure_) {
    scene_size = overall_procedure_->workload_.size() * -points_per_update_ * scale_;
  } else {
    scene_size = 0;
  }
  qreal scene_width = ((scene_size > -view()->geometry().width()) ? -view()->geometry().width() : scene_size);
  rect.setWidth(scene_width/scale_);
  rect.setX(0);
  scene_->setSceneRect(rect);

  int x_coordinates = (view()->horizontalScrollBar()->value() ? view()->horizontalScrollBar()->value() : view()->x() - view()->width()) + 50;
  drawCoordinates(x_coordinates);
}

void RenderArea::addOverallWorkload(long double y)
{
  if (!overall_procedure_) {
    overall_procedure_ = new Procedure(1 << 31, points_per_update_, ClientManager::instance_->getUpdateIntervall(), "Overall Workload", false);
    overall_procedure_->setColor(Qt::red);
  }

  overall_procedure_->addWorkload(y);
}

void RenderArea::addProcedureWorkloads(std::map<int, long double>& procedure_workloads, std::map<int, char*>& procedure_names)
{
  long double sum = 0.0;

  // add calculated value
  for (std::map<int, long double>::iterator it = procedure_workloads.begin();
       it != procedure_workloads.end();
       it++) {
    // check if procedure is new and if so, create it
    Procedure* proc = getProcedure((*it).first);
    if (!proc) {
      boost::unique_lock<boost::shared_mutex> lock(procedures_lock_);
      procedures_.push_back(new Procedure((*it).first, points_per_update_,
                            ClientManager::instance_->getUpdateIntervall(),
                            procedure_names.at((*it).first),
                            false));
      proc = procedures_.back();
      std::cout << "[RenderArea::addProcedureWorkloads] adding Procedure: " << proc->name_.toStdString() << std::endl;
    }
    // add workload to procedure
    proc->addWorkload((*it).second);
    sum += (*it).second;
  }
  //marker_.updatePosition();
  start_marker_.updatePosition();
  end_marker_.updatePosition();
  addOverallWorkload(sum);

  // overall workload has to be <= 100%!
  assert(sum <= 1.0);
  emit updateSceneSignal();
}

void RenderArea::drawCoordinates(int x, QGraphicsScene *scene)
{
//  std::cout << "RenderArea::drawCoordinates" << std::endl;
  qreal scale = 1.0;
  if (!scene) {
    scene = scene_;
    scale = scale_;
    x = x/scale_;
  }

  QPen pen;
  pen.setWidth(pen.width()/scale_);
  scene->addLine(x, 0, x, -100, pen);

  // horizontal
  for (int i = 0; i > -110; i -=10) {
    QPen pen;
    pen.setStyle(Qt::DotLine);
    pen.setWidth(pen.width()/scale);
    scene->addLine(x, i, 0, i, pen);

    QFont font;
    qreal point_size = 8;
    // point_size = (point_size < 1.0) ? 1.0 : point_size;
    font.setPointSize(point_size);

    // set item to scene, scale + position
    QGraphicsTextItem* item = scene->addText(QString::number(-i).append("%"), font);
    item->setScale(1/scale);
    item->setPos(x - item->boundingRect().width()/scale, i - item->boundingRect().height()/(2.0f*scale));
  }

  // vertical
  // tau = 100 -> 10 updates in 1 second (updates_per_second = 1000/tau)
  float updates_per_second = 1000.0/static_cast<float>(ClientManager::instance_->getUpdateIntervall());
  // every update points_per_second_ pixels -> in one second 30 pixels (3*updates_per_second)
  float pixels_per_second = points_per_update_ * updates_per_second;
  // every 60 pixels one vertical line: 60 / pixels_per_second
  qreal x_vertical_line = 60;

  for (qreal i = 0; i >= x; i -= x_vertical_line) {
    QPen pen;
    pen.setWidth(pen.width()/scale);
    QGraphicsLineItem* line = scene->addLine(i, 0, i, 10/scale);
    line->setPen(pen);
    QPen pen2;
    pen2.setWidth((pen2.width()/scale));
    pen2.setStyle(Qt::DotLine);
    QGraphicsLineItem* line2 = scene->addLine(i, 0, i, -100);
    line2->setPen(pen2);

    QFont font;
    qreal point_size = 8;
    font.setPointSize(point_size);

    // set text to scene, scale + position
    QGraphicsTextItem* item = scene->addText(QString::number(-1.0 * static_cast<float>(i) / pixels_per_second).append("s"), font);
    item->setScale(1/scale);
    item->setPos(i - item->boundingRect().width()/(2.0 * scale), 10/scale + item->boundingRect().height()/(8.0 * scale));
  }
  //marker_.drawMarker(scene, scale, ((view()->horizontalScrollBar()->value() ? view()->horizontalScrollBar()->value() : view()->x() - view()->width()) + 55) /scale_);

  start_marker_.drawMarker(scene, scale, ((view()->horizontalScrollBar()->value() ? view()->horizontalScrollBar()->value() : view()->x() - view()->width()) + 55) /scale_);
  end_marker_.drawMarker(scene, scale, ((view()->horizontalScrollBar()->value() ? view()->horizontalScrollBar()->value() : view()->x() - view()->width()) + 55) /scale_);
}

QPolygon RenderArea::checkViewCoordinates(QPolygon p)
{
  qreal min_x = ((view()->horizontalScrollBar()->value() ? view()->horizontalScrollBar()->value() : view()->x() - view()->width()) + 55) /scale_;
  QPolygon ret_poly;
  for (QPolygon::Iterator it = p.end() - 1;
       it != p.begin() - 1;
       it--) {
    if((*it).x() < min_x)
      return ret_poly;
    else if ((*it).x() > min_x && (*it).x() < min_x + view()->width())
      ret_poly.push_back(*it);
  }
  return ret_poly;
}

void RenderArea::update()
{
//  emit updateSceneCoordinatesSignal();
  QFrame::update();
}

Procedure* RenderArea::getProcedure(int id)
{
  boost::shared_lock<boost::shared_mutex> lock(procedures_lock_);
  for (QVector<Procedure*>::iterator it = procedures_.begin();
       it != procedures_.end();
       it++) {
    if ((*it)->id_ == id)
      return *it;
  }

  return 0;
}

Procedure* RenderArea::getOverallProcedure()
{
  return overall_procedure_;
}

void RenderArea::saveData()
{
  export2SVG();
}

void RenderArea::export2SVG()
{
  QSvgGenerator svgGen;
  QGraphicsScene scene2export;
  QPainter painter;
  int x, size = 0;

  // get filename
  QString filter = "SVG (*.svg)";
  QString filename = QFileDialog::getSaveFileName(this,
                                                  tr("Export data to svg"),
                                                  QDir::toNativeSeparators(QDir::homePath().append("/new_record.svg")),
                                                  filter,
                                                  &filter);
  // setup svg generator
  if (filename.isEmpty()) {
    return;
  }
  svgGen.setFileName(filename);
  svgGen.setTitle("SVG painting of scene");
  svgGen.setDescription("Description of my scene");

  // paint procedures to scene
  {
    boost::shared_lock<boost::shared_mutex> lock(procedures_lock_);
    for (QVector<Procedure*>::iterator it = procedures_.begin();
         it != procedures_.end();
         it++) {
      if ((*it)->visible_) {
        QPen pen((*it)->color_, 1.0, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
        QPainterPath path;
        path.addPolygon((*it)->workload_);
        scene2export.addPath(path, pen);
      }
    }

    if (overall_procedure_) {
      QPen pen(overall_procedure_->color_, 2.0, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
      QPainterPath path;
      path.addPolygon(overall_procedure_->workload_);
      scene2export.addPath(path, pen);
    }

    size = overall_procedure_ ? overall_procedure_->workload_.size() : 0;
  }
  // draw coordinates to scene and set svg to scene size
  x = size * -points_per_update_;
  drawCoordinates(x, &scene2export);
  qreal size_of_square = 25;
  qreal y_offset = size_of_square + 10;
  for (QVector<Procedure*>::iterator it = procedures_.begin();
       it != procedures_.end();
       it++) {
    scene2export.addRect(x, y_offset, size_of_square, size_of_square, QPen(), QBrush((*it)->color_));
    QGraphicsTextItem* text = scene2export.addText((*it)->name_);
    text->setPos(x + size_of_square + 5, y_offset);
    y_offset += size_of_square + size_of_square/5;
  }
  svgGen.setSize(QSize(scene2export.sceneRect().width(), scene2export.sceneRect().height()));
  svgGen.setViewBox(QRect(0, 0, scene2export.sceneRect().width(), scene2export.sceneRect().height()));

  // initialise painter with svg gen and draw scene to painter
  painter.begin(&svgGen);
  painter.setBackground(Qt::white);
  scene2export.render(&painter);
  painter.end();
}

void RenderArea::clearProcedures()
{
  {
    boost::unique_lock<boost::shared_mutex> lock(procedures_lock_);
//    std::cout << "[RenderArea::clearProcedures] 1" << std::endl;
    for (QVector<Procedure*>::iterator it = procedures_.begin();
         it != procedures_.end();
         it++) {
      delete (*it);
    }
    //std::cout << "[RenderArea::clearProcedures] 2" << std::endl;
    procedures_.clear();

    if (overall_procedure_) {
      delete overall_procedure_;
      overall_procedure_ = 0;
    }
  }
}

void RenderArea::updateSceneCoordinates()
{
  // draw coordinates
}

