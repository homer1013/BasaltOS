#!/usr/bin/env python3
import argparse
import os
import shutil
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
TEMPLATE_DIR = os.path.join(REPO_ROOT, "sdk", "templates")


def load_template(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)


def main():
    parser = argparse.ArgumentParser(description="Create a new Basalt app skeleton")
    parser.add_argument("name", help="App folder name (e.g. blink_rgb)")
    parser.add_argument(
        "--dest",
        default="apps",
        help="Destination base folder (default: apps)",
    )
    parser.add_argument(
        "--title",
        default=None,
        help="Display name for app.toml (default: derived from name)",
    )
    parser.add_argument(
        "--runtime",
        choices=["python", "lua"],
        default="python",
        help="App runtime and default entry file (default: python)",
    )
    args = parser.parse_args()

    app_name = args.name.strip().rstrip("/").rstrip("\\")
    if not app_name:
        print("error: invalid app name", file=sys.stderr)
        return 1

    if os.path.isabs(args.dest):
        dest_root = args.dest
    else:
        dest_root = os.path.join(REPO_ROOT, args.dest)

    app_dir = os.path.join(dest_root, app_name)
    if os.path.exists(app_dir):
        print(f"error: destination exists: {app_dir}", file=sys.stderr)
        return 1

    title = args.title or app_name.replace("_", " ").title()
    runtime = args.runtime
    entry_name = "main.lua" if runtime == "lua" else "main.py"

    app_toml_path = os.path.join(TEMPLATE_DIR, "app.toml")
    main_py_path = os.path.join(TEMPLATE_DIR, "main.py")
    main_lua_path = os.path.join(TEMPLATE_DIR, "main.lua")
    if not os.path.isfile(app_toml_path) or not os.path.isfile(main_py_path) or not os.path.isfile(main_lua_path):
        print("error: template files missing under sdk/templates", file=sys.stderr)
        return 1

    app_toml = load_template(app_toml_path)
    app_toml = app_toml.replace('name = "My App"', f'name = "{title}"')
    app_toml = app_toml.replace('entry = "main.py"', f'entry = "{entry_name}"')
    app_toml = app_toml.replace('runtime = "python"', f'runtime = "{runtime}"')

    entry_template = main_lua_path if runtime == "lua" else main_py_path
    entry_content = load_template(entry_template)

    os.makedirs(app_dir, exist_ok=True)
    write_file(os.path.join(app_dir, "app.toml"), app_toml)
    write_file(os.path.join(app_dir, entry_name), entry_content)

    print(f"Created app: {app_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
