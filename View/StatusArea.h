#ifndef STATUSAREA_H
#define STATUSAREA_H

#include <map>

#include <QWidget>
#include <QTableView>
#include <QLabel>
#include <QVector>

#include "Data/ProcedureModel.h"

class StatusArea : public QWidget
{
      Q_OBJECT

  public:
    StatusArea(boost::shared_mutex& procedures_lock, QWidget *parent = 0);

    ~StatusArea();

    QSize minimumSizeHint() const;

    QSize sizeHint() const;

    void setGpuInfo();

    void registerProcedure(Procedure *proc);

    void unregisterProcedure(int id);

  private slots:
    void updateTableViewSize();

    /**
     * @brief tableViewDoubleClickEvent is called when a double click is registered on one of the
     * tables
     * @param index of the table which was clicked
     */
    void tableViewDoubleClickEvent(const QModelIndex& index);

  signals:
    void updateGpuInfoSignal();

    void updateTableViewSizeSignal();

  private:
    QLabel* gfxLabel_;
    QLabel* gfxNameLabel_;
    QLabel* gfxCCLabel_;
    QLabel* gfxCCVersionLabel_;
    QTableView table_view_;
    ProcedureModel proc_model_;
};

#endif // STATUSAREA_H
