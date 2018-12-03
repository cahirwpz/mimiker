Docker-based local development
---

In order to quickly prepare local environment with all tools required to develop
mimiker, containerization may be used. You will need to have `docker` and
`docker-compose` installed to set up environment:

```
export MIMIKER_SRC_PATH="<path to mimiker source>"
./run_mimiker.sh
```

The above script will
- build `mimiker-dev` docker image
- attach volume with mimiker source code from your machine
- compile mimiker
- run docker container with tmux

Other commands reference:

`make tmux` - enter the container's shell in a tmux session
`make compile` - compile the mimiker kernel
`make build` - build the mimiker-dev image with tag latest
`make push` - push the mimiker-dev image to the docker hub

Possible issues:
- It is assumed that docker can be managed by non-root user. Take a look at the
first paragraph of the [following page](https://docs.docker.com/install/linux/linux-postinstall/) if unsure

