#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

VERSION="$(python3 - <<'PY'
import re
from pathlib import Path
text = Path("docs/RELEASE_SYNC_STATUS.md").read_text(encoding="utf-8")
for line in text.splitlines():
    m = re.match(r"\|\s*BasaltOS\s*\|\s*(v[0-9A-Za-z.+-]+)\s*\|", line.strip())
    if m:
        print(m.group(1))
        break
else:
    raise SystemExit("unable to detect BasaltOS release version from docs/RELEASE_SYNC_STATUS.md")
PY
)"

# In CI, release docs may be bumped before the release tag is pushed.
# Create a local ephemeral tag for smoke validation when needed.
if ! git rev-parse -q --verify "refs/tags/$VERSION" >/dev/null 2>&1; then
  git tag "$VERSION" HEAD
  trap 'git tag -d "$VERSION" >/dev/null 2>&1 || true' EXIT
fi

python3 tools/release_sync_check.py --self-only --version "$VERSION" >/tmp/release_sync_check.out

grep -q "\[OK\] main.tag: tag exists: $VERSION" /tmp/release_sync_check.out
grep -q "\[OK\] main.changelog: CHANGELOG has heading for $VERSION" /tmp/release_sync_check.out
grep -q "\[OK\] status.doc: BasaltOS release matches $VERSION" /tmp/release_sync_check.out
grep -q "\[OK\] status.doc: BasaltOS_Platform release matches $VERSION" /tmp/release_sync_check.out
grep -q "\[OK\] status.doc: BasaltOS_PlatformIO release matches $VERSION" /tmp/release_sync_check.out

echo "PASS: release sync check smoke"
