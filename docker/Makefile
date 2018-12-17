run: compile tmux

build:
	docker build --no-cache . -t cahirwpz/mimiker-circleci:latest

push:
	docker push cahirwpz/mimiker-circleci:latest

up:
	docker-compose up -d

down:
	docker-compose down

compile: up
	docker-compose exec mimiker make

tmux: up
	docker-compose exec mimiker tmux

.PHONY: build push up down compile tmux run
