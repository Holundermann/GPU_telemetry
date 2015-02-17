#include "BlockArea.h"

#include <QSvgGenerator>
#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#endif

#include "View/MainWindow.h"
#include "Data/ClientManager.h"
#include "Data/Procedure.h"

void BlockView::wheelEvent(QWheelEvent *e)
{
  if (e->modifiers() & Qt::ControlModifier) {
    if (e->delta() > 0)
        view->zoomIn(0.05);
    else
        view->zoomOut(-0.05);
    e->accept();
  } else {
     QGraphicsView::wheelEvent(e);
  }
}

QSizeF BlockView::sizeHint()
{
  return QSizeF(800, 640);
}

BlockArea::BlockArea(QWidget* parent) : QDialog(parent),
                                        max_items2draw(150000),
                                        item_counter_(0),
                                        progress_dialog_(0)
{
  block_view_ = new BlockView(this);
  block_view_->setDragMode(QGraphicsView::ScrollHandDrag);

  QGridLayout *topLayout = new QGridLayout;
  topLayout->addWidget(block_view_, 1, 0);
  setLayout(topLayout);
  setWindowTitle("Block Chart");
  setWindowFlags(Qt::Window);
  block_view_->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  block_view_->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  block_view_->setScene(&scene_);
  createToolBar();
//#ifndef QT_NO_OPENGL
//  block_view_->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
//#endif
}

BlockArea::~BlockArea()
{
  std::cout << "~BlockArea" << std::endl;
}

void BlockArea::resetValues()
{
  std_scale_x_ = 0.05;
  std_scale_y_ = 0.001;

  x_offset_ = qreal(50);
  x_offset_ *= std_scale_x_;

  gpu_info_ = ClientManager::instance_->getGpuInfo();
  x_endpos_ = gpu_info_ ? x_offset_ + qreal(gpu_info_->max_thread_per_mp * gpu_info_->multiprocessor_count) * std_scale_x_
                        : x_offset_;
  outline_pen_.setWidth(2);
  inline_pen_.setWidth(1);
  inline_pen_.setStyle(Qt::DotLine);
  // point_size = (point_size < 1.0) ? 1.0 : point_size;
  label_font_.setPointSize(10);
  label_font_.setBold(true);
  y_pos_ = 0;
  scale_ = 1.0;
  end_time_ = 0;
  start_time_ = 0xFFFFFFFFFFFFFFFF;
}

bool sorting(parsed_data a, parsed_data b) {
  return (a.task_start < b.task_start);
}

int BlockArea::sortParsedDataTime(boost::circular_buffer<parsed_data>::iterator iterator_start, boost::circular_buffer<parsed_data>::iterator iterator_end, boost::circular_buffer<parsed_data>* rb, boost::mutex& rb_lock)
{
  int counter = 1;
  float curr_base_time = (*iterator_start).base_time;
  boost::circular_buffer<parsed_data>::iterator iterator = iterator_start;
  // sort the data over its start time
  while(++iterator != rb->end() && iterator != iterator_end + 1) {
    if ((*iterator).base_time != curr_base_time) {
      std::sort(iterator_start, iterator, sorting);
      iterator_start = iterator;
      curr_base_time = (*iterator).base_time;
    }
    ++counter;
  }
  std::cout << "[BlockArea::sortParsedDataTime] element count: " << counter << std::endl;
  return counter;
}

void BlockArea::drawProcedures(boost::circular_buffer<parsed_data>::iterator it,
                               boost::circular_buffer<parsed_data>::iterator end_it)
{
  float curr_base_time = (*it).base_time;
  float base_time_offset_ = curr_base_time;
  for (;
       it != end_it;
       it++) {
    if (curr_base_time != (*it).base_time) {
      drawTimeLine(start_time_, curr_base_time - base_time_offset_);
      drawTimeLine(end_time_, -1);
      //drawTimeLine(end_time_, curr_base_time - base_time_offset_ + (1.0f/float(gpu_info_->clock_rate * 1000)) * float(end_time_ - start_time_) );
      curr_base_time = (*it).base_time;
//      std::cout << "BlockArea::populateScene: end time: " << end_time_ << ", start time: "<< start_time_ << std::endl;
//      std::cout << "BlockArea::populateScene: y_pos_ is " << y_pos_ << std::endl;
      assert(y_pos_ > 0);
      y_pos_ += end_time_ * std_scale_y_;
      end_time_ = 0;
      start_time_ = 0xFFFFFFFFFFFFFFFF;
    }
    drawProcedure(*it);
    if (item_counter_ > max_items2draw || progress_dialog_->wasCanceled()) {
      break;
    }
    if (item_counter_ % 500 == 0) {
      progress_dialog_->setValue(item_counter_);
      QApplication::processEvents();
    }
  }
  drawTimeLine(start_time_, curr_base_time - base_time_offset_);
  drawTimeLine(end_time_, -1);
}

void BlockArea::populateScene()
{
  resetValues();
  resetSMXOffset();
  if (!gpu_info_) {
    return;
  }
  boost::mutex& rb_lock = MainWindow::instance_->getRBLock();
  boost::unique_lock<boost::mutex> lock(rb_lock);
  boost::circular_buffer<parsed_data>* rb = MainWindow::instance_->getRingBuffer();
  Marker* start_marker = MainWindow::instance_->getStartMarker();
  Marker* end_marker = MainWindow::instance_->getEndMarker();

  if (!start_marker || !end_marker) {
    return;
  }

  std::cout << "[BlockArea::populateScene] drawing smx processors" << std::endl;
  drawSMXProcessors();

  std::cout << "[BlockArea::populateScene] drawing procedures" << std::endl;
  float start_time = start_marker->getTime();
  float end_time = end_marker->getTime();
  if (start_time > end_time) {
    return;
  }
  boost::circular_buffer<parsed_data>::iterator it = getIteratorFromTimestamp(start_time);
  boost::circular_buffer<parsed_data>::iterator end_it = getIteratorFromTimestamp(end_time);

  if (it != rb->end() && end_it != rb->end()) {
    assert(it <= end_it);
    int item_count = sortParsedDataTime(it, end_it, rb, rb_lock);
    QProgressDialog progress("Creating Block Diagram...", "Abort", 0, item_count, this);
    progress.setWindowTitle("Processing");
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    progress.setMinimumHeight(120);
    progress.setMinimumWidth(265);
    progress.setMaximumHeight(120);
    progress.setMaximumWidth(265);
    progress_dialog_ = &progress;
    drawProcedures(it, end_it);
  }
  y_pos_ += end_time_ * std_scale_y_;

  std::cout << "[BlockArea::populateScene] draw vertical lines" << std::endl;
  drawVerticalLines();

  std::cout << "[BlockArea::populateScene] draw bottom line" << std::endl;
  drawBottomLine();

  std::cout << "[BlockArea::populateScene] draw procedure legend" << std::endl;
  drawLegend();

  std::cout << "[BlockArea::populateScene] finished, drawn "<< item_counter_ << " items!" << std::endl;
}

QList<QGraphicsItem*> BlockArea::checkCollision(QGraphicsRectItem *item, int mpid)
{
  QList<QGraphicsItem*> colliding_items;
  QRectF item_scene_rect = item->mapRectFromScene(item->rect());
  qreal y_pos = item_scene_rect.y();
  try {
    QList<QGraphicsItem*>& possible_colliding_items = (colliding_items_per_mp_.at(mpid));
    QList<QGraphicsItem*>::iterator it = possible_colliding_items.begin();
    while (it != possible_colliding_items.end()) {
      if ((*it)->collidesWithItem(item, Qt::IntersectsItemBoundingRect)) {
        colliding_items.push_back((*it));
        it++;
      } else {
        // check if we can discard this iterator because y + height is too low:
        // there will not be a feature procedure which has a lower y value than the current,
        // therefor we can make this optimisation
        QGraphicsRectItem* iter_rect = qgraphicsitem_cast<QGraphicsRectItem*>(*it);
        if (iter_rect) {
          QRectF iter_scene_rect = iter_rect->mapRectToScene(iter_rect->rect());
          if (iter_scene_rect.y() + iter_scene_rect.height() < y_pos) {
            it = possible_colliding_items.erase(it);
          } else {
            it++;
          }
        } else {
          it++;
        }
      }
    }
  } catch (std::out_of_range e) {
    std::cout << "[BlockArea::checkCollision] no item added so far for this mpid: " << mpid << std::endl;
  } catch (std::exception e) {
    std::cout << "[BlockArea::checkCollision] exception occured: " << e.what() << std::endl;
  }

  return colliding_items;
}

void BlockArea::drawProcedure(parsed_data &data)
{
  ++item_counter_;
  Procedure* proc = MainWindow::instance_->getProcedure(data.procedure_id);
  assert(proc);
  // add procedure to procedure list if not added, for the legend on the blockview
  if(std::find(procedures_.begin(), procedures_.end(), proc) == procedures_.end()) {
    procedures_.push_back(proc);
  }
  try {
    // set start + end time if necessary
    if (start_time_ > data.task_start) {
      start_time_ = data.task_start;
    }
    if (end_time_ < data.task_end) {
      end_time_ = data.task_end;
    }
    // draw rectangle on start position of each smx
    QBrush brush(proc->color_);
    QGraphicsRectItem* rect = scene_.addRect(smx_offset_hard_.at(data.multiprocessor_id), // x position
                                             y_pos_ + qreal(data.task_start) * std_scale_y_, // y position
                                             qreal(data.active_thread_mask) * std_scale_x_, // widht
                                             qreal(data.task_end - data.task_start) * std_scale_y_, //height
                                             procedure_pen_, brush);
    QList<QGraphicsItem*> intersection_list = checkCollision(rect, data.multiprocessor_id);
    QGraphicsItem* prev_item = 0;
    QGraphicsItem* tmp_item;
    // iterative set rectangle onto right position
    while(intersection_list.size()) {
      tmp_item = 0;
      for (QList<QGraphicsItem*>::iterator it= intersection_list.begin();
           it != intersection_list.end();
           it++) {
        if (!tmp_item) {
          tmp_item = (*it);
        } else if (tmp_item->scenePos().x() + tmp_item->boundingRect().width() <
                   (*it)->scenePos().x() + (*it)->boundingRect().width()){
          tmp_item = (*it);
        }
      }
      // position gets set relative, so change x position with width of colliding item, 0 is for y repositioning (keep y position)
      // TODO positioning is not always correct - find better solution!
      rect->setPos(tmp_item->mapToScene(tmp_item->boundingRect().width(), 0));
      // check for new intersections
      // intersection_list = scene_.collidingItems(rect, Qt::IntersectsItemBoundingRect);
      intersection_list = checkCollision(rect, data.multiprocessor_id);
      // workaround for miss sets - dont know why it happens but sometimes it collides twice with the same item - even
      // though it does not collide ^^ - so if this happens, break
      if (prev_item == tmp_item) {
        /*for (QList<QGraphicsItem*>::iterator it= intersection_list.begin();
             it != intersection_list.end();
             it++) {
          QGraphicsRectItem* item = qgraphicsitem_cast<QGraphicsRectItem*>(*it);
          if (item) {
            item->setBrush(QBrush(Qt::blue));
          }
        }
        rect->setBrush(QBrush(QColor(Qt::red)));*/
//        std::cout << "prev == current" << std::endl;
        break;
      }
      prev_item = tmp_item;
      //std::cout << "[BlockArea::drawProcedure] working on item " << rect << std::endl;
    }
    colliding_items_per_mp_[data.multiprocessor_id].push_back(rect);
  } catch (std::out_of_range e) {
    std::cout << "[BlockArea::drawProcedure] out of range: " << e.what() << " when accessing element " << data.multiprocessor_id + 1 << std::endl;
    assert(false);
  } catch (std::exception e) {
    std::cout << "[BlockArea::drawProcedure] Exception occured: " << e.what() << std::endl;
  }
}

void BlockArea::drawTimeLine(uint64_t time, float curr_time)
{
  // add horizontal line at y_pos_ + time
//  std::cout << "BlockArea::drawTimeLine drawing timeline to y pos " << pos << std::endl;
  scene_.addLine(x_offset_, y_pos_ + time * std_scale_y_, x_endpos_, y_pos_ + time * std_scale_y_, outline_pen_);
  if (gpu_info_) {
//    std::cout << "BlockArea::drawTimeLine gpu_info_->clock_rate: " << gpu_info_->clock_rate << std::endl;
    if (curr_time >= 0) {
      QGraphicsTextItem* item = scene_.addText(QString::number(curr_time, 'G', 4).append(" ms"), label_font_);
      item->setPos(x_offset_ - item->boundingRect().width(), y_pos_ + time * std_scale_y_ - item->boundingRect().height()/2.0f);
    }
  }
//  y_pos_ += item->boundingRect().height();
}

void BlockArea::drawSMXProcessors()
{
  assert(gpu_info_);
  // add heading "multiprocessor"
  QGraphicsTextItem* item = scene_.addText("multiprocessor", label_font_);
  //std::cout << "x_endpos_ " << x_endpos_ << " item->boundingRect().width() " << item->boundingRect().width() << std::endl;
  item->setPos(x_endpos_/2.0f - item->boundingRect().width()/2.0f, -item->boundingRect().height());
  y_pos_ += item->boundingRect().height();
  // add smx id foreach smx core below "multliprocessor"
  int i = 0;
  for (qreal x = gpu_info_->max_thread_per_mp/2 * std_scale_x_ + x_offset_;
       x < x_endpos_;
       x += gpu_info_->max_thread_per_mp * std_scale_x_, i++) {
    item = scene_.addText(QString::number(i), label_font_);
    item->setPos(x - item->boundingRect().width()/2, y_pos_);
  }
  y_pos_ += item->boundingRect().height();
  y_pos_heading_ = y_pos_;
  // add horizontal line
  scene_.addLine(x_offset_, y_pos_, x_endpos_, y_pos_, outline_pen_);
}

void BlockArea::drawVerticalLines()
{
  assert(gpu_info_);
  qreal init = gpu_info_->max_thread_per_mp * std_scale_x_ + x_offset_;
  // dotted lines for each smx core
  for (qreal x = init;
       x < x_endpos_;
       x += gpu_info_->max_thread_per_mp * std_scale_x_) {
    scene_.addLine(x, y_pos_, x, y_pos_heading_, inline_pen_);
  }
  // outline
  scene_.addLine(x_offset_, y_pos_, x_offset_, y_pos_heading_, outline_pen_);
  scene_.addLine(x_endpos_, y_pos_, x_endpos_, y_pos_heading_, outline_pen_);
}

void BlockArea::drawBottomLine()
{
  scene_.addLine(x_endpos_, y_pos_, x_offset_, y_pos_, outline_pen_);
}

void BlockArea::drawLegend()
{
  qreal x_pos = x_offset_ - 220;
  qreal y_pos = y_pos_heading_;
  qreal rectangle_size = 25;
  qreal y_spacing = 5;
  int proc_counter = 0;
  QPen pen;
  for (std::vector<Procedure*>::iterator it = procedures_.begin();
       it != procedures_.end();
       it++) {
    Procedure* proc = (*it);
//    std::cout << "[BlockArea::drawLegend] y_pos + proc_counter * rectangel_size: " << y_pos + proc_counter * rectangle_size << std::endl;
    scene_.addRect(x_pos, y_pos + proc_counter * (rectangle_size + y_spacing), rectangle_size, rectangle_size, pen, QBrush(proc->color_));
    QGraphicsTextItem* item = scene_.addText(proc->name_, label_font_);
    item->setPos(x_pos + rectangle_size, y_pos + proc_counter * (rectangle_size + y_spacing));
    proc_counter++;
  }
}

void BlockArea::resetSMXOffset()
{
  if (!gpu_info_) {
    return;
  }
  int i;
  for (i = 0; i < gpu_info_->multiprocessor_count; i++) {
    smx_offset_hard_[i] = i * gpu_info_->max_thread_per_mp * std_scale_x_ + x_offset_;
    smx_offset_[i] = smx_offset_hard_[i];
  }
  // set one more for boundary check
  smx_offset_hard_[i] = i * gpu_info_->max_thread_per_mp * std_scale_x_ + x_offset_;
}

void BlockArea::zoomIn(qreal v)
{
  setupMatrix(v);
}

void BlockArea::zoomOut(qreal v)
{
  setupMatrix(v);
}

void BlockArea::setupMatrix(qreal scale)
{
  scale_ += scale;
  if (scale_ <= 0) {scale_ = 0.01;}
//  std::cout << "BlockArea::setupMatrix: setting scale to " << scale_ << std::endl;
  QMatrix matrix;
  matrix.scale(scale_, scale_);
  block_view_->setMatrix(matrix);
//  block_view_->update();
}

void BlockArea::createView()
{
  populateScene();
//  std::cout << "BlockArea::createView: update view" << std::endl;
//  block_view_->update();

}

void BlockArea::export2SVG()
{
  QSvgGenerator svgGen;
  QPainter painter;
  // get filename
  QString filter = "SVG (*.svg)";
  QString filename = QFileDialog::getSaveFileName(this,
                                                  tr("Export data to svg"),
                                                  QDir::toNativeSeparators(QDir::homePath().append("/new_chart.svg")),
                                                  filter,
                                                  &filter);
  // setup svg generator
  if (filename.isEmpty()) {
    return;
  }
  svgGen.setFileName(filename);
  svgGen.setTitle("SVG painting of scene");
  svgGen.setDescription("Description of my scene");
  // set size of canvas
  svgGen.setSize(QSize(scene_.sceneRect().width(), scene_.sceneRect().height()));
  svgGen.setViewBox(QRect(0, 0, scene_.sceneRect().width(), scene_.sceneRect().height()));
  // initialise painter with svg gen and draw scene to painter
  painter.begin(&svgGen);
  painter.setBackground(Qt::white);
  scene_.render(&painter);
  painter.end();
}

void BlockArea::createToolBar()
{
  tb_std_toolbar_ = new QToolBar("Save");

  save_ = new QAction(QIcon("://images/document-save-3.png"), tr("&Save"), this);
  save_->setShortcuts(QKeySequence::Save);
  save_->setStatusTip(tr("Save"));

  tb_std_toolbar_->addAction(save_);
  layout()->setMenuBar(tb_std_toolbar_);
  connect(save_, SIGNAL(triggered()), this, SLOT(export2SVG()));
}

void BlockArea::reject()
{
  scene_.clear();
  delete this;
}

boost::circular_buffer<parsed_data>::iterator BlockArea::getIteratorFromTimestamp(float search_time)
{
  std::cout << "[BlockArea::getIteratorFromTimestamp] search time from marker is: " << search_time << std::endl;
  boost::circular_buffer<parsed_data>* rb = MainWindow::instance_->getRingBuffer();
  if (!rb->size()) {
    return rb->end();
  }

  float curr_time = rb->back().base_time;
  std::cout << "[BlockArea::getIteratorFromTimestamp] current time is: " << curr_time << std::endl;

  for (boost::circular_buffer<parsed_data>::iterator it = rb->begin();
       it != rb->end();
       it++) {
    if ((*it).base_time == search_time) {
      std::cout << "[BlockArea::getIteratorFromTimestamp] found iterator" << std::endl;
      return it;
    }
  }
  std::cout << "[BlockArea::getIteratorFromTimestamp] didn´t find a iterator" << std::endl;
  return rb->end();
}
