#!/bin/bash

# brew command to remove all installed formulas but maintain Casks (items installed into /Applications)
echo "removing brew installed formula"
brew remove --force $(brew list --formulae)

echo "removing py3.9 site-packages"
rm -rf /opt/homebrew/lib/python3.9/site-packages/