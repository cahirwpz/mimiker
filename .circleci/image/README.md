CircleCI image
==============

Docker image with environment prepared for mimiker compilation and testing.

Commands reference:

`make build` - build the mimiker-circleci image with tag latest
`make push` - push the mimiker-circleci image to the docker hub

If you would like to tag the built image with a specific version, please run:

```
docker image tag cahirwpz/mimiker-circleci:latest \
                  cahirwpz/mimiker-circleci:X.Y
```

where X, Y are version numbers, and then run the following command to push the
tag to the docker hub:

`docker push cahirwpz/mimiker-circleci:X.Y`

