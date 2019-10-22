QT       += multimedia widgets

TARGET = openroll
TEMPLATE = app
VERSION = 2.0.1

CONFIG += c++11
CONFIG(debug) {
    # adds -DLOGGER to preprocessor so we can use it in ifdef in source
    DEFINES += LOGGER
}

SOURCES += \
        logger.cpp \
        main.cpp \
        controls.cpp \
        scoreboard.cpp
EXTRAS = \
    AUTHORS
    CHANGELOG
    gpl.txt
    lgpl.txt
    LICENSES.txt
    README.md

HEADERS += \
    controls.h \
    logger.h \
    openroll-config.h \
    scoreboard.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    controls.ui \
    scoreboard.ui

RESOURCES += \
    resources.qrc

win32 {
    RC_ICONS = openroll.ico
    QMAKE_TARGET_COMPANY = "Barker Software"
    QMAKE_TARGET_DESCRIPTION = "Brazilian jiu-jitsu match timer and scoreboard using IBJJF ruleset."
    QMAKE_TARGET_COPYRIGHT = 2019
    QMAKE_TARGET_PRODUCT = "Openroll"
    CONFIG(debug) {
        CONFIG += console
    }
}

osx {
    # Same, but for Apple, use osx tool to convert .ico to .icns file first
    ICON = openroll.icns
    CONFIG += app_bundle
}