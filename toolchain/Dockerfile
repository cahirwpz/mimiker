FROM debian:buster

WORKDIR /root

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
            automake bison ca-certificates curl flex gcc g++ gettext lhasa \
            libtool make patch  pkg-config python3 python3-setuptools quilt \
            texinfo xz-utils zip libpython3-dev debhelper

COPY . .
