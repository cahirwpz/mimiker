#!/bin/sh

# This file is a fork of GitHub Wiki Action plugin by Andrew Chen Wang
# https://github.com/Andrew-Chen-Wang/github-wiki-action

TEMP_CLONE_FOLDER="temp_wiki_$GITHUB_SHA"

# This script assumes there is rsync and git installed in executing container.

if [ -z "$GH_TOKEN" ]; then
  echo "GH_TOKEN ENV is missing."
  exit 1
fi

if [ -z "$GH_MAIL" ]; then
  echo "GH_MAIL ENV is missing."
  exit 1
fi

if [ -z "$GH_NAME" ]; then
  echo "GH_NAME ENV is missing."
  exit 1
fi

REPO=$GITHUB_REPOSITORY
WIKI_DIR="wiki/docs/"

echo "Configuring wiki git..."
mkdir $TEMP_CLONE_FOLDER
cd $TEMP_CLONE_FOLDER
git init

# Setup credentials
git config user.name $GH_NAME
git config user.email $GH_MAIL

git pull https://$GH_TOKEN@github.com/$REPO.wiki.git
cd ..

# Get commit message
message=$(git log -1 --format=%B)
echo "Message:"
echo $message

echo "Copying files to Wiki"
rsync -av --delete $WIKI_DIR $TEMP_CLONE_FOLDER/ --exclude .git

echo "Pushing to Wiki"
cd $TEMP_CLONE_FOLDER
git add .
git commit -m "$message"
git push --set-upstream https://$GH_NAME:$GH_TOKEN@github.com/$REPO.wiki.git master

