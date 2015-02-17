#include "Procedure.h"

#include <iostream>

#include "View/MainWindow.h"

Procedure::Procedure(int id, int points_per_second, int update_intervall, QString name, bool append_id_to_name) : id_(id),
                                                                     name_(name),
                                                                     visible_(true),
                                                                     points_per_update_(points_per_second),
                                                                     update_intervall_(update_intervall)
{
  if (append_id_to_name) {
    name_.append(QString::number(id));
  }
  color_ = QColor(qrand() % 255, qrand() % 255, qrand() % 255);
  std::cout << "[Procedure::Procedure] register procedure " << id_ << std::endl;
  MainWindow::instance_->registerProcedure(this);
}

Procedure::~Procedure()
{
  // unregister procedure from status area!
  std::cout << "[Procedure::~Procedure] unregister procedure " << id_ << std::endl;
  MainWindow::instance_->unregisterProcedure(id_);
}

void Procedure::addWorkload(long double workload)
{
  for (QPolygon::iterator it = workload_.begin();
       it != workload_.end();
       it++) {
    (*it).setX((*it).x() - points_per_update_);
  }

  workload_.push_back(QPoint(0, workload * -100));
  // only hold as many data points as the ringbuffer in mainwindow can hold
  while (workload_.size() &&
         abs(workload_.front().rx()/static_cast<float>(points_per_update_) * static_cast<float>(update_intervall_))
         > MainWindow::instance_->getMaxTime()) {
//    std::cout << "[Procedure::addWorkload] poping front" << std::endl;
    workload_.pop_front();
  }
}
