#-------------------------------------------------
#
# Project created by QtCreator 2012-07-20T08:03:26
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BerrybootInstaller
TEMPLATE = app

LIBS += -lcrypto

INCLUDEPATH += ../../staging/opt/vc/include ../../staging/opt/vc/include/interface/vcos/pthreads
LIBS += -lbcm_host -lvcos -lvchiq_arm -L../../staging/opt/vc/lib

SOURCES += main.cpp\
        mainwindow.cpp \
    diskdialog.cpp \
    installer.cpp \
    adddialog.cpp \
    logviewer.cpp \
    editdialog.cpp \
    clonedialog.cpp \
    localedialog.cpp \
    greenborderdialog.cpp \
    syncthread.cpp \
    downloaddialog.cpp \
    networksettingsdialog.cpp \
    parsedate.c \
    driveformatthread.cpp \
    copythread.cpp \
    exportdialog.cpp \
    iscsidialog.cpp \
    wifidialog.cpp \
    confeditdialog.cpp \
    bootmenudialog.cpp \
    ceclistener.cpp

HEADERS  += mainwindow.h \
    diskdialog.h \
    installer.h \
    adddialog.h \
    logviewer.h \
    editdialog.h \
    clonedialog.h \
    localedialog.h \
    greenborderdialog.h \
    syncthread.h \
    downloaddialog.h \
    networksettingsdialog.h \
    driveformatthread.h \
    copythread.h \
    exportdialog.h \
    iscsidialog.h \
    wifidialog.h \
    confeditdialog.h \
    bootmenudialog.h \
    ceclistener.h

FORMS    += mainwindow.ui \
    diskdialog.ui \
    adddialog.ui \
    logviewer.ui \
    editdialog.ui \
    clonedialog.ui \
    localedialog.ui \
    greenborderdialog.ui \
    downloaddialog.ui \
    networksettingsdialog.ui \
    exportdialog.ui \
    iscsidialog.ui \
    wifidialog.ui \
    confeditdialog.ui \
    bootmenudialog.ui

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    timezones.txt \
    country2keyboard.ini
