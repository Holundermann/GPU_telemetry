#include "ConnectionDialog.h"


#include <Data/ClientManager.h>
#include <Data/HandleConfigFile.h>

void ConnectionDialog::getOption(QString &param, std::string option)
{
  std::string tmp;
  tmp = HandleConfigFile::instance_->getParameter(option);
  if (tmp.size()) {
    std::cout << "[ConnectionDialog::getOption] found parameter for option " << option << ", param: " << tmp << std::endl;
    param.clear();
    param = QString(tmp.c_str());
  }
}

ConnectionDialog::ConnectionDialog(QWidget* parent, QString ip_str, QString port_str, QString update_intervall_str, QString rb_size_str) : QDialog(parent)
{
  getOption(ip_str, "ip");
  getOption(port_str, "port");
  getOption(update_intervall_str, "update_intervall");
  getOption(rb_size_str, "size_of_ringbuffer");
  QHBoxLayout* connection_text_layout_ = new QHBoxLayout;
  ip = new QLineEdit;
  ip->setText(ip_str);
  ip->setFixedWidth(ip->fontMetrics().width("000.000.000.00000"));
  ip->setAlignment(Qt::AlignRight);
  QLabel* ip_label = new QLabel("IP-Address: ");
  connection_text_layout_->addWidget(ip_label);
  connection_text_layout_->addWidget(ip);

  QHBoxLayout* port_layout = new QHBoxLayout;
  port = new QLineEdit;
  port->setText(port_str);
  port->setFixedWidth(port->fontMetrics().width("655380"));
  port->setAlignment(Qt::AlignRight);
  QLabel* port_label = new QLabel("Port: ");
  port_layout->addWidget(port_label);
//  port_layout->addSpacing(200);
  port_layout->addWidget(port);

  QHBoxLayout* update_intervall_layout = new QHBoxLayout;
  update_intervall = new QLineEdit;
  update_intervall->setText(update_intervall_str);
  update_intervall->setFixedWidth(update_intervall->fontMetrics().width("100"));
  update_intervall->setAlignment(Qt::AlignRight);
  QLabel* update_label = new QLabel("Updates per second: ");
  update_intervall_layout->addWidget(update_label);
//  update_intervall_layout->addSpacing(200);
  update_intervall_layout->addWidget(update_intervall);

  QHBoxLayout* rb_layout = new QHBoxLayout;
  rb_size = new QLineEdit;
  rb_size->setText(rb_size_str);
  rb_size->setFixedWidth(rb_size->fontMetrics().width("10000"));
  rb_size->setAlignment(Qt::AlignRight);
  QLabel* rb_size_label = new QLabel("Size of Ringbuffer (MB): ");
  rb_layout->addWidget(rb_size_label);
  rb_layout->addWidget(rb_size);

  QHBoxLayout* buttons_layout_ = new QHBoxLayout;
  QPushButton* ok_button_ = new QPushButton;
  ok_button_->setText("OK");
  QPushButton* cancel_button_ = new QPushButton;
  cancel_button_->setText("Cancel");
  buttons_layout_->addWidget(ok_button_);
  buttons_layout_->addWidget(cancel_button_);

  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->addLayout(connection_text_layout_, 0,0);
  mainLayout->addLayout(port_layout, 1,0);
  mainLayout->addLayout(update_intervall_layout, 2, 0);
  mainLayout->addLayout(rb_layout, 3, 0);
  mainLayout->addLayout(buttons_layout_,5,0);

  setLayout(mainLayout);
  setWindowTitle("new connection");

  connect(ok_button_, SIGNAL(clicked()), this, SLOT(ok_button_pressed()));
  connect(cancel_button_, SIGNAL(clicked()), this, SLOT(cancel_button_pressed()));
}

void ConnectionDialog::ok_button_pressed()
{
  uint64_t abs_rb_size_in_byte = rb_size->text().toInt();
  abs_rb_size_in_byte *= 1024;
  abs_rb_size_in_byte *= 1024;
  ClientManager::instance_->tcpConnect(ip->text().toStdString(), port->text().toInt(), 1000/update_intervall->text().toInt());
  MainWindow::instance_->setupRingBuffer(abs_rb_size_in_byte);
  ClientManager::instance_->createIOThread();
  close();
}

void ConnectionDialog::cancel_button_pressed()
{
  close();
}
