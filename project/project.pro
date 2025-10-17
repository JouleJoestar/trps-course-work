QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# ===================================================================
# Конфигурация для OpenSSL 1.1.1 (Indy Sockets MSVC 64-bit)
# ===================================================================

# Главная папка, куда вы все распаковали
OPENSSL_DIR = C:/openssl-1.1/

# Путь к папке include для 64-битной версии
INCLUDEPATH += $$OPENSSL_DIR/x64/include

# Путь к папке lib для 64-битной версии и сами библиотеки
LIBS += -L$$quote($$OPENSSL_DIR/x64/lib) -llibcrypto -llibssl

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
