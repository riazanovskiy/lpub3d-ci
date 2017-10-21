#####################################################################################
# Automatically generated by qmake (2.01a) Tue 2. Dec 10:46:50 2014
#####################################################################################
QT        += core gui opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT         += widgets

TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= printsupport
}

include(../gitversion.pri)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TARGET +=
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../lc_lib/common ../lc_lib/qt ../ldrawini ../ldglite

# If quazip is alredy installed you can suppress building it again by
# adding CONFIG+=quazipnobuild to the qmake arguments
# Update the quazip header path if not installed at default location below
quazipnobuild {
    INCLUDEPATH += /usr/include/quazip
} else {
    INCLUDEPATH += ../quazip
}

macx {
    CONFIG += c++11
} else {
    lessThan(QT_MAJOR_VERSION, 5): CONFIG += c++11
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

contains(QT_ARCH, x86_64) {
    ARCH = 64
    STG_ARCH = x86_64
} else {
    ARCH = 32
    STG_ARCH = x86
}

CONFIG += precompile_header
PRECOMPILED_HEADER += ../lc_lib/common/lc_global.h
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Winvalid-pch

win32 {

    DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE=1 _CRT_NONSTDC_NO_WARNINGS=1
    QMAKE_EXT_OBJ = .obj
    PRECOMPILED_SOURCE = ../lc_lib/common/lc_global.cpp
    CONFIG += windows
    LIBS += -ladvapi32 -lshell32
    greaterThan(QT_MAJOR_VERSION, 4): LIBS += -lz -lopengl32

    QMAKE_TARGET_COMPANY = "LPub3D Software"
    QMAKE_TARGET_DESCRIPTION = "An LDraw Building Instruction Editor."
    QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2015-2017 Trevor SANDY"
    QMAKE_TARGET_PRODUCT = "LPub3D ($$join(ARCH,,,bit))"
    RC_LANG = "English (United Kingdom)"
    RC_ICONS = "lpub3d.ico"

} else {

    LIBS += -lz
    # Use installed quazip library
    quazipnobuild: LIBS += -lquazip
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

lessThan(QT_MAJOR_VERSION, 5) {
    unix {
        GCC_VERSION = $$system(g++ -dumpversion)
        greaterThan(GCC_VERSION, 4.6) {
            QMAKE_CXXFLAGS += -std=c++11
        } else {
            QMAKE_CXXFLAGS += -std=c++0x
        }
     }
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

unix:!macx: TARGET = lpub3d
else: TARGET = LPub3D
STG_TARGET   = $$TARGET

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Note on x11 platforms you can also pre-install install quazip ($ sudo apt-get install libquazip-dev)
# If quazip is already installed, set CONFIG+=quazipnobuld to use installed library

CONFIG(debug, debug|release) {
    message("~~~ MAIN_APP DEBUG build ~~~")
    DEFINES += QT_DEBUG_MODE
    DESTDIR = debug
    macx {
        LDRAWINI_LIB = LDrawIni_debug
        QUAZIP_LIB = QuaZIP_debug
    }
    win32 {
        LDRAWINI_LIB = LDrawInid161
        QUAZIP_LIB = QuaZIPd07
    }
    unix:!macx {
        LDRAWINI_LIB = ldrawinid
        QUAZIP_LIB = quazipd
    }
    # library target name
    LIBS += -L$$DESTDIR/../../ldrawini/debug -l$$LDRAWINI_LIB
    !quazipnobuild: LIBS += -L$$DESTDIR/../../quazip/debug -l$$QUAZIP_LIB
    # executable target name
    win32: TARGET = $$join(TARGET,,,d$$VER_MAJOR$$VER_MINOR)
} else {
    message("~~~ MAIN_APP RELEASE build ~~~")
    DESTDIR = release
    unix:!macx {
        LIBS += -L$$DESTDIR/../../ldrawini/release -lldrawini
        !quazipnobuild: LIBS += -L$$DESTDIR/../../quazip/release -lquazip
    } else {
        win32 {
            LIBS += -L$$DESTDIR/../../ldrawini/release -lLDrawIni161
            !quazipnobuild: LIBS += -L$$DESTDIR/../../quazip/release -lQuaZIP07
        } else {
            LIBS += -L$$DESTDIR/../../ldrawini/release -lLDrawIni
            !quazipnobuild: LIBS += -L$$DESTDIR/../../quazip/release -lQuaZIP
        }
    }
    !macx: TARGET = $$join(TARGET,,,$$VER_MAJOR$$VER_MINOR)
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static {                                     # everything below takes effect with CONFIG ''= static
    message("~~~ MAIN_APP STATIC build ~~~") # this is for information, that the static build is done
    CONFIG+= static
    LIBS += -static
    DEFINES += STATIC
    DEFINES += QUAZIP_STATIC                 # this is so the compiler can detect quazip static
    macx: TARGET = $$join(TARGET,,,_static)  # this adds an _static in the end, so you can seperate static build from non static build
    win32: TARGET = $$join(TARGET,,,s)       # this adds an s in the end, so you can seperate static build from non static build
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR     = $$DESTDIR/.moc
RCC_DIR     = $$DESTDIR/.qrc
UI_DIR      = $$DESTDIR/.ui

#~~file distributions~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Use these switches to enable/disable copying/install of 3rd party executables, documentation and resources.
# e.g. $ qmake "CONFIG+=copy3rdexe" "CONFIG+=copy3rdexeconfig" "CONFIG+=copy3rdcontent" "CONFIG+=stagewindistcontent"
# or you can hard code here:
# Copy 3rd party executables
!contains(CONFIG, copy3rdexe): CONFIG +=
# Copy 3rd party for executable configuration file(s)
!contains(CONFIG, copy3rdexeconfig): CONFIG +=
# Copy 3rd party documents and resources
!contains(CONFIG, copy3rdcontent): CONFIG +=
# Stage 3rd party executables, documentation and resources (Windows builds Only)
!contains(CONFIG, stagewindistcontent): CONFIG +=

unix: {
    # For linux and MacOS builds on Travis-CI - install 3rd party executables, documentation and resources.
    create_package = $$(LP3D_CREATE_PKG)
    contains(create_package, true) {
    message(~~~ CREATE DISTRIBUTION: $$create_package ~~~)
    CONFIG+=copy3rdexe
    CONFIG+=copy3rdexeconfig
    CONFIG+=copy3rdcontent
    }
}

# Download 3rd party repository when required
if(copy3rdexe|copy3rdexeconfig|copy3rdcontent|stagewindistcontent) {
    unix:!macx:REPO = lpub3d_linux_3rdparty
    macx:REPO       = lpub3d_macos_3rdparty
    win32:REPO      = lpub3d_windows_3rdparty
    !exists($$_PRO_FILE_PWD_/../../$$REPO/.gitignore) {
        REPO_NOT_FOUND_MSG = GIT REPOSITORY $$REPO was not found. It will be downloaded.
        REPO_DOWNLOADED_MSG = GIT REPOSITORY $$REPO downloaded.
        GITHUB_URL = https://github.com/trevorsandy
        QMAKE_POST_LINK += $$escape_expand(\n\t) \
                           echo $$shell_quote$${REPO_NOT_FOUND_MSG}
        win32 {
           QMAKE_POST_LINK += $$escape_expand(\n\t) \
                                $$escape_expand(\n\t) \
                                cd $$_PRO_FILE_PWD_/../../ \
                                $$escape_expand(\n\t) \
                                git clone $${GITHUB_URL}/$${REPO}.git \
                                $$escape_expand(\n\t) \
                                DIR $$_PRO_FILE_PWD_/../../ /S \
                                $$escape_expand(\n\t) \
                                CD
        } else {
           QMAKE_POST_LINK += $$escape_expand(\n\t) \
                                cd $$_PRO_FILE_PWD_/../../ \
                                && git clone $${GITHUB_URL}/$${REPO}.git \
                                && pwd \
                                && ls \
                                && cd ${REPO} \
                                && pwd \
                                && ls
        }
        QMAKE_POST_LINK += $$escape_expand(\n\t) \
                        echo $$shell_quote$${REPO_DOWNLOADED_MSG}
    }
}

VER_LDVIEW      = ldview-4.3
VER_LDGLITE     = ldglite-1.3
VER_POVRAY      = lpub3d_trace_cui-3.8

win32:include(winfiledistro.pri)
macx:include(macosfiledistro.pri)
unix:!macx:include(linuxfiledistro.pri)

#~~ includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include(../lc_lib/lc_lib.pri)
include(../qslog/QsLog.pri)
include(../qsimpleupdater/QSimpleUpdater.pri)
include(../LPub3DPlatformSpecific.pri)

#~~ inputs ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

HEADERS += \
    aboutdialog.h \
    application.h \
    annotations.h \
    archiveparts.h \
    backgrounddialog.h \
    backgrounditem.h \
    borderdialog.h \
    callout.h \
    calloutbackgrounditem.h \
    color.h \
    commands.h \
    commonmenus.h \
    csiitem.h \
    dependencies.h \
    dialogexportpages.h \
    dividerdialog.h \
    editwindow.h \
    excludedparts.h \
    fadestepcolorparts.h \
    globals.h \
    gradients.h \
    highlighter.h \
    hoverpoints.h \
    ldrawfiles.h \
    ldsearchdirs.h \
    lpub.h \
    lpub_preferences.h \
    meta.h \
    metagui.h \
    metaitem.h \
    metatypes.h \
    name.h \
    numberitem.h \
    pagebackgrounditem.h \
    pageattributetextitem.h \
    pageattributepixmapitem.h \
    pairdialog.h \
    pageorientationdialog.h \
    pagesizedialog.h \
    parmshighlighter.h \
    parmswindow.h \
    paths.h \
    placement.h \
    placementdialog.h \
    pli.h \
    pliannotationdialog.h \
    pliconstraindialog.h \
    plisortdialog.h \
    plisubstituteparts.h \
    pointer.h \
    pointeritem.h \
    preferencesdialog.h \
    range.h \
    range_element.h \
    ranges.h \
    ranges_element.h \
    ranges_item.h \
    render.h \
    reserve.h \
    resize.h \
    resolution.h \
    rotateiconitem.h \
    rx.h \
    scaledialog.h \
    step.h \
    textitem.h \
    threadworkers.h \
    updatecheck.h \
    where.h \
    sizeandorientationdialog.h \
    version.h

SOURCES += \
    aboutdialog.cpp \
    application.cpp \
    annotations.cpp \
    archiveparts.cpp \
    assemglobals.cpp \
    backgrounddialog.cpp \
    backgrounditem.cpp \
    borderdialog.cpp \
    callout.cpp \
    calloutbackgrounditem.cpp \
    calloutglobals.cpp \
    color.cpp \
    commands.cpp \
    commonmenus.cpp \
    csiitem.cpp \
    dependencies.cpp \
    dialogexportpages.cpp \
    dividerdialog.cpp \
    editwindow.cpp \
    excludedparts.cpp \
    fadestepcolorparts.cpp \
    fadestepglobals.cpp \
    formatpage.cpp \
    gradients.cpp \
    highlighter.cpp \
    hoverpoints.cpp \
    ldrawfiles.cpp \
    ldsearchdirs.cpp \
    lpub.cpp \
    lpub_preferences.cpp \
    meta.cpp \
    metagui.cpp \
    metaitem.cpp \
    multistepglobals.cpp \
    numberitem.cpp \
    openclose.cpp \
    pagebackgrounditem.cpp \
    pageattributetextitem.cpp \
    pageattributepixmapitem.cpp \
    pageglobals.cpp \
    pageorientationdialog.cpp \
    pagesizedialog.cpp \
    pairdialog.cpp \
    parmshighlighter.cpp \
    parmswindow.cpp \
    paths.cpp \
    placement.cpp \
    placementdialog.cpp \
    pli.cpp \
    pliannotationdialog.cpp \
    pliconstraindialog.cpp \
    pliglobals.cpp \
    plisortdialog.cpp \
    plisubstituteparts.cpp \
    pointeritem.cpp \
    preferencesdialog.cpp \
    printfile.cpp \
    projectglobals.cpp \
    range.cpp \
    range_element.cpp \
    ranges.cpp \
    ranges_element.cpp \
    ranges_item.cpp \
    render.cpp \
    resize.cpp \
    resolution.cpp \
    rotateiconitem.cpp \
    rotate.cpp \
    rx.cpp \
    scaledialog.cpp \
    sizeandorientationdialog.cpp \
    step.cpp \
    textitem.cpp \
    threadworkers.cpp \
    traverse.cpp \
    updatecheck.cpp \
    undoredo.cpp

FORMS += \
    preferences.ui \
    aboutdialog.ui \
    dialogexportpages.ui

OTHER_FILES += \
    Info.plist \
    lpub3d.desktop \
    lpub3d.xml \
    lpub3d.sh \
    $$MAN_PAGE \
    ../builds/macx/CreateDmg.sh \
    ../builds/macx/CreateDmgAlt.sh \
    ../builds/linux/CreateRpm.sh \
    ../builds/linux/CreateDeb.sh \
    ../builds/linux/CreatePkg.sh \
    ../builds/linux/obs/_service \
    ../builds/linux/obs/lpub3d.spec \
    ../builds/linux/obs/PKGBUILD \
    ../builds/linux/obs/debian/rules \
    ../builds/linux/obs/debian/control \
    ../builds/linux/obs/debian/copyright \
    ../builds/linux/obs/debian/lpub3d.dsc \
    ../builds/linux/obs/_service \
    ../builds/windows/CreateExe.bat \
    ../builds/utilities/Copyright-Source-Headers.txt \
    ../builds/utilities/update-config-files.sh \
    ../builds/utilities/update-config-files.bat \
    ../builds/utilities/dmg-utils/dmg-license.py \
    ../builds/utilities/dmg-utils/template.appplescript \
    ../builds/utilities/nsis-scripts/LPub3DNoPack.nsi \
    ../builds/utilities/nsis-scripts/nsisFunctions.nsh \
    ../builds/utilities/create-dmg \
    ../builds/utilities/README.md \
    ../README.md \
    ../.gitignore \
    ../.travis.yml \
    ../appveyor.yml

RESOURCES += \
    lpub3d.qrc

DISTFILES += \
    ldraw_document.icns

# Suppress warnings
QMAKE_CFLAGS_WARN_ON += -Wall -W \
    -Wno-deprecated-declarations \
    -Wno-unused-parameter \
    -Wno-sign-compare
macx {
QMAKE_CFLAGS_WARN_ON += \
    -Wno-overloaded-virtual \
    -Wno-sometimes-uninitialized \
    -Wno-self-assign \
    -Wno-unused-result
} else {
QMAKE_CFLAGS_WARN_ON += \
    -Wno-strict-aliasing
}
QMAKE_CXXFLAGS_WARN_ON = $${QMAKE_CFLAGS_WARN_ON}

#message($$CONFIG)


