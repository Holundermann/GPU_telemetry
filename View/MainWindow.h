#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QPoint>
#include <QAction>

#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>

#include <vector>
#include <map>
#include <list>

#include <View/RenderArea.h>
#include <View/StatusArea.h>
#include <View/BlockArea.h>
#include <View/ConnectionDialog.h>
#include <Data/IODataTypes.h>
#include <Data/Marker.h>

#define STATUSBAR_UPDATE_INTERVALL 1000

class Procedure;

class MainWindow : public QMainWindow
{
    Q_OBJECT
  private:
    RenderArea* render_area_;
    StatusArea* status_area_;
    BlockArea* block_area_;
    ConnectionDialog* connection_dialog_;
    QWidget* central_widget_;
    QPoint point_;
    boost::circular_buffer<parsed_data>* parsed_data_rb_;
    boost::mutex parsed_data_rb_lock_;
    int rb_size_;
    int rb_size_counter_;
    boost::mutex base_time_lookup_table_lock_; ///< this mutex also locks update_counter! Has to get locked after parsed_data_rb_lock!
    std::list<float> base_time_lookup_table_;
    uint64_t update_counter_;
    //-------TOOLBAR----------
    QToolBar* tb_std_toolbar_;
    QAction* new_connection_;
    QAction* save_svg_;
    QAction* save_binary_;
    QAction* block_diagram_;

    QLabel* sb_connected_;
    QLabel* sb_speed_;
    QLabel* sb_area_in_ms_;

    MainWindow();

    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void setupCentralWidget();
    QString getSelectedAreaInMS();

  private slots:
    void updateStatusBar();
    void newConnection();
    void saveDiagram();
    void saveBinary();
    void newBlockDiagram();

  public:
    static MainWindow* instance_;
    static boost::mutex instance_lock_;

    ~MainWindow();

    static void createInstance();

    void addPerformanceData(std::map<int, long double>& procedure_workloads, std::map<int, char *> &procedure_names, float base_time);

    void setGpuInfo();

    void registerProcedure(Procedure* proc);

    void clearProcedures();

    void unregisterProcedure(int id);

    void setupRingBuffer(uint64_t size);

    void addElement2RB(parsed_data data);

    __inline boost::circular_buffer<parsed_data>* getRingBuffer() {return parsed_data_rb_;}

    __inline boost::mutex& getRBLock(){return parsed_data_rb_lock_;}

    uint64_t getTickCounter();

    size_t getBaseTimeLookupTableSize();

    float getBaseTime(uint64_t index);

    Procedure* getProcedure(int id);

    Procedure* getOverallProcedure();

    Marker* getStartMarker();
    Marker* getEndMarker();

    float getMaxTime();

    QSize sizeHint() const;

    void resetSelectedArea();
};

#endif // MAINWINDOW_H
