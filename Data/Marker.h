#ifndef MARKER_H
#define MARKER_H

#include <QGraphicsScene>

#include <stdint.h>
#include <boost/atomic.hpp>

static int triangle_size_ = 2;

class Marker
{
  public:

    Marker(int pixels_per_second);

    /**
     * @brief setMarker to set the marker
     * @param pos where to set the marker
     */
    void setMarker(qreal pos);

    /**
     * @brief drawMarker to given scene with given scale
     * @param scene to draw marker at
     * @param scale factor for drawing
     */
    void drawMarker(QGraphicsScene* scene, qreal scale, qreal min_x);

    /**
     * @brief getTime
     * @return time offset of marker according to pos_ / points_per_update_ * update_intervall_
     */
    float getTime();

    void updatePosition();

      private:
    qreal points_per_update_; ///< determine the change of position for each update
    qreal pos_; ///< start time according to "second" axis in render area
    uint64_t set_at_tick;
    uint64_t offset_from_marker_;
    QGraphicsScene *prev_scene_;
    QGraphicsPolygonItem* poly_;

    QGraphicsPolygonItem *drawRectangle(QGraphicsScene* scene, qreal scale, qreal pos);

    uint64_t getBaseTimeLookupTableIndex();
};

#endif // MARKER_H
