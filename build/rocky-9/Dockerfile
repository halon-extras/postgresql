FROM rockylinux:9
LABEL org.opencontainers.image.authors="support@halon.io"

ARG TARGETARCH
ARG TARGETOS

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN yum update -y && yum install -y epel-release
RUN crb enable

RUN echo "[halon]" >> /etc/yum.repos.d/Halon.repo
RUN echo "name=Halon repository" >> /etc/yum.repos.d/Halon.repo
RUN echo "baseurl=https://${HALON_REPO_USER}:${HALON_REPO_PASS}@repo.halon.io/dists/rhel9/${TARGETARCH}/stable" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgcheck=1" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgkey=https://raw.githubusercontent.com/halon/changelog/master/pgp-keys/7F0A73B5.asc" >> /etc/yum.repos.d/Halon.repo
RUN echo "enabled=1" >> /etc/yum.repos.d/Halon.repo
RUN yum install -y halon-5.12.1

RUN yum install -y cmake3 make gcc-c++ rpm-build git libpq-devel

RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/jtv/libpqxx/archive/refs/tags/7.9.2.tar.gz \
    && echo "e37d5774c39f6c802e32d7f418e88b8e530404fb54758516e884fc0ebdee6da4 7.9.2.tar.gz" | sha256sum -c \
    && tar -zxvf 7.9.2.tar.gz && cd libpqxx-7.9.2 \
    && mkdir -p /libpqxx && cp COPYING /libpqxx/LICENSE \
    && mkdir build && cd build \
    && cmake -DSKIP_BUILD_TEST=on -DBUILD_SHARED_LIBS=off -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. \
    && make && make install
RUN rm -rf /tmp/halon

RUN echo -n "ROCKY9" > /OSRELEASE.txt

COPY build.sh /build.sh
CMD ["/build.sh"]
