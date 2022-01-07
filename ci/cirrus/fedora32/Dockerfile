FROM fedora:32

RUN dnf -y update && dnf clean all

RUN dnf makecache \
  && dnf -y install \
    gcc \
    cmake \
    diffutils \
    make \
    ninja-build \
  && dnf clean all
