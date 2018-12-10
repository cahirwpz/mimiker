build:
	docker build . -t cahirwpz/mimiker-circleci:latest

push:
	docker push cahirwpz/mimiker-circleci:latest

up:
	docker-compose up -d

compile:
	docker-compose exec mimiker make

tmux:
	docker-compose exec mimiker tmux

run: up compile tmux

