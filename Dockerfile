FROM mimiker:latest

RUN apt-get install -y --no-install-recommends locales vim gdb tmux
# UTF-8 is required to run tmux
RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen

