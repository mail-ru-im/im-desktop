#-------------------------------------------------
#
# Project created by QtCreator 2015-12-27T23:44:05
#
#-------------------------------------------------

QT += widgets

TARGET = updater
TEMPLATE = app

SOURCES += main.cpp

CONFIG += 32

QMAKE_LIBS_THREAD = -lxcb-util -lffi -lpcre -lexpat -lXext -lXau -lXdmcp -lz -Wl,-Bdynamic -ldl -lpthread -lX11

QMAKE_LFLAGS += -Wl,-Bstatic -static-libgcc -static-libstdc++
QMAKE_CXXFLAGS += -std=c++0x

CONFIG(64, 64|32) {
    QMAKE_LFLAGS += -L/x64
    RESOURCES = resource64.qrc
} else {
    QMAKE_LFLAGS += -L/x32
    RESOURCES = resource32.qrc
}
