#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QFrame>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPoint>
#include <QPolygon>
#include <QVector>

#include <map>
#include <utility>
#include <vector>
#include <string>
#include <atomic>

#include <boost/thread.hpp>

#include "Data/Procedure.h"
#include "Data/Marker.h"

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSlider)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class RenderArea;

class GraphicsView : public QGraphicsView
{
     Q_OBJECT
  public:
     GraphicsView(RenderArea *v) : QGraphicsView(), view(v), click_counter_(true) { }

  protected:
     void wheelEvent(QWheelEvent *);
     bool viewportEvent(QEvent* event);
     void mousePressEvent(QMouseEvent *e);
     void mouseReleaseEvent(QMouseEvent *e);

  private:
     RenderArea *view;

     bool click_counter_;
     qreal checkMaxXVal(qreal x);

     QPointF start_point_;
     QPointF end_point_;
};


class RenderArea : public QFrame
{
    Q_OBJECT
  private:
    int points_per_update_; /// determines how many pixels gets painted each update

  public:
      Marker marker_;
      Marker start_marker_;
      Marker end_marker_;
      std::atomic<float> selected_area_in_ms_;

      RenderArea(const QString &name, QWidget *parent = 0);

      ~RenderArea();

      QGraphicsView *view() const;

      QGraphicsScene* scene_;

      void addProcedureWorkloads(std::map<int, long double>& procedure_workloads, std::map<int, char *>& procedure_names);

      void update();

      Procedure* getProcedure(int id);

      Procedure* getOverallProcedure();

      __inline QVector<Procedure*>* getProcedures() {return &procedures_;}

      void saveData();

      void clearProcedures();

      boost::shared_mutex& getProcedureLock() {return procedures_lock_;}

      void drawCoordinates(int x = 0, QGraphicsScene* scene = 0);

  public slots:
      void zoomIn(int level = 1);

      void zoomOut(int level = 1);

  signals:
      void updateSceneSignal();
      void updateSceneCoordinatesSignal();

  private slots:
      void resetView();

      void setResetButtonEnabled();

      void setupMatrix();

      void togglePointerMode();

      void toggleOpenGL();

      void toggleAntialiasing();

      void print();

      void updateScene();

      void updateSceneCoordinates();

      void togglePause();

  private:
      GraphicsView *graphicsView;
      QLabel *label;
      QLabel *label2;
      QToolButton *selectModeButton;
      QToolButton *dragModeButton;
      QToolButton *pauseButton;
      QToolButton *openGlButton;
      QToolButton *antialiasButton;
      QToolButton *printButton;
      QToolButton *resetButton;
      QSlider *zoomSlider;

      Procedure* overall_procedure_;
      boost::shared_mutex procedures_lock_;
      QVector<Procedure*> procedures_;
      qreal scale_;

      void addOverallWorkload(long double y);

      QPolygon checkViewCoordinates(QPolygon p);

      void export2SVG();
};

#endif // RENDERAREA_H
