run: up compile tmux

build:
	docker build --no-cache . -t cahirwpz/mimiker-circleci:latest

push:
	docker push cahirwpz/mimiker-circleci:latest

up:
	docker-compose up -d

compile:
	docker-compose exec mimiker make

tmux:
	docker-compose exec mimiker tmux

.PHONY: build push up compile tmux run
