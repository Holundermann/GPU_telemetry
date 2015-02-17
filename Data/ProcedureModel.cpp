#include "ProcedureModel.h"

#include <qnamespace.h>

ProcedureModel::ProcedureModel(boost::shared_mutex& procedures_lock, QObject *parent) :
  QAbstractTableModel(parent),
  procedures_lock_(procedures_lock),
  rows_(0)
{
  qRegisterMetaType<QVector<int> >();
}

void ProcedureModel::registerProcedure(Procedure* proc)
{
  std::cout << "[ProcedureModel::registerProcedure] register Procedure " << proc->id_ << std::endl;

  QModelIndex row_first_col = createIndex(rows_, 0);
  QModelIndex row_last_col = createIndex(rows_, columnCount() - 1);

  beginInsertRows(QModelIndex(), rows_, rows_);
  row2Procedure_[rows_] = proc;
  ++rows_;
  endInsertRows();

  emit dataChanged(row_first_col, row_last_col);
}

void ProcedureModel::unregisterProcedure(int id)
{
  beginRemoveRows(QModelIndex(), rows_, rows_);
  std::map<int, Procedure*> tmp;
  int row2remove(0), i(0);
  {
    assert(!procedures_lock_.try_lock());
    std::cout << "[ProcedureModel::unregisterProcedure] removing procedure " << id << " from model" <<std::endl;
    for (std::map<int, Procedure*>::iterator it = row2Procedure_.begin();
         it != row2Procedure_.end();
         it++) {
      if (!(*it).second->id_ == id) {
        tmp[i] = (*it).second;
        i++;
      } else {
        row2remove = (*it).first;
      }
    }
  }

  row2Procedure_.swap(tmp);
  --rows_;
  std::cout << "[ProcedureModel::unregisterProcedure] row2remove: " << row2remove << std::endl;
  removeRow(row2remove);
  endRemoveRows();
}

int ProcedureModel::rowCount(const QModelIndex &parent) const
{
  return rows_;
}

int ProcedureModel::columnCount(const QModelIndex &parent) const
{
  return COLUMNS;
}

QVariant ProcedureModel::data(const QModelIndex &index, int role) const
{
  boost::shared_lock<boost::shared_mutex> lock(procedures_lock_);
//  Procedure* proc = procedures_->at(row2Procedure_.at(index.row()));
//  std::cout << "[ProcedureModel::data] get procedure on row: " << index.row() <<std::endl;

 std::map<int, Procedure*>::const_iterator it = row2Procedure_.find(index.row());

  if (it != row2Procedure_.end())
  {
    Procedure* proc = (*it).second;
    switch(role) {
      case Qt::DisplayRole:
            switch(index.column()) {
              case 0:
                return proc->color_;
              case 1:
                return proc->name_;
            }
        break;
      case Qt::BackgroundRole:
        switch(index.column()) {
          case 0:
            return QBrush(proc->color_);
        }
        break;
      case Qt::CheckStateRole:
        switch(index.column()) {
          case 2:
            return proc->visible_ ? Qt::Checked : Qt::Unchecked;
        }
        break;
    }
  }

  return QVariant();
}

QVariant ProcedureModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
          return QString("color");
      case 1:
          return QString("Proc Name");
      case 2:
          return QString("Visible");
      }
    }
  }

  return QVariant();
}

bool ProcedureModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  boost::shared_lock<boost::shared_mutex> lock(procedures_lock_);
  switch (role) {
    case Qt::CheckStateRole:
      row2Procedure_.at(index.row())->visible_ = value.toBool();
  }

  return true;
}

Qt::ItemFlags ProcedureModel::flags(const QModelIndex &index) const
{
  switch(index.column()) {
    case 0:
      return Qt::ItemIsEnabled;
    case 2:
      return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
  }
  return Qt::ItemFlags();
}

Procedure* ProcedureModel::getProcedure(int row_index)
{
  assert(!procedures_lock_.try_lock());
  try {
    return row2Procedure_.at(row_index);
  } catch (std::out_of_range e) {
    std::cout << "ProcedureModel::getProcedure: accessing row_index " << row_index << " results in out of range: " << e.what() << std::endl;
    assert(false);
  }
  return 0;
}
