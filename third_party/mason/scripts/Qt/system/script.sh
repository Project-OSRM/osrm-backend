#!/usr/bin/env bash

MASON_NAME=Qt
MASON_VERSION=system
MASON_SYSTEM_PACKAGE=true

. ${MASON_DIR}/mason.sh

QT_LIBS=(${2:-QtCore})

if hash qmake 2>/dev/null; then
    QMAKE_CMD=qmake
#Some systems such as Fedora23 uses qmake-qt5
elif hash qmake-qt5 2>/dev/null; then
    QMAKE_CMD=qmake-qt5
elif hash qmake-qt4 2>/dev/null; then
    QMAKE_CMD=qmake-qt4
else
    mason_error "Can't find qmake executable"
    exit 1
fi

# Qt5 libs are called Qt5*, so we have to use the correct name to pkg-config
QT_VERSION_MAJOR=$($QMAKE_CMD -query QT_VERSION | cut -d. -f1)
if [ ${QT_VERSION_MAJOR} -gt 4 ] ; then
    QT_LIBS=(${QT_LIBS[@]/#Qt/Qt${QT_VERSION_MAJOR}})
fi

for LIB in ${QT_LIBS[@]} ; do
    if ! `pkg-config ${LIB} --exists` ; then
        mason_error "Can't find ${LIB}"
        exit 1
    fi
done

function mason_system_version {
    if [ ${QT_VERSION_MAJOR} -gt 4 ] ; then
        pkg-config Qt${QT_VERSION_MAJOR}Core --modversion
    else
        pkg-config QtCore --modversion
    fi
}

function mason_build {
    :
}

# pkg-config on OS X returns "-framework\ QtCore", which results in invalid arguments.
function cleanup_args {
    python -c "import sys, re; print re.sub(r'(-framework)\\\\', r'\\1', ' '.join(sys.argv[1:]))" "$@"
}

function mason_cflags {
    echo ${MASON_CFLAGS} $(cleanup_args `pkg-config ${QT_LIBS[@]} --cflags`)
}

function mason_ldflags {
    echo ${MASON_LDFLAGS} $(cleanup_args `pkg-config ${QT_LIBS[@]} --libs`)
}

mason_run "$@"
