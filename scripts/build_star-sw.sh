#!/usr/bin/env bash

# Set default values for primary parameters
: ${STAR_CVS_REF:="master"}
: ${STAR_BUILD_TYPE:="Release"}
: ${STAR_BUILD_32BIT:=""}
: ${STAR_BASE_OS:="centos7"}
: ${STAR_SW_DIR:="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"}
: ${STAR_SW_REF:="master"}
: ${DOCKER_ID_NAMESPACE:="starbnl/"}
: ${DRY_RUN:=""}

read -r -d '' HELP_MSG << EOF
Usage: $(basename $0) [<star-cvs_ref>] [OPTIONS]
Options:
 <star-cvs_ref> [=master]               Any commit reference in star-cvs repo (branch, tag, or hash)
 -t Debug|Release|RelWithDebInfo [=Release]
                                        A valid value for CMAKE_BUILD_TYPE
 -m                                     Force 32-bit builds on a 64-bit platform
 -b <star_base_os> [=centos7]           For available bases see docker files in star-sw/docker
 -p <path_to_star-sw> [=$STAR_SW_DIR]
                                        Defaults to this script location but can be set to anything
 -s <star-sw_ref> [=master]             Any commit reference in star-sw repo (branch, tag, or hash)
 -d                                     Print commands but do not execute them
 -h                                     Print this help message
EOF

function print_usage() {
	echo "$HELP_MSG"
	exit 1;
}


# Process options and their values
options=$(getopt -o "t:mb:p:s:dh" -n "$0" -- "$@")
[ $? -eq 0 ] || print_usage
eval set -- "$options"

while true; do
	case "$1" in
		-t)
			STAR_BUILD_TYPE="$2" ;
			[[ ! $STAR_BUILD_TYPE =~ Debug|Release|RelWithDebInfo ]] && {
				echo -e "Error: Incorrect value provided for -t option\n"
				print_usage
			}
			shift 2 ;;
		-m)
			STAR_BUILD_32BIT="Yes" ;
			shift 1 ;;
		-b)
			STAR_BASE_OS="$2" ;
			shift 2 ;;
		-p)
			STAR_SW_DIR="$2" ;
			shift 2 ;;
		-s)
			STAR_SW_REF="$2" ;
			shift 2 ;;
		-d)
			DRY_RUN="Yes" ;
			shift 1 ;;
		-h)
			print_usage ;
			shift ;;
		--)
			shift ;
			break ;;
	esac
done

# Process the only positional argument
if [ -n "$1" ]; then
	STAR_CVS_REF=$1
fi

# Just print out the variable's values for the record
echo "The following variables are set:"
echo -e "\t STAR_CVS_REF:          \"$STAR_CVS_REF\""
echo -e "\t STAR_BUILD_TYPE:       \"$STAR_BUILD_TYPE\""
echo -e "\t STAR_BUILD_32BIT:      \"$STAR_BUILD_32BIT\""
echo -e "\t STAR_BASE_OS:          \"$STAR_BASE_OS\""
echo -e "\t STAR_SW_DIR:           \"$STAR_SW_DIR\""
echo -e "\t STAR_SW_REF:           \"$STAR_SW_REF\""
echo -e "\t DOCKER_ID_NAMESPACE:   \"$DOCKER_ID_NAMESPACE\""

# Format base image name as star-base:$STAR_BASE_OS[-m32]
STAR_BASE_IMAGE_TAG="$STAR_BASE_OS"
STAR_BASE_IMAGE_TAG+=$([ -z "$STAR_BUILD_32BIT" ] && echo "" || echo "-m32")
STAR_BASE_IMAGE_TAG+=$([ "$STAR_SW_REF"  = "master" ] && echo "" || echo "-$STAR_SW_REF")

STAR_BASE_IMAGE_NAME=${DOCKER_ID_NAMESPACE}star-base:${STAR_BASE_IMAGE_TAG}

# Format image tag as star-sw:$STAR_CVS_REF[-$STAR_BUILD_TYPE][-$STAR_BASE_OS][-m32][-$STAR_SW_REF]
STAR_IMAGE_TAG=$([ "$STAR_CVS_REF" = "master" ] && echo "latest" || echo "$STAR_CVS_REF")
STAR_IMAGE_TAG+=$([ "$STAR_BUILD_TYPE" = "Release" ] && echo "" || echo "-$STAR_BUILD_TYPE")
STAR_IMAGE_TAG+=$([ "$STAR_BASE_OS" = "centos7" ] && echo "" || echo "-$STAR_BASE_OS")
STAR_IMAGE_TAG+=$([ -z "$STAR_BUILD_32BIT" ] && echo "" || echo "-m32")
STAR_IMAGE_TAG+=$([ "$STAR_SW_REF"  = "master" ] && echo "" || echo "-$STAR_SW_REF")

STAR_IMAGE_NAME=${DOCKER_ID_NAMESPACE}star-sw:${STAR_IMAGE_TAG}

echo
echo -e "\t STAR_BASE_IMAGE_NAME:   \"$STAR_BASE_IMAGE_NAME\""
echo -e "\t STAR_IMAGE_NAME:        \"$STAR_IMAGE_NAME\""

# Set default attributes for the files copied into the containers
# Having same attributes ensures reproducibility of layer hashes and therefore,
# reuse of cache even when containers are built on different hosts
touch -d "@0" docker/packages.* patches/*.patch
chmod 644 docker/packages.* patches/*.patch


cmd="docker build -t ${STAR_BASE_IMAGE_NAME} \
	-f ${STAR_SW_DIR}/docker/Dockerfile.star-base-${STAR_BASE_OS} \
	--build-arg STAR_BUILD_32BIT=${STAR_BUILD_32BIT} \
	--cache-from ${STAR_BASE_IMAGE_NAME} \
	${STAR_SW_DIR}
"
echo
echo $ $cmd
[ -z "$DRY_RUN" ] && eval "$cmd"

cmd="docker build -t ${STAR_IMAGE_NAME}-build \
	--build-arg STAR_CVS_REF=${STAR_CVS_REF} \
	--build-arg STAR_BUILD_TYPE=${STAR_BUILD_TYPE} \
	--build-arg STAR_BASE_IMAGE=${STAR_BASE_IMAGE_NAME} \
	--build-arg STAR_SW_REF=${STAR_SW_REF} \
	--target build-stage \
	${STAR_SW_DIR}
"
echo
echo $ $cmd
[ -z "$DRY_RUN" ] && eval "$cmd"

cmd="docker build -t ${STAR_IMAGE_NAME} \
	--build-arg STAR_CVS_REF=${STAR_CVS_REF} \
	--build-arg STAR_BUILD_TYPE=${STAR_BUILD_TYPE} \
	--build-arg STAR_BASE_IMAGE=${STAR_BASE_IMAGE_NAME} \
	--build-arg STAR_SW_REF=${STAR_SW_REF} \
	${STAR_SW_DIR}
"
echo
echo $ $cmd
[ -z "$DRY_RUN" ] && eval "$cmd"

cmd="docker build -t ${STAR_IMAGE_NAME}-cons \
	-f ${STAR_SW_DIR}/docker/Dockerfile.star-sw-cons \
	--build-arg STAR_CVS_REF=${STAR_CVS_REF} \
	--build-arg STAR_BUILD_TYPE=${STAR_BUILD_TYPE} \
	--build-arg STAR_BUILD_32BIT=${STAR_BUILD_32BIT} \
	--build-arg STAR_BASE_IMAGE=${STAR_BASE_IMAGE_NAME} \
	${STAR_SW_DIR}
"
echo
echo $ $cmd
[ -z "$DRY_RUN" ] && eval "$cmd"
