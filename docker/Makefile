run: compile tmux

build:
	docker build --no-cache . -t cahirwpz/mimiker-circleci:latest

perms:
	# Set the directory permissions to hardcoded GID
	sudo chown :1024 ../
	sudo chmod 775 ../
	# Ensure new files will inherit the rigth group ID
	sudo chmod g+s ../

push:
	docker push cahirwpz/mimiker-circleci:latest

up: perms
	docker-compose up -d

down:
	docker-compose down

compile: up
	docker-compose exec mimiker make

tmux: up
	docker-compose exec mimiker tmux

.PHONY: build push up down compile perms tmux run
