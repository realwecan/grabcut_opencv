TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp

LIBS += \
    -lopencv_core -lopencv_highgui -lopencv_imgproc -lml \
    -L/usr/lib

INCLUDEPATH += \
    /usr/include
