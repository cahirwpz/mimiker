VERSION = 1.7.2

all:

build:
	docker build . -t cahirwpz/mimiker-circleci:latest

tag:
	docker image tag cahirwpz/mimiker-circleci:latest \
		         cahirwpz/mimiker-circleci:$(VERSION)

push:
	docker push cahirwpz/mimiker-circleci:latest
	docker push cahirwpz/mimiker-circleci:$(VERSION)

clean:
	rm -v -f *~

.PHONY: build push tag clean
