TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += \
    verlet.h

SOURCES += \
    main.cpp \
    verlet.cpp

linux-*{
    LIBS += -lglut -lGL -lGLU -lm

    DEFINES     += _LINUX_
}

win32{
    DEFINES  += _WIN32_
}

