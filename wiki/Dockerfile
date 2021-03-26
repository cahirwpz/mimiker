# This file is a fork of GitHub Wiki Action plugin by Andrew Chen Wang
# https://github.com/Andrew-Chen-Wang/github-wiki-action

FROM alpine/git

RUN apk add rsync

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
