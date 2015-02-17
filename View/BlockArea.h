#ifndef BLOCKAREA_H
#define BLOCKAREA_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QDialog>
#include <QToolBar>
#include <QProgressDialog>

#include <map>

#include <boost/circular_buffer.hpp>
#include <boost/thread.hpp>

#include "Data/IODataTypes.h"
#include "Data/Procedure.h"

class BlockArea;

class BlockView: public QGraphicsView
{
  public:
     BlockView(BlockArea *v) : QGraphicsView(), view(v) { }

  private:
     BlockArea* view;

  protected:
     void wheelEvent(QWheelEvent *);
     QSizeF sizeHint();
};

class BlockArea : public QDialog
{
    Q_OBJECT

  public:
    BlockArea(QWidget* parent = 0);

    virtual ~BlockArea();

    /**
     * @brief zoomIn scene_
     * @param v determines how far to zoom in
     */
    void zoomIn(qreal v);

    /**
     * @brief zoomOut scene_
     * @param v determines how far to zoom out
     */
    void zoomOut(qreal v);

    /**
     * @brief createView calls populateScene and sets scene to view
     */
    void createView();


private slots:
    /**
     * @brief export2SVG exports the populated scene to svg
     */
    void export2SVG();


  private:
    BlockView* block_view_; ///< view to display scene
    QGraphicsScene scene_; ///< scene to draw data to
    qreal y_pos_; ///< gives the offset where every object gets relative drawn to
    qreal y_pos_heading_; ///< y position of header
    qreal x_offset_; ///< x offset
    qreal x_endpos_; ///< end position of x outline
    QPen procedure_pen_; ///< pen which is used for drawing the rectangles of procedures
    QPen outline_pen_; ///< pen which is used to draw the outline
    QPen inline_pen_; ///< pen which is used to draw the inline/dotted lines
    QFont label_font_; ///< font which is used by the axis labels
    std::map<int, qreal> smx_offset_; ///< offset for the smx processor with id (int = id, qreal = offset)
    std::map<int, qreal> smx_offset_hard_; ///< offset for each smx processor, used for boundary check
    gpu_info* gpu_info_; ///< containing information about the used gpu (smx count, threads/smx,...)
    qreal scale_; ///< which is set by scaling the view (mouse wheel...) zoomIn/zoomOut
    qreal std_scale_x_; ///< scaling which is used for horizontal drawing (# threads * std_scale_x_ = x width)
    qreal std_scale_y_; ///< scaling which is used for vertical drawing (# clockcycles * std_scale_y_ = y height)
    uint64_t start_time_; ///< start time of first executed procedure in this iteration
    uint64_t end_time_; ///< end time of last executed procedure in this iteration
    int max_items2draw; ///< determines how many frames the blockdiagram should display
    std::vector<Procedure*> procedures_; ///< for drawing procedure legend
    uint64_t item_counter_; ///< to determine how many items have been drawn
    std::map<int, QList<QGraphicsItem*> > colliding_items_per_mp_; ///< for collision dedection, only check items which can collide (items on the same multiprocessor)
    QProgressDialog* progress_dialog_;

    //-------TOOLBAR----------//
    QToolBar* tb_std_toolbar_;
    QAction* save_;

    /**
     * @brief updateQApplication triggers update for QApplication that window events get handled + drawn
     * @param progress bar to update for each step
     */
    void updateQApplication(int item_count);

    /**
     * @brief drawProcedures iterates over procedures between it and end it and draws them to the scene
     * @param end_it up to draw procedures to
     * @param it start iterator
     * @param progress progress dialog to check for abort
     */
        void drawProcedures(boost::circular_buffer<parsed_data>::iterator it, boost::circular_buffer<parsed_data>::iterator end_it);

    /**
     * @brief drawProcedure draws given procedure to scene
     * @param data contains data to draw
     */
    void drawProcedure(parsed_data& data);

    /**
     * @brief drawTimeLine to given time
     * @param time to draw the time line in respect to offset_position_
     */
    void drawTimeLine(uint64_t time, float curr_time);

    /**
     * @brief drawSMXProcessors draws the smx processors of used graphics card to the top of the scene
     */
    void drawSMXProcessors();

    /**
     * @brief drawVerticalLines from each SMX processor to the end_position_
     */
    void drawVerticalLines();

    /**
     * @brief drawBottomLine draws a horizontal line at the bottom at position end_position_
     */
    void drawBottomLine();

    /**
     * @brief drawLegend draws procedure color + name
     */
    void drawLegend();

    /**
     * @brief resetSMXOffset resets values in smx_offset_ map
     */
    void resetSMXOffset();

    /**
     * @brief setupMatrix used for scaling the view
     * @param scale scale factor which is added to scale_ and the matrix gets scaled accordingly
     */
    void setupMatrix(qreal scale);

    /**
     * @brief resetValues resets variables to initiale values
     */
    void resetValues();

    /**
     * @brief populateScene populates scene with the data from the ringbuffer (draws the procedures onto the scene)
     */
    void populateScene();

    void createToolBar();

    void reject();

    /**
     * @brief getFirstIterator returns first iterator with given time offset from last base time
     * @param time offset
     * @return iterator for given time
     */
    boost::circular_buffer<parsed_data>::iterator getIteratorFromTimestamp(float search_time);

    QList<QGraphicsItem*> checkCollision(QGraphicsRectItem* item, int mpid);

    int sortParsedDataTime(boost::circular_buffer<parsed_data>::iterator iterator, boost::circular_buffer<parsed_data>::iterator iterator_end, boost::circular_buffer<parsed_data> *rb, boost::mutex &rb_lock);

};

#endif // BLOCKAREA_H
