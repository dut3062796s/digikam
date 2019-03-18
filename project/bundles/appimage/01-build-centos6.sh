#!/bin/bash

# Script to build a CentOS 6 installation to compile an AppImage bundle of digiKam.
# This script must be run as sudo
#
# Copyright (c) 2015-2019, Gilles Caulier, <caulier dot gilles at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

# Halt and catch errors
set -eE
trap 'PREVIOUS_COMMAND=$THIS_COMMAND; THIS_COMMAND=$BASH_COMMAND' DEBUG
trap 'echo "FAILED COMMAND: $PREVIOUS_COMMAND"' ERR

#################################################################################################
# Manage script traces to log file

mkdir -p ./logs
exec > >(tee ./logs/build-centos6.full.log) 2>&1

#################################################################################################

echo "01-build-centos6.sh : build a CentOS 6 installation to compile an AppImage of digiKam."
echo "--------------------------------------------------------------------------------------"

#################################################################################################
# Pre-processing checks

. ./config.sh
. ./common.sh
ChecksRunAsRoot
StartScript
ChecksCPUCores
CentOS6Adjustments
ORIG_WD="`pwd`"

#################################################################################################

echo -e "---------- Update Linux CentOS 6\n"

yum -y install epel-release

if [[ "$(arch)" = "x86_64" ]] ; then

    yum upgrade ca-certificates --disablerepo=epel

fi

if [[ ! -f /etc/yum.repos.d/epel.repo ]] ; then

    yum install epel-release

    # we need to be up to date in order to install the xcb-keysyms dependency
    yum -y update
fi

#################################################################################################

echo -e "---------- Install New Development Packages\n"

# Packages for base dependencies and Qt5.
yum -y install wget \
               tar \
               bzip2 \
               gettext \
               git \
               subversion \
               libtool \
               which \
               fuse \
               automake \
               mesa-libEGL \
               cmake3 \
               gcc-c++ \
               patch \
               libxcb \
               libxcb-devel \
               xcb-util \
               xcb-util-keysyms-devel \
               xcb-util-devel \
               xcb-util-image-devel \
               xcb-util-renderutil-devel \
               xcb-util-wm-devel \
               xkeyboard-config \
               gperf \
               bison \
               flex \
               zlib-devel \
               expat-devel \
               fuse-devel \
               libtool-ltdl-devel \
               glib2-devel \
               glibc-headers \
               mysql-devel \
               eigen3-devel \
               cppunit-devel \
               libstdc++-devel \
               libxml2-devel \
               libstdc++-devel \
               libXrender-devel \
               lcms2-devel \
               libXi-devel \
               mesa-libGL-devel \
               mesa-libGLU-devel \
               glibc-devel \
               libudev-devel \
               libuuid-devel \
               sqlite-devel \
               libusb-devel \
               libexif-devel \
               libical-devel \
               libxslt-devel \
               xz-devel \
               lz4-devel \
               inotify-tools-devel \
               cups-devel \
               openal-soft-devel \
               openssl-devel \
               libical-devel \
               libcap-devel \
               libicu-devel \
               libXScrnSaver-devel \
               mesa-libEGL-devel \
               patchelf \
               dpkg

#################################################################################################

if [[ ! -f /etc/yum.repos.d/mlampe-python2.7_epel6.repo ]] ; then

    echo -e "---------- Install New Python Interpreter\n"

    cd /etc/yum.repos.d
    wget https://copr.fedorainfracloud.org/coprs/g/python/python2.7_epel6/repo/epel-6/mlampe-python2.7_epel6.repo
    yum -y --nogpgcheck install python2.7

fi

if [[ ! -f /opt/rh/rh-ruby24/enable ]] ; then

    echo -e "---------- Install New Ruby Interpreter\n"

    yum -y install centos-release-scl-rh centos-release-scl
    yum -y --nogpgcheck install rh-ruby24

fi

if [[ ! -f /etc/yum.repos.d/mlampe-devtoolset-7-epel-6.repo ]] ; then

    echo -e "---------- Install New Compiler Tools Set\n"

    cd /etc/yum.repos.d
    wget https://copr.fedorainfracloud.org/coprs/mlampe/devtoolset-7/repo/epel-6/mlampe-devtoolset-7-epel-6.repo
    yum -y --nogpgcheck install devtoolset-7-toolchain

fi

#################################################################################################

# Install new repo to get ffmpeg dependencies

if [[ ! -f /etc/yum.repos.d/nux-dextop.repo ]] ; then

    echo -e "---------- Install Repository for ffmpeg packages\n"

    if [[ "$(arch)" = "x86_64" ]] ; then

        rpm --import http://li.nux.ro/download/nux/RPM-GPG-KEY-nux.ro
        rpm -Uvh http://li.nux.ro/download/nux/dextop/el6/x86_64/nux-dextop-release-0-2.el6.nux.noarch.rpm

    else

        rpm --import http://li.nux.ro/download/nux/RPM-GPG-KEY-nux.ro
        rpm -Uvh http://li.nux.ro/download/nux/dextop/el6/i386/nux-dextop-release-0-2.el6.nux.noarch.rpm

    fi

fi

yum -y install fdk-aac-devel \
               faac-devel \
               lame-devel \
               opencore-amr-devel \
               opus-devel \
               librtmp-devel \
               speex-devel \
               libtheora-devel \
               libvorbis-devel \
               libvpx-devel \
               x264-devel \
               x265-devel \
               xvidcore-devel \
               yasm \
               fribidi-devel

#################################################################################################

echo -e "---------- Clean-up Old Packages\n"

# Remove system based devel package to prevent conflict with new one.
yum -y erase qt-devel \
             boost-devel \
             libgphoto2 \
             sane-backends \
             libjpeg-devel \
             jasper-devel \
             libpng-devel \
             libtiff-devel \
             ffmpeg \
             ffmpeg-devel \
             ant \
             pulseaudio-libs-devel \
             libfreetype-devel \
             libfontconfig-devel

#################################################################################################

echo -e "---------- Prepare CentOS to Compile Extra Dependencies\n"

# Workaround for: On CentOS 6, .pc files in /usr/lib/pkgconfig are not recognized
# However, this is where .pc files get installed when bulding libraries... (FIXME)
# I found this by comparing the output of librevenge's "make install" command
# between Ubuntu and CentOS 6
ln -sf /usr/share/pkgconfig /usr/lib/pkgconfig

# Make sure we build from the /, parts of this script depends on that. We also need to run as root...
cd /

# Create the working directories.

if [ ! -d $BUILDING_DIR ] ; then
    mkdir -p $BUILDING_DIR
fi
if [ ! -d $DOWNLOAD_DIR ] ; then
    mkdir -p $DOWNLOAD_DIR
fi
if [ ! -d $INSTALL_DIR ] ; then
    mkdir -p $INSTALL_DIR
fi


# enable new compiler
. /opt/rh/devtoolset-7/enable

# enable new Ruby interpreter
. /opt/rh/rh-ruby24/enable

#################################################################################################
# Low level libraries dependencies
# NOTE: The order to compile each component here is very important.

cd $BUILDING_DIR

rm -rf $BUILDING_DIR/* || true

cmake3 $ORIG_WD/../3rdparty \
       -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR \
       -DINSTALL_ROOT=$INSTALL_DIR \
       -DEXTERNALS_DOWNLOAD_DIR=$DOWNLOAD_DIR

# --- digiKam dependencies stage1 -------------

cmake3 --build . --config RelWithDebInfo --target ext_jpeg          -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_jasper        -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_png           -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_tiff          -- -j$CPU_CORES
#cmake3 --build . --config RelWithDebInfo --target ext_openssl       -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_freetype      -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_fontconfig    -- -j$CPU_CORES    # depend of freetype
#cmake3 --build . --config RelWithDebInfo --target ext_libicu        -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_qt            -- -j$CPU_CORES    # depend of fontconfig, freetype, libtiff, libjpeg, libpng
cmake3 --build . --config RelWithDebInfo --target ext_qtwebkit      -- -j$CPU_CORES    # depend of qt

# --- digiKam dependencies stage2 -------------

cmake3 --build . --config RelWithDebInfo --target ext_boost         -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_libass        -- -j$CPU_CORES    # depend of fontconfig
cmake3 --build . --config RelWithDebInfo --target ext_ffmpeg        -- -j$CPU_CORES    # depend of libass
cmake3 --build . --config RelWithDebInfo --target ext_qtav          -- -j$CPU_CORES    # depend of qt and ffmpeg
cmake3 --build . --config RelWithDebInfo --target ext_libgphoto2    -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_sane          -- -j$CPU_CORES    # depend of libgphoto2
cmake3 --build . --config RelWithDebInfo --target ext_exiv2         -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_opencv        -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_lensfun       -- -j$CPU_CORES
#cmake3 --build . --config RelWithDebInfo --target ext_liblqr        -- -j$CPU_CORES
cmake3 --build . --config RelWithDebInfo --target ext_linuxdeployqt -- -j$CPU_CORES    # depend of qt

#################################################################################################

TerminateScript
