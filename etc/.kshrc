export PATH=/bin/ubase:$PATH
export TERM=xterm
if [ "$(stty size)" == "0 0" ]; then
  setwinsize
fi
