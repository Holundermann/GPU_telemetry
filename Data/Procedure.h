#ifndef PROCEDURE_H
#define PROCEDURE_H

#include <QPolygon>
#include <QString>
#include <QColor>
#include <QGraphicsScene>

#define MAX_DATA_POINTS 10000

class Procedure
{
  public:
    int id_;
    QString name_;
    QColor color_;
    QPolygon workload_;
    bool visible_;
    int max_data_points_;
    int points_per_update_;
    int update_intervall_;

    Procedure(int id, int points_per_update, int update_intervall, QString name = "Procedure", bool append_id_to_name = true);
    ~Procedure();

    void addWorkload(long double workload);

    __inline void setColor(QColor color) {color_ = color;}

    __inline void setMaxDataPoints(int v) {max_data_points_ = v;}

  private:
    Procedure(const Procedure& tocpy);
};

#endif // PROCEDURE_H
