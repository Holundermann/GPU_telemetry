#include <fstream>

#include <QGridLayout>
#include <QStatusBar>
#include <QToolBar>
#include <QTimer>

#include "View/MainWindow.h"
#include "Data/ClientManager.h"

MainWindow* MainWindow::instance_ = 0;
boost::mutex MainWindow::instance_lock_;

#define TIMEOUT 10000

MainWindow::MainWindow() : block_area_(0),
                           connection_dialog_(0),
                           parsed_data_rb_(0),
                           update_counter_(0)
{
  createActions();
  createMenus();
  createToolBars();
  setupCentralWidget();
  createStatusBar();
  std::cout << "MainWindow::MainWindow()" << std::endl;
//  QTimer* timer = new QTimer(this);
//  connect(timer, SIGNAL(timeout()), this, SLOT(close()));
//  timer->start(TIMEOUT);
}

void MainWindow::createInstance()
{
  boost::unique_lock<boost::mutex> lock(instance_lock_);
  if(!instance_) {
    instance_ = new MainWindow;
  }
}

MainWindow::~MainWindow()
{
  delete render_area_;
  render_area_ = 0;
  delete status_area_;
  status_area_ = 0;
  if (parsed_data_rb_) {
    parsed_data_rb_->clear();
    delete parsed_data_rb_;
  }
  instance_ = 0;
  std::cout << "MainWindow::~MainWindow()" << std::endl;
}

void MainWindow::createActions()
{

}

void MainWindow::createMenus()
{

}

void MainWindow::createToolBars()
{
  tb_std_toolbar_ = addToolBar(tr("ToolBar"));

  // new connection
  new_connection_ = new QAction(QIcon("://images/connect-icon.png"), tr("&New"), this);
  new_connection_->setShortcuts(QKeySequence::New);
  new_connection_->setStatusTip(tr("Create a new connection"));

  // svg save
  save_svg_ = new QAction(QIcon("://images/document-save_svg.png"), tr("&Save"), this);
  save_svg_->setShortcuts(QKeySequence::Save);
  save_svg_->setStatusTip(tr("Save diagram as svg"));

  // binary save
  save_binary_ = new QAction(QIcon("://images/document-save_binary"), tr("&Save"), this);
  save_binary_->setShortcuts(QKeySequence::SaveAs);
  save_binary_->setStatusTip(tr("Save collected data binary"));

  // block diagram
  block_diagram_ = new QAction(QIcon("://images/diagramm.png"), tr("&Block Diagram"), this);
  block_diagram_->setShortcut(QKeySequence::Bold);
  block_diagram_->setStatusTip(tr("creates block diagram"));

  tb_std_toolbar_->addAction(new_connection_);
  tb_std_toolbar_->addAction(save_svg_);
  tb_std_toolbar_->addAction(save_binary_);
  tb_std_toolbar_->addAction(block_diagram_);

  connect(new_connection_, SIGNAL(triggered()), this, SLOT(newConnection()));
  connect(save_svg_, SIGNAL(triggered()), this, SLOT(saveDiagram()));
  connect(block_diagram_, SIGNAL(triggered()), this, SLOT(newBlockDiagram()));
  connect(save_binary_, SIGNAL(triggered()), this, SLOT(saveBinary()));
}

void MainWindow::createStatusBar()
{
  sb_connected_ = new QLabel(" W999 ");
  sb_connected_->setAlignment(Qt::AlignHCenter);
  sb_connected_->setMinimumSize(sb_connected_->sizeHint());

  sb_speed_ = new QLabel(" 1234 ");
  sb_speed_->setIndent(3);

  sb_area_in_ms_ = new QLabel("no area selected");

  statusBar()->addWidget(sb_connected_);
  statusBar()->addWidget(sb_speed_);
  statusBar()->addWidget(sb_area_in_ms_);
  updateStatusBar();

  // update statusbar every second
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(updateStatusBar()));
  timer->start(STATUSBAR_UPDATE_INTERVALL);
}

void MainWindow::setupCentralWidget()
{
  central_widget_ = new QWidget;
  render_area_ = new RenderArea("Workload", this);
  std::cout << "MainWindow::setupCentralWidget()" << std::endl;
  status_area_ = new StatusArea(render_area_->getProcedureLock(), this);
  setCentralWidget(central_widget_);

  // add layout with widgets and do the layout stuff
  QGridLayout *mainLayout = new QGridLayout;

  mainLayout->addWidget(status_area_, 1, 0);
  mainLayout->addWidget(render_area_, 1, 1);
  mainLayout->setColumnStretch(0, 1);
  mainLayout->setColumnStretch(1, 20);

  central_widget_->setLayout(mainLayout);
}

void MainWindow::addPerformanceData(std::map<int, long double>& procedure_workloads, std::map<int, char*>& procedure_names, float base_time)
{
  if (render_area_) {
    render_area_->addProcedureWorkloads(procedure_workloads, procedure_names);
    boost::unique_lock<boost::mutex> lck(base_time_lookup_table_lock_);
    // erase as long base time from the lt as saved based time is lower
    if (parsed_data_rb_->size()) {
        ++update_counter_;
        base_time_lookup_table_.push_back(base_time);
        while ((*base_time_lookup_table_.begin()) < (*parsed_data_rb_->begin()).base_time) {
          base_time_lookup_table_.pop_front();
      }
    }
  }
}

void MainWindow::updateStatusBar()
{
  if (sb_connected_) {
    sb_connected_->setText(ClientManager::instance_->getStatus());
  }
  if (sb_speed_) {
    sb_speed_->setText(ClientManager::instance_->getTransferInfo());
  }
  if (sb_area_in_ms_) {
    sb_area_in_ms_->setText(getSelectedAreaInMS());
  }
}

void MainWindow::newConnection()
{
  if (!connection_dialog_) {
    connection_dialog_ = new ConnectionDialog(this);
    connection_dialog_->setModal(true);
    connection_dialog_->raise();
  }
  connection_dialog_->show();
//  connection_dialog_->setFocus();
  connection_dialog_->activateWindow();
}

void MainWindow::setGpuInfo()
{
  if (status_area_) {
    status_area_->setGpuInfo();
  }
}

void MainWindow::registerProcedure(Procedure* proc)
{
  if (status_area_) {
    status_area_->registerProcedure(proc);
  }
}

void MainWindow::saveDiagram()
{
  if (render_area_) {
    render_area_->saveData();
  }
}

void appendData(char* buffer, const char* data, size_t& pos, size_t length_data)
{
  for(size_t i = 0; i < length_data; i++) {
    buffer[i + pos] = data[i];
  }
  pos += length_data;
}

void MainWindow::saveBinary()
{
  // get filename
  QString filter = "Binary (*.bin)";
  QString filename = QFileDialog::getSaveFileName(this,
                                                  tr("Export data binary"),
                                                  QDir::toNativeSeparators(QDir::homePath().append("/new_record.bin")),
                                                  filter,
                                                  &filter);

  if (filename.isEmpty()) {
    return;
  }
  // open file for binary writing
  std::ofstream file;
  file.open(filename.toStdString().c_str(), std::ofstream::out | std::ofstream  ::binary);
  // binary file hast the form: number of procedures | number of char of procedure name | procedure name | next procedure...
  // ... | parsed data as it is in ring buffer
  if (file.is_open()) {
    { // scope for procedure lock
      // first write number of Procedures into file
      boost::shared_lock<boost::shared_mutex> lock(render_area_->getProcedureLock());
      QVector<Procedure*> procedures = *(render_area_->getProcedures());
      int number_of_procedures = procedures.size();
      file.write(static_cast<char*>(static_cast<void*>(&number_of_procedures)), sizeof(int));
      // now the names of the procedures in the form of # characters | name
      for (QVector<Procedure*>::iterator it = procedures.begin();
           it != procedures.end();
           it++) {
        Procedure* proc = *it;
        int name_size = proc->name_.size();
        file.write(static_cast<char*>(static_cast<void*>(&name_size)), sizeof(int));
        std::string proc_name = (proc->name_.toStdString());
        std::cout << proc_name << std::endl;
        file.write(proc_name.c_str(), proc->name_.size() + 1);
      }
    }
    // and now the data - as it is in ring buffer
    { // scope for ring buffer lock
      /*
       *struct parsed_data {
          unsigned int procedure_id;
          float base_time;
          uint64_t task_start;
          uint64_t task_end;
          int multiprocessor_id;
          int active_thread_mask;
          int not_sure;
        } ;
       * */
      boost::unique_lock<boost::mutex> lock(parsed_data_rb_lock_);
      // to speed up writing we first write everything to one buffer and then write it to file
      std::cout << "MainWindow::saveBinary: before allocating" << std::endl;
      char* buffer = new char[sizeof(parsed_data)];
      std::cout << "MainWindow::saveBinary: after allocating, allocated: "<< parsed_data_rb_->size() << std::endl;
      size_t pos = 0;
      for (boost::circular_buffer<parsed_data>::iterator it = parsed_data_rb_->begin();
           it != parsed_data_rb_->end();
           it++) {
        pos = 0;
        parsed_data data = (*it);
        //std::cout << "MainWindow::saveBinary: &(data.procedure_id)" << &(data.procedure_id) << std::endl;
        appendData(buffer, (char*) &(data.procedure_id), pos, sizeof(unsigned int));
        appendData(buffer, (char*) &(data.base_time), pos, sizeof(float));
        appendData(buffer, (char*) &(data.task_start), pos, sizeof(uint64_t));
        appendData(buffer, (char*) &(data.task_end), pos, sizeof(uint64_t));
        appendData(buffer, (char*) &(data.multiprocessor_id), pos, sizeof(int));
        appendData(buffer, (char*) &(data.active_thread_mask), pos, sizeof(int));
        appendData(buffer, (char*) &(data.not_sure), pos, sizeof(int));
        file.write(buffer, sizeof(parsed_data));
      }
      delete[] buffer;
      buffer = 0;
      file.close();
    }
  }
}

void MainWindow::newBlockDiagram()
{
  block_area_ = new BlockArea(this);
  block_area_->createView();
  block_area_->show();
}

void MainWindow::clearProcedures()
{
  if (render_area_) {
    render_area_->clearProcedures();
  }
}

void MainWindow::unregisterProcedure(int id)
{
  if (status_area_) {
    status_area_->unregisterProcedure(id);
  }
}

void MainWindow::setupRingBuffer(uint64_t size)
{
  size = size/sizeof(parsed_data);
  std::cout << "[MainWindow::setupRingBuffer] init ringbuffer with size "<< size << std::endl;
  boost::unique_lock<boost::mutex> lock(parsed_data_rb_lock_);
  if(parsed_data_rb_) {
    parsed_data_rb_->clear();
    delete parsed_data_rb_;
    boost::unique_lock<boost::mutex> lt_lock(base_time_lookup_table_lock_);
    base_time_lookup_table_.clear();
    update_counter_ = 0;
  }
  rb_size_ = size;
  parsed_data_rb_ = new boost::circular_buffer<parsed_data>(size);
  std::cout << "[MainWindow::setupRingBuffer] initialised" << std::endl;
}

void MainWindow::addElement2RB(parsed_data data)
{
  boost::lock_guard<boost::mutex> lock(parsed_data_rb_lock_);
  parsed_data_rb_->push_back(data);
}

Procedure* MainWindow::getProcedure(int id)
{
  if (render_area_) {
    return render_area_->getProcedure(id);
  }
  return 0;
}

Procedure* MainWindow::getOverallProcedure()
{
  if (render_area_) {
    return render_area_->getOverallProcedure();
  }
  return 0;
}

Marker* MainWindow::getStartMarker()
{
  if (render_area_) {
    return &render_area_->start_marker_;
  }
  return 0;
}

Marker* MainWindow::getEndMarker()
{
  if (render_area_) {
    return &render_area_->end_marker_;
  }
  return 0;
}

float MainWindow::getMaxTime()
{
  boost::lock_guard<boost::mutex> lock(parsed_data_rb_lock_);
  if (parsed_data_rb_->size()) {
    // get time intervall which is saved
//    std::cout << "[MainWindow::getMaxTime] time intervall which is saved: " << parsed_data_rb_->back().base_time - parsed_data_rb_->front().base_time <<std::endl;
    return parsed_data_rb_->back().base_time - parsed_data_rb_->front().base_time;
  }
  return 0;
}

QSize MainWindow::sizeHint() const
{
  return QSize(1024,500);
}

uint64_t MainWindow::getTickCounter()
{
  boost::unique_lock<boost::mutex> lock(base_time_lookup_table_lock_);
  return update_counter_;
}

size_t MainWindow::getBaseTimeLookupTableSize()
{
  boost::unique_lock<boost::mutex> lock(base_time_lookup_table_lock_);
  return base_time_lookup_table_.size();
}

float MainWindow::getBaseTime(uint64_t index)
{
  boost::unique_lock<boost::mutex> lock(base_time_lookup_table_lock_);
  std::list<float>::iterator it = std::next(base_time_lookup_table_.begin(), index);
  std::cout << "[MainWindow::getBaseTime] base time " << (*it) << std::endl;
  if (it != base_time_lookup_table_.end()) {
    return (*it);
  } else {
    std::cout << "[MainWindow::getBaseTime] couldn´t find base time with index " << index << std::endl;
  }
  return 0;
}

QString MainWindow::getSelectedAreaInMS()
{
  QString prefix = "Selected timeperiod: ";
  if (render_area_) {
    return QString(prefix + QString::number(abs(render_area_->selected_area_in_ms_), 'f', 2) + " ms");
  }
  return QString(prefix + "--- ms");
}

void MainWindow::resetSelectedArea()
{
  render_area_->selected_area_in_ms_ = 0;
}
