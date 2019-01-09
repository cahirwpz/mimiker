run: compile tmux

M_UID=$(shell id -u)
M_GID=$(shell id -u)

gen:
	export MIMIKER_UID=$(M_UID) MIMIKER_GID=$(M_GID) && \
	envsubst < Dockerfile.tpl > Dockerfile

build: gen
	docker build --no-cache . -t mimiker-dev:latest

up:
	docker-compose up -d

down:
	docker-compose down

compile: up
	docker-compose exec mimiker make

tmux: up
	docker-compose exec mimiker tmux

clean:
	docker container prune --filter "until=24h"
	docker image prune
	rm Dockerfile

.PHONY: build up down compile tmux run clean gen
