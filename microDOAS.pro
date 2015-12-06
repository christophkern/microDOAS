TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    cconfig.cpp \
    cspectrometer.cpp \
    data-collection.cpp \
    gpsparser.cpp \
    leds.cpp \
    tinystr.cpp \
    tinyxml.cpp \
    tinyxmlerror.cpp \
    tinyxmlparser.cpp \
    toolbox.cpp \
    util.cpp \
    BBBiolib.c \
    radiomanager.cpp \
    main.cpp

DISTFILES += \
    dhcpip.sh \
    microDOAS.sh \
    start_microDOAS.sh \
    start_x11vnc.sh \
    staticip.sh \
    microDOAS.xml \
    led810blink.py \
    led812blink.py \
    led814blink.py \
    led814doubleblink.py \
    led816blink.py \
    switch818.py

HEADERS += \
    BBBiolib.h \
    cconfig.h \
    cspectrometer.h \
    globals.h \
    tinystr.h \
    tinyxml.h \
    util.h \
    radiomanager.h

