#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 TRAPSTATION OS contributors

set -e
cd "$(dirname "$0")/.."
R="$1"
[[ "$R" =~ ^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$ ]] || { echo "usage: $0 OWNER/REPO"; exit 1; }
O="${R%%/*}"; O="${O,,}"; N="${R#*/}"                 # Pages hosts are lower case: OWNER.github.io/REPO
grep -rl 'OWNER' README.md README.de.md INSTALL.md INSTALL.de.md docs 2>/dev/null |
  xargs -r sed -i "s#OWNER\.github\.io/REPO#$O.github.io/$N#g; s#OWNER/REPO#$R#g; s#github\.com/OWNER/#github.com/${R%%/*}/#g"
for f in web/index_pkg.html web/home.html web/samples.html; do
  [ -f "$f" ] || continue
  sed -i "s#https://github.com/UnkleBlairy/BBL-JAM-BOX#https://github.com/$R#g; s#>github.com/UnkleBlairy/BBL-JAM-BOX<#>github.com/$R<#g" "$f"
done
python3 - "$R" <<'PY'
import json, sys
p = "fmhub.json"
d = json.load(open(p, encoding="utf-8"))
d["cheat"] = f"https://github.com/{sys.argv[1]}/raw/main/docs/cheatsheet/Trapstation-Cheat-Sheet.pdf"
open(p, "w", encoding="utf-8").write(json.dumps(d, indent=2, ensure_ascii=False) + "\n")
PY
echo "set to https://github.com/$R"
grep -rn "UnkleBlairy/BBL-JAM-BOX" web/index_pkg.html web/home.html web/samples.html || echo "web: no BBL source links left"
grep -rn -E "OWNER" README.md README.de.md INSTALL.md INSTALL.de.md docs fmhub.json || echo "no placeholders left"
