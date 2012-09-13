TEMPLATE = app
CONFIG += console
CONFIG -= qt

LIBS += -ldialog -lncursesw

SOURCES += main.cpp \
    berryboot.cpp \
    mainwindow.cpp

HEADERS += \
    berryboot.h \
    mainwindow.h

