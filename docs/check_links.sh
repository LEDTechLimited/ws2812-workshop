#!/usr/bin/env bash
# Broken relative-link check (docs/ + root *.md). Skips fenced ``` blocks and inline `code`.
set -u
cd "$(dirname "$0")/.." || exit 2
rc=0
files=$( { find docs -name '*.md'; find . -maxdepth 1 -name '*.md'; } | sort -u )
while IFS= read -r f; do
  [ -f "$f" ] || continue
  dir=$(dirname "$f")
  links=$(awk 'BEGIN{fence=0} /^[[:space:]]*```/{fence=!fence;next} !fence' "$f" \
          | sed 's/`[^`]*`//g' | grep -oE '\]\([^)]+\)' | sed -E 's/^\]\(//; s/\)$//')
  while IFS= read -r link; do
    [ -z "$link" ] && continue
    case "$link" in http*|\#*|mailto:*|/*) continue ;; esac
    t="${link%%#*}"; [ -z "$t" ] && continue
    [ -e "$dir/$t" ] || { echo "✗ $f → $t (broken link)"; rc=1; }
  done <<< "$links"
done <<< "$files"
[ "$rc" -eq 0 ] && echo "✓ no broken relative links"
exit "$rc"
