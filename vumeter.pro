#-------------------------------------------------
#
# Project created by QtCreator 2010-12-22T16:22:50
#
#-------------------------------------------------

QT       += core gui
QT       += opengl

TARGET = vumeter
TEMPLATE = app


SOURCES += main.cpp\
        vumeter.cpp \
    alsalisten.cpp \

HEADERS  += vumeter.h \
    alsalisten.h \

LIBS += -lasound -lrfftw -lfftw -lm
