#!/bin/sh

TEMP_CLONE_FOLDER="temp_wiki_$GITHUB_SHA"
TEMP_EXCLUDED_FILE="temp_wiki_excluded_$GITHUB_SHA.txt"

if [ -z "$GH_TOKEN" ]; then
  echo "GH_TOKEN ENV is missing. Use $\{{ secrets.GITHUB_TOKEN }} or a PAT if your wiki repo is different from your current repo."
  exit 1
fi

if [ -z "$GH_MAIL" ]; then
  echo "GH_MAIL ENV is missing. Use the email for a user that can push to this repository."
  exit 1
fi

if [ -z "$GH_NAME" ]; then
  echo "GH_NAME ENV is missing. Use the username for a user that can push to this repository."
  exit 1
fi

if [ -z "$REPO" ]; then
  echo "REPO ENV is missing. Using the current one."
  REPO=$GITHUB_REPOSITORY
fi

if [ -z "$WIKI_DIR" ]; then
    echo "WIKI_FOLDER ENV is missing, using default wiki/"
    WIKI_DIR='wiki/'
fi

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
if [ -z "$WIKI_PUSH_MESSAGE" ]; then
  message=$(git log -1 --format=%B)
else
  message=$WIKI_PUSH_MESSAGE
fi
echo "Message:"
echo $message

echo "Copying files to Wiki"
# Configuring a file to exclude specified files
if [ -z "$EXCLUDED_FILES" ]; then
  rsync -av --delete $WIKI_DIR $TEMP_CLONE_FOLDER/ --exclude .git
else
  for file in $EXCLUDED_FILES; do
    echo "$file" >> ./$TEMP_EXCLUDED_FILE
  done
  rsync -av --delete $WIKI_DIR $TEMP_CLONE_FOLDER/ --exclude .git --exclude-from=$TEMP_EXCLUDED_FILE
  # Delete files in target repo if it was a reminant.
  for file in $EXCLUDED_FILES; do
    rm -r $TEMP_CLONE_FOLDER/$file
  done
fi

echo "Pushing to Wiki"
cd $TEMP_CLONE_FOLDER
git add .
git commit -m "$message"
git push --set-upstream https://$GH_NAME:$GH_TOKEN@github.com/$REPO.wiki.git master || git push --set-upstream https://$GH_TOKEN@github.com/$REPO.wiki.git master

