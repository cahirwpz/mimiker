#!/bin/bash

nocolor='\033[0m'
col_red='\033[0;31m'
col_orange='\033[0;33m'
col_blue='\033[0;34m'

function fatal() {
    echo -e "${col_red}ERROR${nocolor}: "$1
    exit 1
}

function warn() {
    echo -e "${col_orange}WARN${nocolor}: "$1
}

function info() {
    echo -e "${col_blue}INFO${nocolor}: "$1
}

pre_commit_install=1

# Check if pre-commit hook exists
if [ -f .git/hooks/pre-commit ]; then
  warn "You already have a pre-commit hook installed. Please remove it if you want to use our hook."
  read -p "Do you want to install pre-commit hook? (Y/n) " choice
  case $choice in
      [yY] ) ;;
        * ) pre_commit_install=0;
          info "Won't install pre-commit hook";;
  esac
fi

if [ $pre_commit_install -eq 1 ]; then
  ln -sf ../../scripts/pre-commit .git/hooks/pre-commit \
    || fatal "Failed to install pre-commit hook"
fi
