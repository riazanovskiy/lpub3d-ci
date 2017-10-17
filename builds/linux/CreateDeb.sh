#!/bin/bash
# Trevor SANDY
# Last Update October 16 2017
# To run:
# $ chmod 755 CreateDeb.sh
# $ ./CreateDeb.sh

ME="$(basename "$(test -L "$0" && readlink "$0" || echo "$0")")"
CWD=`pwd`
BUILD_DATE=`date "+%Y%m%d"`
CHANGE_DATE_LONG=`date +%a,\ %d\ %b\ %Y\ %H:%M:%S\ %z`
LPUB3D=lpub3d-ci

echo "Start $ME at $CWD..."

# when running locally, use this block...
if [ "${TRAVIS}" != "true"  ]; then
    # Travis starting point: /home/travis/build/trevorsandy/lpub3d-ci
    #
    # logging stuff
    # increment log file name
    f="${CWD}/$ME"
    ext=".log"
    if [[ -e "$f$ext" ]] ; then
        i=1
        f="${f%.*}";
        while [[ -e "${f}_${i}${ext}" ]]; do
          let i++
        done
        f="${f}_${i}${ext}"
        else
        f="${f}${ext}"
    fi
    # output log file
    LOG="$f"
    exec > >(tee -a ${LOG} )
    exec 2> >(tee -a ${LOG} >&2)
# For Travis CI, use this block (script called from [pwd] lpub3d/)
else
    # move outside lpub3d/
    cd ../
fi

echo "1. create DEB working directories..."
if [ ! -d debbuild ]
then
    mkdir -p debbuild/SOURCES
else
    rm -rf debbuild
    mkdir -p debbuild/SOURCES
fi

cd debbuild/SOURCES

if [ "${TRAVIS}" != "true"  ]; then
    echo "2. download source..."
    git clone https://github.com/trevorsandy/${LPUB3D}.git
else
    echo "2. copy source..."
    cp -rf "../../${LPUB3D}" .
fi

echo "3. capture version info..."
#         1 2  3  4   5       6    7  8  9       10
# format "2 0 20 17 663 410fdd7 2017 02 12 19:50:21"
FILE="${LPUB3D}/builds/utilities/version.info"
if [ -f ${FILE} -a -r ${FILE} ]
then
    VERSION_INFO=`cat ${FILE}`
else
    echo "Error: Cannot read ${FILE} from `pwd`"
    echo "$ME terminated!"
    exit 1
fi
read VER_MAJOR VER_MINOR VER_PATCH VER_REVISION VER_BUILD VER_SHA_HASH THE_REST <<< ${VERSION_INFO//'"'}
VERSION=${VER_MAJOR}"."${VER_MINOR}"."${VER_PATCH}
APP_VERSION=${VERSION}"."${VER_BUILD}
APP_VERSION_LONG=${VERSION}"."${VER_REVISION}"."${VER_BUILD}_${BUILD_DATE}
#echo "WORK_DIR..........${WORK_DIR}"
echo "VER_MAJOR.........${VER_MAJOR}"
echo "VER_MINOR.........${VER_MINOR}"
echo "VER_PATCH.........${VER_PATCH}"
echo "VER_REVISION......${VER_REVISION}"
echo "VER_BUILD.........${VER_BUILD}"
echo "VER_SHA_HASH......${VER_SHA_HASH}"
echo "VERSION...........${VERSION}"
echo "APP_VERSION.......${APP_VERSION}"
echo "APP_VERSION_LONG..${APP_VERSION_LONG}"
echo "BUILD_DATE........${BUILD_DATE}"
echo "CHANGE_DATE_LONG..${CHANGE_DATE_LONG}"

SOURCE_DIR=${LPUB3D}-${APP_VERSION}
echo "SOURCE_DIR........${SOURCE_DIR}"
mv ${LPUB3D} ${SOURCE_DIR}

echo "4. create cleaned tarball ${LPUB3D}_${APP_VERSION}.orig.tar.gz using ${SOURCE_DIR}/"
tar -czvf ../${LPUB3D}_${APP_VERSION}.orig.tar.gz ${SOURCE_DIR} \
        --exclude="${SOURCE_DIR}/builds/linux/standard" \
        --exclude="${SOURCE_DIR}/builds/macx" \
        --exclude="${SOURCE_DIR}/.travis.yml" \
        --exclude="${SOURCE_DIR}/.git" \
        --exclude="${SOURCE_DIR}/.gitattributes" \
        --exclude="${SOURCE_DIR}/LPub3D.pro.user" \
        --exclude="${SOURCE_DIR}/README.md" \
        --exclude="${SOURCE_DIR}/_config.yml" \
        --exclude="${SOURCE_DIR}/.gitignore" \
        --exclude="${SOURCE_DIR}/appveyor.yml"

echo "5. get LDraw archive libraries..."
# we pull in the library archives here because the lpub3d.spec file copies them
# to the extras location. This config thus supports both Suse OBS and Travis CI build procs.
if [ ! -f lpub3dldrawunf.zip ]
then
     wget -q http://www.ldraw.org/library/unofficial/ldrawunf.zip -O lpub3dldrawunf.zip
fi
if [ ! -f complete.zip ]
then
     wget -q http://www.ldraw.org/library/updates/complete.zip
fi

echo "6. re-create soruce directory ${SOURCE_DIR}..."
cd ../
if [  -d ${LPUB3D}_${APP_VERSION} ]
then
   rm -rf ${LPUB3D}_${APP_VERSION}
fi
tar zxf ${LPUB3D}_${APP_VERSION}.orig.tar.gz

echo "7. copy debian configuration directory..."
cp -rf ${SOURCE_DIR}/builds/linux/obs/debian ${SOURCE_DIR}
cd ${SOURCE_DIR}/debian

echo "8. build application package..."
cd ../
chmod 755 debian/rules
/usr/bin/dpkg-buildpackage -us -uc

cd ../
DISTRO_FILE=`ls *.deb`
if [ -f ${DISTRO_FILE} ] && [ ! -z ${DISTRO_FILE} ]
then
    echo "9. create update and download packages..."
    IFS=_ read DEB_NAME DEB_VERSION DEB_ARCH_EXTENSION <<< ${DISTRO_FILE}

    cp -rf ${DISTRO_FILE} "${LPUB3D}_${APP_VERSION_LONG}_${DEB_ARCH_EXTENSION}"
    echo "    Download package: ${LPUB3D}_${APP_VERSION_LONG}_${DEB_ARCH_EXTENSION}"

    mv ${DISTRO_FILE} "LPub3D-UpdateMaster_${VERSION}_${DEB_ARCH_EXTENSION}"
    echo "      Update package: LPub3D-UpdateMaster_${VERSION}_${DEB_ARCH_EXTENSION}"
else
    echo "9. package ${DISTRO_FILE} not found"
fi

if [ "${TRAVIS}" == "true"  ]; then
  # export vars used by travis.yml so paths must be relative to project download dir
  export LP3D_Download_Package="../${LPUB3D}_${APP_VERSION_LONG}_${DEB_ARCH_EXTENSION}"
  export LP3D_Update_Package="../LPub3D-UpdateMaster_${VERSION}_${DEB_ARCH_EXTENSION}"
  echo "../ content:" `ls ../`
  env | grep -P 'LP3D*'
  env | grep -P 'TRAVIS*'
fi

# create deb - end #
echo "$ME Finished!"
# mv $LOG "${CWD}/debbuild/$ME.log"
