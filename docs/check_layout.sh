#!/usr/bin/env bash
# Enforce the closed docs taxonomy (see docs/index.md). Run manually, in CI, or as a
# pre-commit hook. Exits non-zero on any violation.
#
# Rule: inside docs/, the ONLY allowed top-level entries are the convention's buckets (dirs)
# plus index.md / README.md. No ad-hoc top-level docs or invented categories.
set -u
cd "$(dirname "$0")/.." || exit 2   # repo root (parent of docs/)

BUCKETS="architecture adr decisions reference how-to runbooks tutorials archive"
ALLOWED_FILES="index.md README.md"
rc=0

check_docs_dir() {
  local d="$1"
  [ -d "$d" ] || return 0
  for entry in "$d"/*; do
    [ -e "$entry" ] || continue
    local base; base="$(basename "$entry")"
    if [ -d "$entry" ]; then
      case " $BUCKETS _templates " in *" $base "*) ;; *)   # _templates = doc-type skeletons
        echo "✗ $d/$base/ — not an allowed bucket ($BUCKETS)"; rc=1 ;;
      esac
    else
      case "$base" in
        *.md)
          case " $ALLOWED_FILES " in *" $base "*) ;; *)
            echo "✗ $entry — stray top-level doc; move it into a bucket (see docs/index.md)"; rc=1 ;;
          esac ;;
        *.sh|.DS_Store) ;;                       # lint scripts + OS cruft are fine
        *) echo "⚠ $entry — non-doc file in docs/ root (code/migrations belong with code)" ;;
      esac
    fi
  done
}

check_docs_dir "docs"
[ "$rc" -eq 0 ] && echo "✓ docs layout OK (closed taxonomy respected)"
exit "$rc"
