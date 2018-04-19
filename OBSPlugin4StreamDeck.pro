#-------------------------------------------------
#
# Project created by QtCreator 2016-11-28T14:58:54
#
#-------------------------------------------------

QT       += widgets

TARGET   = StreamDeckPlugin
TEMPLATE = lib

DEFINES += OBSPLUGINSTREAMDECK_LIBRARY

SOURCES += \
    streamdeckplugin_module.cpp \
    infodialog.cpp \
    actionhelp.cpp \
    ipc_thread.cpp

HEADERS +=\
    obspluginstreamdeck_global.h \
    streamdeckplugin_module.h \
    infodialog.h \
    actionhelp.h \
    ipc_thread.h

CONFIG += c++11

VERSION = 2.3.1

QMAKE_TARGET_DESCRIPTION = "Stream Deck OBS Plugin"
QMAKE_TARGET_COMPANY = "Elgato Systems GmbH"
QMAKE_TARGET_COPYRIGHT = "Copyright \\251 2017, Elgato Systems GmbH"

DEFINES += VERSION_STR=\\\"$$VERSION\\\"

include(./SharedFile/SharedFile.pri)   # IPC
INCLUDEPATH += ./SharedFile/

INCLUDEPATH += $$PWD/obs-dev/headers/libobs/ \
				$$PWD/obs-dev/headers/UI/

win32 {
	
    CONFIG += dynamiclib
    
	LIBS += -L$$PWD/obs-dev/bin/win \
			-lobs \
			-lobs-frontend-api


} else: macx {
	
    CONFIG += plugin
    QMAKE_LFLAGS_PLUGIN -= -dynamiclib
    QMAKE_LFLAGS_PLUGIN += -bundle
    QMAKE_EXTENSION_SHLIB = so
    
    LIBS += -L$$PWD/obs-dev/bin/mac \
            -lobs \
            -lobs-frontend-api
}

FORMS += infodialog.ui
