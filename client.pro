#-------------------------------------------------
#
# Project created by QtCreator 2014-03-06T13:19:38
#
#-------------------------------------------------

QT += core gui
QT += svg
QT += opengl
QT += printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = client
TEMPLATE = app

wine32: DEFINES += WIN32 \
                   _WIN32 \
                   WIN32_LEAN_AND_MEAN \
                   NOMINMAX

SOURCES += main.cpp \
    View/RenderArea.cpp \
    View/StatusArea.cpp \
    Data/ProcessData.cpp \
    Data/ClientManager.cpp \
    Data/Network/TcpClient.cpp \
    Data/Client.cpp \
    View/MainWindow.cpp \
    View/ConnectionDialog.cpp \
    Data/Procedure.cpp \
    Data/ProcedureModel.cpp \
    Data/Marker.cpp \
    View/BlockArea.cpp \
    Data/HandleConfigFile.cpp

HEADERS  += Data/IODataTypes.h \
    View/RenderArea.h \
    View/StatusArea.h \
    Data/ProcessData.h \
    Data/ClientManager.h \
    Data/Network/TcpClient.h \
    Data/Client.h \
    View/MainWindow.h \
    View/ConnectionDialog.h \
    Data/Procedure.h \
    Data/ProcedureModel.h \
    Data/Marker.h \
    View/BlockArea.h \
    Data/HandleConfigFile.h

win32:CONFIG(release, debug|release): LIBS += -LC:\Boost\lib -lboost_system-vc110-mt-1_55 -lboost_thread-vc110-mt-1_55 -lWS2_32
else:win32:CONFIG(debug, debug|release): LIBS += -LC:\Boost\lib -lboost_system-vc110-mt-gd-1_55 -lboost_thread-mgw48-mt-gd-1_55 -lWS2_32
else:unix: LIBS += -lboost_system -lpthread -lboost_thread

win32:INCLUDEPATH += C:\Boost\include\boost-1_55 \
                     .\

QMAKE_CXXFLAGS += -std=c++11

RESOURCES += resources.qrc
