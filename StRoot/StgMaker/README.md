To run a simple test inside a container do

    docker pull starbnl/star-sw:SL19e-m32-build
    clone git@github.com:star-bnl/star-sw.git
    cd star-sw && git checkout stg-basic-test
    docker run -v /path/to/star-sw:/tmp/star-sw starbnl/star-sw:SL19e-m32-build /tmp/star-sw/StRoot/StgMaker/test.sh

Once this PR is accepted and the container is rebuilt the test can be initiated with a shorter command:

    docker run starbnl/star-sw:SL19e-m32-build /tmp/star-sw/StRoot/StgMaker/test.sh

In order to save the results locally one should mount a directory on the host as:

    mkdir /path/to/output/dir
    docker run -v /path/to/output/dir:/tmp/star-test-stg starbnl/star-sw:SL19e-m32-build /tmp/star-sw/StRoot/StgMaker/test.sh
