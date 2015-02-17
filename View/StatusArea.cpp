#include "StatusArea.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QCheckBox>
#include <QColorDialog>

#include "Data/ClientManager.h"

StatusArea::StatusArea(boost::shared_mutex& procedures_lock, QWidget* parent)
  : QWidget(parent),
    proc_model_(procedures_lock, this)
{
  gfxLabel_ = new QLabel(tr("---"));
  gfxCCLabel_ = new QLabel(tr("---"));

  QFormLayout *mainLayout = new QFormLayout;
  mainLayout->addRow(tr("&Video card:"), gfxLabel_);
  mainLayout->addRow(tr("&CC:"),gfxCCLabel_);

  table_view_.setModel(&proc_model_);
  table_view_.resizeColumnsToContents();

  QHBoxLayout* table_view_layout = new QHBoxLayout;
  table_view_layout->addWidget(&table_view_);
  table_view_layout->setAlignment(Qt::AlignCenter);
  mainLayout->addRow(table_view_layout);
  setLayout(mainLayout);

  connect(this, SIGNAL(updateGpuInfoSignal()), this, SLOT(update()));
  connect(this, SIGNAL(updateTableViewSizeSignal()), this, SLOT(updateTableViewSize()));
  connect(&table_view_, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(tableViewDoubleClickEvent(const QModelIndex&)));
}

StatusArea::~StatusArea()
{
  std::cout << "~StatusArea" << std::endl;
}

QSize StatusArea::minimumSizeHint() const
{
  return QSize(280, 240);
}

QSize StatusArea::sizeHint() const
{
  return QSize(280, 240);
}

void StatusArea::setGpuInfo()
{
  if (ClientManager::instance_->getGpuInfo() && ClientManager::instance_->getGpuInfo()->device_name) {
//    std::cout << "StatusArea::setGpuInfo: Multiprocessor count: " << ClientManager::instance_->getGpuInfo()->multiprocessor_count << std::endl;
//    std::cout << "StatusArea::setGpuInfo: Threads per mp: " << ClientManager::instance_->getGpuInfo()->max_thread_per_mp << std::endl;
    gfxLabel_->setText(ClientManager::instance_->getGpuInfo()->device_name);
    gfxCCLabel_->setText(QString::number(ClientManager::instance_->getGpuInfo()->cc_major));
  } else {
    gfxLabel_->setText("---");
    gfxCCLabel_->setText("---");
  }
  emit updateGpuInfoSignal();
}

void StatusArea::registerProcedure(Procedure* proc)
{
  proc_model_.registerProcedure(proc);
  emit updateTableViewSizeSignal();
}

void StatusArea::updateTableViewSize()
{
  table_view_.resizeColumnsToContents();
  QRect rect = table_view_.geometry();
  rect.setWidth(table_view_.style()->pixelMetric(QStyle::PM_ScrollBarExtent) + 2 // width of scrollbar
                + table_view_.verticalHeader()->width()
                + table_view_.columnWidth(0)
                + table_view_.columnWidth(1)
                + table_view_.columnWidth(2));

  QAbstractItemModel* model = table_view_.model();
  QHeaderView* verHeader = table_view_.verticalHeader();
  int rows = model->rowCount();
  int y = verHeader->sectionViewportPosition(rows-1) + verHeader->offset()
          + verHeader->sectionSize(rows-1) + 50;
  rect.setHeight(y);
  table_view_.setGeometry(rect);
  table_view_.setFixedHeight(y);
  update();
}

void StatusArea::unregisterProcedure(int id)
{
  proc_model_.unregisterProcedure(id);
}

void StatusArea::tableViewDoubleClickEvent(const QModelIndex &index)
{
  std::cout << "StatusArea::setColor: index row " << index.row() << " index column " <<index.column() << std::endl;
  if (index.column() == 0) {
    boost::shared_mutex& procedures_lock = proc_model_.getProceduresLock();
    boost::shared_lock<boost::shared_mutex> lock(procedures_lock);
    Procedure* proc = proc_model_.getProcedure(index.row());
    if (proc) {
      QColor tmp = QColorDialog::getColor(proc->color_, this);
      if (tmp.isValid()) {
        proc->setColor(tmp);
      }
    }
  }
}
