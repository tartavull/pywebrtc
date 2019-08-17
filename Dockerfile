# Defining environment
# from https://antmedia.io/building-and-cross-compiling-webrtc-for-raspberry/
FROM ubuntu:18.04 
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update &&  apt install -y git \
   wget \ 
   xz-utils \
   python \ 
   openssh-client \
   sshpass \
   pkg-config \
   rsync \
   cmake \ 
   unzip \
   gawk

RUN mkdir -p /webrtc
WORKDIR /webrtc
RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /opt/depot_tools
ENV echo “export PATH=/opt/depot_tools:\$PATH” | tee /etc/profile.d/depot_tools.sh

RUN git clone https://github.com/raspberrypi/tools.git /opt/rpi_tools
RUN echo “export PATH=/opt/rpi_tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin:\$PATH” | tee /etc/profile.d/rpi_tools.sh
RUN apt-get install -y qemu-user-static debootstrap
RUN debootstrap --arch armhf --foreign --include=g++,libasound2-dev,libpulse-dev,libudev-dev,libexpat1-dev,libnss3-dev,libgtk2.0-dev stretch rootfs
RUN cp /usr/bin/qemu-arm-static rootfs/usr/bin/
#RUN chroot rootfs /debootstrap/debootstrap --second-stage
#RUN find rootfs/usr/lib/arm-linux-gnueabihf -lname ‘/*’ -printf ‘%p %l\n’ | while read link target do ln -snfv “../../..${target}” “${link}” done
#RUN find rootfs/usr/lib/arm-linux-gnueabihf/pkgconfig -printf “%f\n” | while read target do ln -snfv “../../lib/arm-linux-gnueabihf/pkgconfig/${target}” rootfs/usr/share/pkgconfig/${target} done
#RUN fetch –no-history –nohooks webrtc
#WORKDIR /webrtc/src
#RUN ./build/linux/sysroot_scripts/install-sysroot.py –arch=arm
#RUN gn gen out/Default –args='target_os="linux" target_cpu="arm"'
#RUN mv webrtc/modules/rtp_rtcp/test/testFec/test_packet_masks_metrics.cc webrtc/modules/rtp_rtcp/test/testFec/test_packet_masks_metrics.cc.bak
#RUN touch webrtc/modules/rtp_rtcp/test/testFec/test_packet_masks_metrics.cc
#RUN ninja -C out/Default
