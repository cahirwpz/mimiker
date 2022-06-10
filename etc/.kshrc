export TERM=xterm
if [ "$(stty size)" == "0 0" ]; then
  setwinsize
fi
