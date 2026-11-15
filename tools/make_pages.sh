#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 TRAPSTATION OS contributors

set -e
cd "$(dirname "$0")/.."
PKG="$1"; VER="$2"
[ -f "$PKG" ] && [[ "$VER" =~ ^[0-9](\.[0-9])+$ ]] || { echo "usage: $0 PACKAGE.fwsc VERSION"; exit 1; }
if grep -q "UnkleBlairy/BBL-JAM-BOX" web/home.html web/index_pkg.html web/samples.html; then
  echo "the pages still link BBL JAM BOX's source: run tools/set_repo_url.sh OWNER/REPO first"; exit 1
fi
SITE=$(mktemp -d); IDX=$(mktemp -u)
trap 'rm -rf "$SITE" "$IDX"' EXIT
python3 web/make_site.py "$PKG" "$VER" "$SITE"
PARENT=$(git rev-parse -q --verify refs/heads/gh-pages || true)
GD=$(git rev-parse --absolute-git-dir)
TREE=$(cd "$SITE" && export GIT_DIR="$GD" GIT_WORK_TREE="$SITE" GIT_INDEX_FILE="$IDX" && git add -A -f && git write-tree)
COMMIT=$(git commit-tree "$TREE" ${PARENT:+-p "$PARENT"} -m "Installer site for TRAPSTATION OS $VER")
git update-ref refs/heads/gh-pages "$COMMIT"
echo "gh-pages: $(git log --oneline -1 gh-pages)  ($(git ls-tree -r --name-only gh-pages | wc -l) files)"
echo "next: git push origin gh-pages, then Settings > Pages > gh-pages"
