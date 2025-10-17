QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

OPENSSL_PATH = C:/OpenSSL-1.1.1-Win32/

INCLUDEPATH += $$OPENSSL_PATH/include

# ВАЖНО: Явно указываем полные имена .lib файлов
LIBS += $$OPENSSL_PATH/lib/libcrypto.lib \
        $$OPENSSL_PATH/lib/libssl.lib

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    authdialog.cpp \
    cryptographymanager.cpp \
    database.cpp \
    main.cpp \
    mainwindow.cpp \
    networkmanager.cpp

HEADERS += \
    authdialog.h \
    cryptographymanager.h \
    database.h \
    mainwindow.h \
    networkmanager.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
