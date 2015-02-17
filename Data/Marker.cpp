#include "Marker.h"

#include <QGraphicsPolygonItem>

#include <assert.h>
#include <iostream>

#include "Data/ClientManager.h"

Marker::Marker(int points_per_update) :
  points_per_update_(points_per_update),
  pos_(10),
  poly_(0),
  prev_scene_(0)
{
}

void Marker::drawMarker(QGraphicsScene *scene, qreal scale, qreal min_x)
{
  if (pos_ <= 0) {
    Procedure* proc = MainWindow::instance_->getOverallProcedure();
    if (proc && -proc->workload_.size() * proc->points_per_update_ > pos_) {
      pos_ = 10;
      MainWindow::instance_->resetSelectedArea();
      return;
    } else if (pos_ < min_x) {
      return;
    }
    poly_ = drawRectangle(scene, scale, pos_);
    poly_->setBrush(QBrush(Qt::green));
  }
}

QGraphicsPolygonItem* Marker::drawRectangle(QGraphicsScene *scene, qreal scale, qreal pos)
{
  QPen pen;
  pen.setWidth(pen.width()/scale);

  QPolygon poly;
  poly.push_back(QPoint(pos, 0));
  poly.push_back(QPoint((pos - triangle_size_), triangle_size_));
  poly.push_back(QPoint((pos + triangle_size_), triangle_size_));

  return scene->addPolygon(poly, pen);
}

void Marker::setMarker(qreal pos)
{
  std::cout << "[Marker::setMarker] setting marker to pos " << pos << std::endl;
  pos_ = pos;
  set_at_tick = MainWindow::instance_->getTickCounter();
  std::cout << "[Marker::setMarker] points_per_update_: " << points_per_update_ << std::endl;
  offset_from_marker_ = std::abs(pos_/points_per_update_);
  std::cout << "[Marker::setMarker] marker was set at tick: " << set_at_tick << std::endl;
  std::cout << "[Marker::setMarker] offset from marker is: " << offset_from_marker_ << std::endl;
}

float Marker::getTime()
{
  return MainWindow::instance_->getBaseTime(getBaseTimeLookupTableIndex());
}

void Marker::updatePosition()
{
  if (pos_ < 0) {
    pos_ -= points_per_update_;
  }
}

uint64_t Marker::getBaseTimeLookupTableIndex()
{
  uint64_t x = MainWindow::instance_->getTickCounter() - MainWindow::instance_->getBaseTimeLookupTableSize();
  uint64_t index = 0;
  if (set_at_tick - x - offset_from_marker_ > 0) {
    index = set_at_tick - x - offset_from_marker_;
  }
  std::cout << "[Marker::getBaseTimeLookupTableIndex] index from beginning is: " << index << std::endl;
  return index;
}
