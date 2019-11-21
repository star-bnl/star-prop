Docker images containing STAR software (i.e. libraries, root4star, and starsim)
and all of its run-time dependencies can be built using the Dockerfiles provided
here. These are not used in any test or production environment so, consider them
**examples** for now. To build the images `cd` into the `star-sw` directory
first.


# Building Containers

**star-base**

We provide Dockerfiles to build an image based on Ubuntu 16.04 (Xenial) and
18.04 (Bionic) with all dependencies needed by the STAR software. The major
components include

  - gcc 5.4.0, gcc 7.4.0
  - ROOT 5.34.38
  - CERNLIB 2006
  - Eigen 3.3.7

These `star-base` images (2GB) can be produced locally by running the following
commands:

    cd /path/to/star-sw
    docker build --rm -t star-base -f docker/Dockerfile.star-base-ubuntu16 docker/

    cd /path/to/star-sw
    docker build --rm -t star-base -f docker/Dockerfile.star-base-ubuntu18 docker/

**star-sw**

An image named `star-sw-test`(4.5GB) is appropriate for tests and can be created
as:

    cd /path/to/star-sw
    docker build --rm -t star-sw-test -f docker/Dockerfile.star-sw-test docker/

A thinner image `star-sw-release` (2.3GB) would be more appropriate for a STAR
release:

    cd /path/to/star-sw
    docker build --rm -t star-sw-release -f docker/Dockerfile.star-sw-release docker/


# Building and Running STAR Software in a Docker Container

One can build locally modified code inside a docker container, e.g. to use
`star-base-ubuntu16` do

    docker run -it --rm \
               -v /path/to/star-cvs:/tmp/star-cvs \
               -v /path/to/star-sw:/tmp/star-sw \
               -v /path/to/star/data:/tmp/data \
               -w /tmp star-base-ubuntu16 bash
    # At this point you should be in /tmp inside the container
    mkdir star-build && cd star-build
    cmake /tmp/star-sw -DSTAR_SRC=/tmp/star-cvs -DCMAKE_INSTALL_PREFIX=/tmp/star-install -DCERNLIB_ROOT=/cern/2006 -DSTAR_PATCH=gcc540
    make -j4
    make install
    source /tmp/star-install/bin/thisstar.sh
    source /usr/local/bin/thisroot.sh
    cd /tmp/star-install
    root4star -b '/tmp/star-install/StRoot/macros/bfc.C(3, "bfc,chain,options", "/tmp/data/st_physics_file.daq")'
