#-------------------------------------------------
#
# Project created by QtCreator 2012-07-20T08:03:26
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BerrybootInstaller
TEMPLATE = app

LIBS += -lcurl -lssl -lcrypto -lz -lblkid

RPI_USERLAND_DIR=../../staging/usr
exists($${RPI_USERLAND_DIR}/include/interface/vmcs_host/vc_cecservice.h) {
    INCLUDEPATH += $${RPI_USERLAND_DIR}/include $${RPI_USERLAND_DIR}/include/interface/vcos/pthreads
    LIBS += -lbcm_host -lvcos -lvchiq_arm -lvchostif -L$${RPI_USERLAND_DIR}/lib
    DEFINES += RASPBERRY_CEC_SUPPORT
} else {
    message(Disabling CEC support for Raspberry Pi - rpi-userland headers not found at $${RPI_USERLAND_DIR})
}

exists($${RPI_USERLAND_DIR}/include/wpa_ctrl.h) {
    LIBS += -lwpa_client
    DEFINES += WPA_CLIENT
    SOURCES += statusdialog.cpp
    HEADERS += statusdialog.h
    FORMS += statusdialog.ui
} else {
    message(Not building wifi status dialog - wpa_ctrl.h not found)
}

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
    driveformatthread.cpp \
    copythread.cpp \
    exportdialog.cpp \
    iscsidialog.cpp \
    wifidialog.cpp \
    confeditdialog.cpp \
    bootmenudialog.cpp \
    ceclistener.cpp \
    logindialog.cpp \
    berrybootsettingsdialog.cpp \
    downloadthread.cpp \
    twoiconsdelegate.cpp \
    wificountrydetector.cpp

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
    ceclistener.h \
    logindialog.h \
    berrybootsettingsdialog.h \
    downloadthread.h \
    twoiconsdelegate.h \
    wificountrydetector.h

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
    bootmenudialog.ui \
    logindialog.ui \
    berrybootsettingsdialog.ui

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    timezones.txt \
    country2keyboard.ini \
    countries.txt \
    country2timezone.ini
