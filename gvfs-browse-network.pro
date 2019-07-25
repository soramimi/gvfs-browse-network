
TARGET = gvfs-browse-network
TEMPLATE = app
CONFIG += console
CONFIG -= qt app_bundle
INCLUDEPATH += . /usr/include/glib-2.0 /usr/lib/x86_64-linux-gnu/glib-2.0/include

DESTDIR = $$PWD/_bin
LIBS += -lglib-2.0 -lgio-2.0 -lgobject-2.0

SOURCES += main.c
