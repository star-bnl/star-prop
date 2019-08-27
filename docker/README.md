Docker images containing STAR software (i.e. libraries and root4star) and all of
its run-time dependencies can be built using the Dockerfiles provided here.
These are not used in any test or production environment so, consider them as
examples. To build the images `cd` into the `star-sw/docker` sub-directory
first.

**star-sw**

An image named `star-sw-test`(4.5GB) is appropriate for tests and can be created
as:

    docker build --rm -t star-sw-test -f star-sw/Dockerfile.test star-sw/

A thinner image `star-sw-release` (2.3GB) would be more appropriate for a STAR
release:

    docker build --rm -t star-sw-release -f star-sw/Dockerfile.release star-sw/

**star-base**

We also provide a Dockerfile to build an image based on ubuntu xenial (16.04)
with all dependencies needed by the STAR software. The major components include

  - gcc 5.4.0
  - ROOT 5.34.38
  - CERNLIB 2006

This `star-base` image (2GB) can be produced by running the following command:

    docker build --rm -t star-base star-base/
