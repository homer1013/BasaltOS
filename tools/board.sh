#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOARDS_DIR="$ROOT_DIR/boards"

usage() {
  cat <<'EOF'
usage:
  tools/board.sh --list
  tools/board.sh <selector>

selector can be:
  - <platform>/<board_dir>   (example: esp32/cyd_3248s035r)
  - <board_dir>              (if unique)
  - <board_id>               (board.json "id", if unique)
EOF
}

list_boards() {
  python - "$BOARDS_DIR" <<'PY'
import json
import sys
from pathlib import Path

boards_dir = Path(sys.argv[1])
rows = []
for board_json in sorted(boards_dir.rglob("board.json")):
    platform = board_json.parent.parent.name
    board_dir = board_json.parent.name
    data = json.loads(board_json.read_text(encoding="utf-8"))
    board_id = str(data.get("id", board_dir))
    has_profile = (board_json.parent / "sdkconfig.defaults").exists() and (board_json.parent / "partitions.csv").exists()
    rows.append((platform, board_dir, board_id, has_profile))

if not rows:
    print("No boards found.")
    sys.exit(0)

print("Available boards:")
for platform, board_dir, board_id, has_profile in rows:
    marker = "profile-ready" if has_profile else "metadata-only"
    print(f"  - {platform}/{board_dir:22s} id='{board_id}'  [{marker}]")
PY
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

if [[ "$1" == "--list" ]]; then
  list_boards
  exit 0
fi

SELECTOR="$1"
if [[ ! -d "$BOARDS_DIR" ]]; then
  echo "missing boards directory: $BOARDS_DIR"
  exit 1
fi

if ! RESOLVED="$(
python - "$BOARDS_DIR" "$SELECTOR" <<'PY'
import json
import sys
from pathlib import Path

boards_dir = Path(sys.argv[1])
selector = sys.argv[2].strip()

rows = []
for board_json in boards_dir.rglob("board.json"):
    platform = board_json.parent.parent.name
    board_dir = board_json.parent.name
    data = json.loads(board_json.read_text(encoding="utf-8"))
    board_id = str(data.get("id", board_dir))
    target = str(data.get("target", "")).strip()
    has_profile = (board_json.parent / "sdkconfig.defaults").exists() and (board_json.parent / "partitions.csv").exists()
    has_board_cfg = (board_json.parent / "board_config.h").exists()
    rows.append({
        "platform": platform,
        "board_dir": board_dir,
        "board_id": board_id,
        "target": target,
        "path": str(board_json.parent),
        "has_profile": has_profile,
        "has_board_cfg": has_board_cfg,
    })

if not rows:
    print("ERROR=no boards found")
    sys.exit(2)

selector_norm = selector.replace("\\", "/")
parts = selector_norm.split("/")
matches = []

if len(parts) == 2 and parts[0] and parts[1]:
    p, b = parts
    matches = [r for r in rows if r["platform"] == p and r["board_dir"] == b]
else:
    by_dir = [r for r in rows if r["board_dir"] == selector_norm]
    if len(by_dir) == 1:
        matches = by_dir
    elif len(by_dir) > 1:
        print("ERROR=ambiguous board_dir selector")
        for r in sorted(by_dir, key=lambda x: (x["platform"], x["board_dir"])):
            print(f"CANDIDATE={r['platform']}/{r['board_dir']} (id={r['board_id']})")
        sys.exit(3)
    else:
        by_id = [r for r in rows if r["board_id"] == selector_norm]
        if len(by_id) == 1:
            matches = by_id
        elif len(by_id) > 1:
            print("ERROR=ambiguous board_id selector")
            for r in sorted(by_id, key=lambda x: (x["platform"], x["board_dir"])):
                print(f"CANDIDATE={r['platform']}/{r['board_dir']} (id={r['board_id']})")
            sys.exit(3)

if len(matches) != 1:
    print(f"ERROR=unknown board selector: {selector}")
    sys.exit(4)

r = matches[0]
print(f"BOARD_PATH={r['path']}")
print(f"PLATFORM={r['platform']}")
print(f"BOARD_DIR={r['board_dir']}")
print(f"BOARD_ID={r['board_id']}")
print(f"TARGET={r['target']}")
print(f"HAS_PROFILE={1 if r['has_profile'] else 0}")
print(f"HAS_BOARD_CONFIG={1 if r['has_board_cfg'] else 0}")
PY
)"; then
  echo "$RESOLVED"
  echo ""
  echo "Use 'tools/board.sh --list' to see valid selectors."
  exit 1
fi

BOARD_PATH=""
PLATFORM=""
BOARD_DIR=""
BOARD_ID=""
TARGET=""
HAS_PROFILE="0"
HAS_BOARD_CONFIG="0"
while IFS='=' read -r key value; do
  case "$key" in
    BOARD_PATH) BOARD_PATH="$value" ;;
    PLATFORM) PLATFORM="$value" ;;
    BOARD_DIR) BOARD_DIR="$value" ;;
    BOARD_ID) BOARD_ID="$value" ;;
    TARGET) TARGET="$value" ;;
    HAS_PROFILE) HAS_PROFILE="$value" ;;
    HAS_BOARD_CONFIG) HAS_BOARD_CONFIG="$value" ;;
  esac
done <<< "$RESOLVED"

if [[ -z "$BOARD_PATH" || ! -d "$BOARD_PATH" ]]; then
  echo "$RESOLVED"
  echo ""
  echo "Use 'tools/board.sh --list' to see valid selectors."
  exit 1
fi

if [[ "$HAS_PROFILE" != "1" ]]; then
  echo "Board resolved: $PLATFORM/$BOARD_DIR (id=$BOARD_ID)"
  echo "This board is metadata-only and does not provide sdkconfig/partitions profile files yet."
  echo ""
  echo "Use the configurator flow instead:"
  echo "  python tools/configure.py --platform $PLATFORM --board $BOARD_DIR"
  exit 1
fi

cp "$BOARD_PATH/sdkconfig.defaults" "$ROOT_DIR/sdkconfig.defaults"
cp "$BOARD_PATH/partitions.csv" "$ROOT_DIR/partitions.csv"

if [[ "$HAS_BOARD_CONFIG" == "1" ]]; then
  cp "$BOARD_PATH/board_config.h" "$ROOT_DIR/runtime/python/basalt_mod/board_config.h"
  cp "$BOARD_PATH/board_config.h" "$ROOT_DIR/runtime/python/micropython_embed/port/board_config.h"
fi

echo "Board profile set to: $PLATFORM/$BOARD_DIR (id=$BOARD_ID)"
if [[ -n "$TARGET" ]]; then
  echo "Recommended target: idf.py set-target $TARGET"
fi
