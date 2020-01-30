To run a simple test inside a container do

    docker pull starbnl/star-sw:SL19e-m32-build
    docker run starbnl/star-sw:SL19e-m32-build /tmp/star-sw/StRoot/StgMaker/test.sh

In order to save the results locally one should mount a directory on the host as:

    mkdir /path/to/output/dir
    docker run -v /path/to/output/dir:/tmp/star-test-stg starbnl/star-sw:SL19e-m32-build /tmp/star-sw/StRoot/StgMaker/test.sh
