#!/usr/bin/env python3
"""Release sync checker for BasaltOS multi-repo workflow.

Validates release/version alignment signals in BasaltOS_Main and (optionally)
local sibling repos BasaltOS_Platform and BasaltOS_PlatformIO.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


VERSION_RE = re.compile(r"^v\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$")


def run_git(repo: Path, args: list[str]) -> str:
    cmd = ["git", *args]
    proc = subprocess.run(cmd, cwd=repo, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed in {repo}: {proc.stderr.strip()}")
    return proc.stdout.strip()


def read_text(path: Path) -> str:
    if not path.exists():
        raise FileNotFoundError(str(path))
    return path.read_text(encoding="utf-8")


def detect_latest_tag(repo: Path) -> str | None:
    out = run_git(repo, ["tag", "--list", "v*", "--sort=-v:refname"])
    tags = [t.strip() for t in out.splitlines() if t.strip()]
    return tags[0] if tags else None


def detect_version_from_changelog(repo: Path) -> str | None:
    text = read_text(repo / "CHANGELOG.md")
    m = re.search(r"^##\s+(v\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?)\b", text, re.MULTILINE)
    return m.group(1) if m else None


def check_main_release_signals(main_repo: Path, version: str, results: list[tuple[str, str, str]]) -> None:
    # tag exists
    try:
        run_git(main_repo, ["rev-parse", "-q", "--verify", f"refs/tags/{version}"])
        results.append(("OK", "main.tag", f"tag exists: {version}"))
    except RuntimeError:
        results.append(("FAIL", "main.tag", f"missing tag: {version}"))

    # changelog contains heading
    try:
        changelog = read_text(main_repo / "CHANGELOG.md")
        if re.search(rf"^##\s+{re.escape(version)}\b", changelog, re.MULTILINE):
            results.append(("OK", "main.changelog", f"CHANGELOG has heading for {version}"))
        else:
            results.append(("FAIL", "main.changelog", f"CHANGELOG missing heading for {version}"))
    except FileNotFoundError:
        results.append(("FAIL", "main.changelog", "missing CHANGELOG.md"))

    # readme status marker
    try:
        readme = read_text(main_repo / "README.md")
        if re.search(rf"Status\s*\(\s*{re.escape(version)}\s*\)", readme):
            results.append(("OK", "main.readme", f"README status references {version}"))
        else:
            results.append(("WARN", "main.readme", f"README status does not reference {version}"))
    except FileNotFoundError:
        results.append(("WARN", "main.readme", "missing README.md"))


def parse_release_sync_status(path: Path) -> dict[str, tuple[str, str]]:
    # returns repo -> (release, status)
    text = read_text(path)
    data: dict[str, tuple[str, str]] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if not line.startswith("|"):
            continue
        parts = [p.strip() for p in line.strip("|").split("|")]
        if len(parts) < 4:
            continue
        repo, release, status = parts[0], parts[1], parts[2]
        if repo in {"Repo", "---"}:
            continue
        if repo:
            data[repo] = (release, status)
    return data


def check_sync_status_doc(main_repo: Path, version: str, results: list[tuple[str, str, str]]) -> None:
    status_doc = main_repo / "docs" / "RELEASE_SYNC_STATUS.md"
    try:
        rows = parse_release_sync_status(status_doc)
    except FileNotFoundError:
        results.append(("FAIL", "status.doc", f"missing {status_doc}"))
        return

    expected = ["BasaltOS", "BasaltOS_Platform", "BasaltOS_PlatformIO"]
    for repo in expected:
        if repo not in rows:
            results.append(("FAIL", "status.doc", f"missing row for {repo}"))
            continue
        rel, st = rows[repo]
        if rel != version:
            results.append(("FAIL", "status.doc", f"{repo} release is {rel}, expected {version}"))
        else:
            results.append(("OK", "status.doc", f"{repo} release matches {version} (status={st})"))


def check_repo_binding(path: Path, expected_repo_slug: str, label: str, results: list[tuple[str, str, str]], required: bool) -> None:
    if not path.exists():
        sev = "FAIL" if required else "WARN"
        results.append((sev, label, f"missing local repo path: {path}"))
        return
    try:
        wt = run_git(path, ["rev-parse", "--is-inside-work-tree"])
        if wt != "true":
            results.append(("FAIL", label, f"not a git repo: {path}"))
            return
    except RuntimeError as err:
        sev = "FAIL" if required else "WARN"
        results.append((sev, label, str(err)))
        return

    try:
        origin = run_git(path, ["remote", "get-url", "origin"])
    except RuntimeError as err:
        sev = "FAIL" if required else "WARN"
        results.append((sev, label, str(err)))
        return

    if expected_repo_slug in origin:
        results.append(("OK", label, f"origin matches expected repo ({expected_repo_slug})"))
    else:
        results.append(("WARN", label, f"origin does not include '{expected_repo_slug}': {origin}"))


def main() -> int:
    ap = argparse.ArgumentParser(description="Check BasaltOS cross-repo release alignment")
    ap.add_argument("--version", default="", help="Release tag/version (example: v0.1.0). Auto-detected if omitted.")
    ap.add_argument("--main-repo", default=".", help="Path to BasaltOS_Main repo")
    ap.add_argument("--platform-repo", default="../BasaltOS_Platform", help="Path to BasaltOS_Platform repo")
    ap.add_argument("--platformio-repo", default="../BasaltOS_PlatformIO", help="Path to BasaltOS_PlatformIO repo")
    ap.add_argument("--self-only", action="store_true", help="Only run checks within BasaltOS_Main")
    ap.add_argument("--require-siblings", action="store_true", help="Fail if sibling repos are missing/unavailable")
    args = ap.parse_args()

    main_repo = Path(args.main_repo).resolve()
    platform_repo = Path(args.platform_repo).resolve()
    platformio_repo = Path(args.platformio_repo).resolve()

    results: list[tuple[str, str, str]] = []

    version = args.version.strip()
    if version:
        if not VERSION_RE.match(version):
            print(f"[FAIL] input.version: invalid version format '{version}'", file=sys.stderr)
            return 2
    else:
        version = detect_latest_tag(main_repo) or ""
        if not version:
            version = detect_version_from_changelog(main_repo) or ""
        if not version:
            print("[FAIL] version.detect: unable to infer version from tags or CHANGELOG", file=sys.stderr)
            return 2

    results.append(("OK", "version", f"target release: {version}"))

    check_main_release_signals(main_repo, version, results)
    check_sync_status_doc(main_repo, version, results)

    if not args.self_only:
        check_repo_binding(main_repo, "homer1013/BasaltOS", "repo.main", results, required=True)
        check_repo_binding(platform_repo, "homer1013/BasaltOS_Platform", "repo.platform", results, required=args.require_siblings)
        check_repo_binding(platformio_repo, "homer1013/BasaltOS_PlatformIO", "repo.platformio", results, required=args.require_siblings)

    fails = 0
    warns = 0
    for sev, key, msg in results:
        print(f"[{sev}] {key}: {msg}")
        if sev == "FAIL":
            fails += 1
        elif sev == "WARN":
            warns += 1

    print(f"summary: fail={fails} warn={warns} release={version}")
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(main())
