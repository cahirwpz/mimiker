VERSION = 1.6

run: compile tmux

build:
	docker build --no-cache . -t cahirwpz/mimiker-circleci:latest

perms:
	# Set the directory permissions to hardcoded GID
	sudo chown :1024 ../
	sudo chmod 775 ../
	# Ensure new files will inherit the rigth group ID
	sudo chmod g+s ../

up: perms
	docker-compose up -d

down:
	docker-compose down

compile: up
	docker-compose exec mimiker make

tmux: up
	docker-compose exec mimiker tmux

tag:
	docker image tag cahirwpz/mimiker-circleci:latest \
		         cahirwpz/mimiker-circleci:$(VERSION)

push:
	docker push cahirwpz/mimiker-circleci:latest
	docker push cahirwpz/mimiker-circleci:$(VERSION)

.PHONY: build push up down compile perms tmux tag push run
