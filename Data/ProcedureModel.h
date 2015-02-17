#ifndef PROCEDUREMODEL_H
#define PROCEDUREMODEL_H

#include <map>

#include <QAbstractTableModel>

#include <boost/thread.hpp>

#include "Data/Procedure.h"

#define COLUMNS 3

class ProcedureModel : public QAbstractTableModel
{
      Q_OBJECT

  public:
    ProcedureModel(boost::shared_mutex& procedures_lock, QObject* parent = 0);

    int rowCount(const QModelIndex& parent=  QModelIndex()) const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool setData(const QModelIndex & index, const QVariant & value, int role);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void registerProcedure(Procedure* id);

    void unregisterProcedure(int id);

    Procedure* getProcedure(int row_index);

    boost::shared_mutex& getProceduresLock() {return procedures_lock_;}

  private:
    boost::shared_mutex& procedures_lock_;
    std::map<int, Procedure*> row2Procedure_;
    int rows_;
};

#endif // PROCEDUREMODEL_H
