ARG STAR_BASE_IMAGE=starbnl/star-base:centos7

FROM ${STAR_BASE_IMAGE} AS build-stage

# Set arguments default values
ARG STAR_SW_REF=master
ARG STAR_CVS_REF=master
ARG STAR_BUILD_TYPE=Release

WORKDIR /tmp

# Get STAR software
ADD https://api.github.com/repos/star-bnl/star-sw/commits/${STAR_SW_REF} star-sw-ref.json
RUN curl -s -L https://github.com/star-bnl/star-sw/archive/${STAR_SW_REF}.tar.gz  | tar -xz -C /tmp \
 && ln -s /tmp/star-sw-${STAR_SW_REF} /tmp/star-sw

ADD https://api.github.com/repos/star-bnl/star-cvs/commits/${STAR_CVS_REF} star-cvs-ref.json
RUN curl -s -L https://github.com/star-bnl/star-cvs/archive/${STAR_CVS_REF}.tar.gz | tar -xz -C /tmp \
 && ln -s /tmp/star-cvs-${STAR_CVS_REF} /tmp/star-cvs

# Build STAR software
WORKDIR /tmp/star-build

RUN cmake /tmp/star-sw -DSTAR_SRC=/tmp/star-cvs \
    -DSTAR_PATCH=gcc540 -DCMAKE_INSTALL_PREFIX=/tmp/star-install \
    -DCERNLIB_ROOT=/cern/2006 -DCMAKE_BUILD_TYPE=${STAR_BUILD_TYPE} \
 && make -j $(nproc) \
 && make install

# Install additional packages for interactive work
RUN yum update -q -y \
 && yum install -y emacs vim-minimal \
 && yum clean all

# Get rid of source and build artifacts and only keep installed STAR software
FROM ${STAR_BASE_IMAGE}
COPY --from=build-stage /tmp/star-install /tmp/star-install
WORKDIR /tmp/star-install

ENTRYPOINT ["/tmp/star-install/.entrypoint.sh"]
CMD ["/bin/bash"]
