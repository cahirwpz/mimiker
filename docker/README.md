Docker-based local development
---

Docker image with environment prepared for mimiker compilation and testing.

In order to quickly prepare local environment with all tools required to develop
mimiker, you will need to have `docker` and `docker-compose` installed.
To set up enviroment:

```
make run
```

The above script will
- attach volume with mimiker source code from your machine
- compile mimiker
- run docker container with tmux

Other commands reference:
 - `make build` - build mimiker image with 'latest' tag to docker hub
 - `make push` - push mimiker image to docker hub
 - `make up` - bring the container up
 - `make down` - bring the container down
 - `make compile` - compile mimiker kernel
 - `make tmux` - enter the container's shell in a tmux session
 - `make tag` - tag latest image with the version specified in Makefile
 - `make push` - push image with the version specified in Makefile

Possible issues:
- It is assumed that docker can be managed by non-root user. Take a look at the
first paragraph of Docker [post-installation](https://docs.docker.com/install/linux/linux-postinstall/) page if unsure.
