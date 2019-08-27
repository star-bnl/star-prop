FROM star-base

# Install dependencies
WORKDIR /tmp

# Install STAR software
COPY patch patch
RUN git clone https://gitlab.com/star-bnl/star-sw.git \
 && git clone --depth 1 https://gitlab.com/star-bnl/star-cvs.git \
 && cd star-cvs && git apply /tmp/patch/*.patch

WORKDIR /tmp/star-build
ENV ROOTSYS /usr/local
RUN cmake ../star-sw -DSTAR_SRC=../star-cvs -DCMAKE_INSTALL_PREFIX=../star-install -DCERNLIB_ROOT=/cern/2006 \
 && make -j $(nproc) \
 && make install
