#-------------------------------------------------
#
# Project created by QtCreator 2023-02-01T20:55:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network
QT += multimedia

TARGET = wifi_mic_player
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_NO_COMPRESS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += embed_manifest_exe QMAKE_LFLAGS_WINDOWS += $$quote( /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\" )

#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

CONFIG += c++11

SOURCES += \
        main.cpp

HEADERS += \
    mainwindow.h \
    wav_header.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    mainwindow.ui


win32:{
    RC_ICONS = $$PWD/mic_wifi.ico
    VERSION = 1.0.0
#    QMAKE_TARGET_COMPANY = Название компании
#    QMAKE_TARGET_PRODUCT = Название программы
#    QMAKE_TARGET_DESCRIPTION = Описание программы
#    QMAKE_TARGET_COPYRIGHT = Автор
}
win32: {
#x86
INCLUDEPATH += $$PWD/OpenSSL/Win_x86/include/
LIBS += -L$$PWD/OpenSSL/Win_x86/bin -llibcrypto-1_1 -llibssl-1_1
#x64
#INCLUDEPATH += $$PWD/OpenSSL/Win_x64/include/
#LIBS += -L$$PWD/OpenSSL/Win_x64/bin -llibcrypto-1_1-x64 -llibssl-1_1-x64

# for static build x86
#LIBS += -L$$PWD/OpenSSL/Win_x86/bin -llibssl -llibcrypto
}

android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/res/values/libs.xml

    INCLUDEPATH += $$PWD/OpenSSL/Win_x86/include/
    LIBS += -L$$PWD/OpenSSL/arm -lssl -lcrypto
    ANDROID_EXTRA_LIBS += $$PWD/OpenSSL/arm/libssl_1_1.so
    ANDROID_EXTRA_LIBS += $$PWD/OpenSSL/arm/libcrypto_1_1.so
}

RESOURCES += \
    images.qrc
