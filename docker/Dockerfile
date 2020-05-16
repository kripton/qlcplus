FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN \
        apt-get update && \
        apt-get install -y --no-install-recommends \
        apt-utils \   
        bison \
        build-essential \
        ca-certificates \
        chrpath \
        git \  
        gperf \
        libasound2-dev \
        libclang-dev \
        libgl1-mesa-dev \
        libgudev-1.0-dev \
        libudev-dev \
        libx11-xcb-dev \
        libxcb-xfixes0-dev \
        libxcb1-dev \
        libxfixes-dev \
        perl \
        python \
        sudo \
        wget

RUN \
        wget https://download.qt.io/official_releases/qt/5.14/5.14.2/single/qt-everywhere-src-5.14.2.tar.xz && \
        tar -xvf qt-* && \
        cd qt-* && \
        ./configure -confirm-license -opensource -nomake examples -nomake tests -no-compile-examples && \
        make -j8 && \
        make install

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

USER docker

WORKDIR /home/docker

RUN \
        source /opt/qt514/bin/qt514-env.sh && \
        git clone https://github.com/mcallegari/qlcplus.git && \
        cd qlcplus && \
        ./create-appimage.sh
