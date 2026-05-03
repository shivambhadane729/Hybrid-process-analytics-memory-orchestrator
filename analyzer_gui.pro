QT += core gui widgets

CONFIG += c++17
CONFIG += console    # Keep console open for backend cout messages

TARGET = analyzer_gui
TEMPLATE = app

INCLUDEPATH += .

SOURCES += \
    main.cpp \
    gui/mainwindow.cpp \
    gui/dashboard_tab.cpp \
    gui/storage_tab.cpp \
    gui/ds_visualizer.cpp \
    gui/fault_tab.cpp

HEADERS += \
    data_structures.h \
    i_process_collector.h \
    linux_process_collector.h \
    analyzer.h \
    storage_engine.h \
    fault_monitor.h \
    gui/mainwindow.h \
    gui/dashboard_tab.h \
    gui/storage_tab.h \
    gui/ds_visualizer.h \
    gui/fault_tab.h \
    gui/styles.h

win32 {
    DEFINES += NOMINMAX WIN32_LEAN_AND_MEAN
    HEADERS += windows_process_collector.h
    LIBS += -lpsapi -lshell32 -luser32 -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lwinmm -lws2_32 -lole32 -luuid -ladvapi32
}

linux {
    LIBS += -lpthread
}
