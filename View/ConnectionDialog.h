#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QtWidgets>


class ConnectionDialog : public QDialog
{
    Q_OBJECT
  public:
    ConnectionDialog(QWidget* parent = 0,
                     QString ip = "127.0.0.1",
                     QString port = "4242",
                     QString update_intervall_str = "5",
                     QString rb_size_str = "200");

  public slots:
    void ok_button_pressed();
    void cancel_button_pressed();

  private:
    QLineEdit* ip;
    QLineEdit* port;
    QLineEdit* update_intervall;
    QLineEdit* rb_size;

    void getOption(QString& param, std::string option);
};

#endif // CONNECTIONDIALOG_H
