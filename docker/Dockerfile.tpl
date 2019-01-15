# vim: syntax=dockerfile:
# TODO use :latest when it will be available on docker hub
FROM cahirwpz/mimiker-circleci:1.6

RUN addgroup --gid ${MIMIKER_GID} mimiker && \
    adduser --uid ${MIMIKER_UID} --disabled-password \
    --gecos "" --force-badname \
    --ingroup mimiker --ingroup sudo mimiker

WORKDIR /home/mimiker

RUN apt-get update -q || apt-get update -q
RUN apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
    less locales patch file sudo zsh vim-nox tmux tig ack
RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
    locale-gen && update-locale LANG="en_US.UTF-8"
RUN touch .gitconfig && \
     echo "add-auto-load-safe-path ~/mimiker/.gdbinit" > .gdbinit && \
     echo "mimiker  ALL=(ALL)   NOPASSWD: ALL" >> /etc/sudoers

ADD https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh \
    zsh-install.sh
RUN sed -ie "s/~\//\/home\/mimiker\//g" zsh-install.sh && \
    bash zsh-install.sh && \
    chsh -s $(which zsh) mimiker
USER mimiker
