run: compile tmux

M_UID=$(shell id -u)
M_GID=$(shell id -g)


gen:
	export MIMIKER_UID=$(M_UID) MIMIKER_GID=$(M_GID) && \
	envsubst < Dockerfile.tpl > Dockerfile

build: gen
	docker build --no-cache . -t mimiker-dev:latest

up:
	touch /tmp/.docker.xauth
	touch ~/.tmux.conf
	touch ~/.gitconfig
	xauth nlist :0 | sed -e 's/^..../ffff/' | xauth -f /tmp/.docker.xauth nmerge -
	docker-compose up -d

down:
	docker-compose down

compile: gen up
	docker-compose exec mimiker make

tmux: gen up
	docker-compose exec mimiker tmux

clean:
	docker container prune --filter "until=24h"
	docker image prune
	rm -f Dockerfile

.PHONY: build up down compile tmux run clean gen
