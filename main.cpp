#include <QApplication>
#include <QTimer>

#include <iostream>

#include "View/MainWindow.h"
#include "Data/ClientManager.h"
#include "Data/HandleConfigFile.h"

using namespace std;
#define TIMEOUT 600

int main(int argc, char* argv[])
{
  //QTimer *timer = new QTimer();
  QApplication a(argc, argv);
  HandleConfigFile::createInstance();
  ClientManager::createInstance();
  MainWindow::createInstance();

  // only call once!
//  ClientManager::instance_->stdConnect();
//  ClientManager::instance_->createIOThread();
  MainWindow::instance_->show();
//  connect(timer, SIGNAL(timeout()), MainWindow::instance_, SLOT(close()));
//  timer->start(TIMEOUT);
  int ret = a.exec();

  delete ClientManager::instance_;
  delete MainWindow::instance_;
  delete HandleConfigFile::instance_;

  return ret;
}
