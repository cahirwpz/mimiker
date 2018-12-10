Docker-based local development
---

Docker image with environment prepared for mimiker compilation and testing.

In order to quickly prepare local environment with all tools required to develop
mimiker, you will need to have `docker` and `docker-compose` installed.
To set up enviroment:

    `make run`

The above script will
- attach volume with mimiker source code from your machine
- compile mimiker
- run docker container with tmux

Other commands reference:

`make build` - build the mimiker-dev image with tag latest
`make push` - push the mimiker-dev image to the docker hub
`make up` - bring the containers up
`make compile` - compile the mimiker kernel
`make tmux` - enter the container's shell in a tmux session

If you would like to tag the built image with a specific version, please run:

```
docker image tag cahirwpz/mimiker-circleci:latest \
                  cahirwpz/mimiker-circleci:X.Y
```

where X, Y are version numbers, and then run the following command to push the
tag to the docker hub:

 `docker push cahirwpz/mimiker-circleci:X.Y`


Possible issues:
- It is assumed that docker can be managed by non-root user. Take a look at the
first paragraph of the [following page](https://docs.docker.com/install/linux/linux-postinstall/) if unsure

