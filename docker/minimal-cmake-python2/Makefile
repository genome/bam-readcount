NAME   := seqfu/minimal-cmake-python2
TAG    := $$(git log -1 --pretty=%h)
IMG    := ${NAME}:${TAG}
LATEST := ${NAME}:latest
HOST   := $$(basename ${NAME})
PWD    := $$(pwd)

build:
	@docker build -t ${IMG} .
	@docker tag ${IMG} ${LATEST}
 
push:
	@docker push ${NAME}

interact:
	docker run --rm -v "$$(pwd)/../..:/bam-readcount" -w "/bam-readcount" --hostname ${HOST} --detach-keys="ctrl-@" -t -i ${LATEST} /bin/bash

test:
	docker run --rm -v "$$(pwd)/../..:/bam-readcount" -w "/bam-readcount/docker/minimal-cmake-python2" --hostname ${HOST} --detach-keys="ctrl-@" -t -i ${LATEST} ./make_test.sh

login:
	@docker log -u ${DOCKER_USER} -p ${DOCKER_PASS}
