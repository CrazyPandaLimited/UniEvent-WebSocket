TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += src ../CPP-panda-lib/src ../Panda-WebSocket/src ../Panda-Event/src ../Panda-XS/src \
               ../Panda-Encode-Base2N/src

SOURCES += \
    src/panda/websocket/server/Connection.cc \
    src/panda/websocket/server/Listener.cc \
    src/panda/websocket/server/Server.cc \
    src/panda/websocket/server/utils.cc \
    src/xs/websocket/server/XSServer.cc \
    src/panda/websocket/Client.cc \
    src/panda/websocket/BaseConnection.cc

HEADERS += \
    src/panda/websocket/server/Connection.h \
    src/panda/websocket/server/Listener.h \
    src/panda/websocket/server/Server.h \
    src/panda/websocket/server/utils.h \
    src/panda/websocket/server.h \
    src/xs/websocket/server/XSServer.h \
    src/xs/websocket/server.h \
    src/xs/websocket/server/XSConnection.h \
    src/panda/websocket/Client.h \
    src/panda/websocket/BaseConnection.h

DISTFILES += \
    Server.xs \
    Server.xsi \
    Connection.xsi \
    typemap \
    Client.xsi \
    Makefile.PL

