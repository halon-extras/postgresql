FROM ubuntu:24.04
LABEL org.opencontainers.image.authors="support@halon.io"

ARG HALON_REPO_USER
ENV HALON_REPO_USER=$HALON_REPO_USER
ARG HALON_REPO_PASS
ENV HALON_REPO_PASS=$HALON_REPO_PASS

RUN apt-get update

RUN apt-get install -y wget gnupg
RUN apt-get install -y apt-transport-https

RUN wget -qO - https://raw.githubusercontent.com/halon/changelog/master/pgp-keys/7F0A73B5.asc | apt-key add -
RUN echo "deb https://repo.halon.io/ noble stable" >> /etc/apt/sources.list.d/halon.list
RUN echo "machine repo.halon.io login ${HALON_REPO_USER} password ${HALON_REPO_PASS}" >> /etc/apt/auth.conf
RUN apt-get update && apt-get install -y halon=6.3.0

RUN /usr/bin/install -d /var/run/halon
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH

RUN apt-get install -y build-essential cmake git curl pkg-config libpq-dev

RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/jtv/libpqxx/archive/refs/tags/7.9.2.tar.gz \
    && tar -zxvf 7.9.2.tar.gz && cd libpqxx-7.9.2 \
    && mkdir -p /libpqxx && cp COPYING /libpqxx/LICENSE \
    && mkdir build && cd build \
    && cmake -DSKIP_BUILD_TEST=on -DBUILD_SHARED_LIBS=off -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. \
    && make && make install
RUN rm -rf /tmp/halon

RUN echo -n "UBU2404" > /OSRELEASE.txt

COPY ./entrypoint.sh /entrypoint.sh
RUN chmod 755 /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]

RUN apt-get install -y supervisor
COPY ./supervisord.conf /etc/supervisor/conf.d/supervisord.conf
CMD ["/usr/bin/supervisord"]