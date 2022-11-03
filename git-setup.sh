#!/bin/bash

# Check if pre-commit hook exists
[ -f .git/hooks/pre-commit ] \
  && echo "You already have a pre-commit hook installed. Please remove it if you want to use our hook." \
  && exit 1

# Install the pre-commit hook
ln -s ./scripts/pre-commit .git/hooks/pre-commit \
  || echo "Failed to install pre-commit hook" \
  && exit 1

echo Hooks successfully installed.
exit 0
