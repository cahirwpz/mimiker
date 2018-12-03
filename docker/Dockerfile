FROM cahirwpz/mimiker-circleci:1.4

RUN apt-get install -y --no-install-recommends locales vim-nox tmux && \
    # UTF-8 is required to run tmux
    echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen

