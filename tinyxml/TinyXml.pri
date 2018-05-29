INCLUDEPATH += $$PWD

DEFINES += TIXML_USE_STL

SOURCES += \
    $$PWD/tinystr.cpp \
    $$PWD/tinyxml.cpp \
    $$PWD/tinyxmlerror.cpp \
    $$PWD/tinyxmlparser.cpp \
    $$PWD/xmltest.cpp
		   
HEADERS += \
    $$PWD/tinystr.h \
    $$PWD/tinyxml.h
		   
OTHER_FILES += \
    $$PWD/readme.txt \
	$$PWD/changes.txt
		  