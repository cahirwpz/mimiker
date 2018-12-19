VERSION = 1.0

run: compile tmux

build:
	docker build --no-cache . -t cahirwpz/mimiker-dev:latest

perms:
	# Set the directory permissions to hardcoded GID
	sudo chown :1024 ../
	sudo chmod 775 ../
	# Ensure new files will inherit the rigth group ID
	sudo chmod g+s ../
	touch $@

up: perms
	docker-compose up -d

down:
	docker-compose down

compile: up
	docker-compose exec mimiker make

tmux: up
	docker-compose exec mimiker tmux

tag:
	docker image tag cahirwpz/mimiker-dev:latest \
		         cahirwpz/mimiker-dev:$(VERSION)

push:
	docker push cahirwpz/mimiker-dev:latest
	docker push cahirwpz/mimiker-dev:$(VERSION)

clean:
	docker container prune --filter "until=24h"
	docker image prune

.PHONY: build push up down compile tmux tag push run clean
