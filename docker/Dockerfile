# TODO use :latest when it will be available on docker hub
FROM cahirwpz/mimiker-circleci:1.6

# XXX: Create a group in which we will operate with a hardcoded GID,
# so that we are able to set proper directory permissions on host
# for volume with source code.
RUN addgroup --gid 1024 mimiker && \
    adduser --disabled-password --gecos "" --force-badname \
    --ingroup mimiker --ingroup sudo mimiker

WORKDIR /home/mimiker

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
    less locales patch file sudo zsh vim-nox tmux tig ack
RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
    locale-gen && update-locale LANG="en_US.UTF-8"
RUN touch .gitconfig && \
     echo "add-auto-load-safe-path ~/mimiker/.gdbinit" > .gdbinit && \
     echo "mimiker  ALL=(ALL)   NOPASSWD: ALL" >> /etc/sudoers

USER mimiker
# ADD https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh \
#     zsh-install.sh
# RUN bash zsh-install.sh
