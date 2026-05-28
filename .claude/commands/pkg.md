---
description: Commit pending changes, ship a release-please release, and rebuild the Arch package.
---

# /pkg — release and rebuild the Arch package

Drive the full ship pipeline for this project: commit → push → release-please → tag → PKGBUILD bump → `makepkg`.

Project specifics:
- GitHub repo: `iharob/kwhatstux`
- Default branch: `main`
- Release tooling: release-please (workflow `release-please.yml`), v-prefixed tags (e.g. `v1.2.1`)
- PR style: squash-merge release-please PRs (`gh pr merge <n> --squash --auto`)
- Arch package source dir: `/data/AUR/kwhatstux/` (PKGBUILD, `.SRCINFO`)
- The source tarball is downloaded from `https://github.com/iharob/kwhatstux/archive/refs/tags/v<pkgver>.tar.gz`
- Per `/home/iharob/.claude/CLAUDE.md`: do NOT add `Co-Authored-By: Claude` unless Claude wrote a significant portion. Run `git status`/`git diff` first to judge.

## Steps

1. **Inspect working tree.** `git -C /data/Code/kwhatstux status` + `git diff`. If clean, skip to step 3.
2. **Commit pending changes.**
   - Pick the conventional-commit prefix that fits the diff (`fix:`, `feat:`, `chore:`, etc.). If the user passed a prefix as an argument, honor it.
   - Write a short subject + body explaining *why*, not *what*.
   - Stage explicit files (no `git add -A`).
3. **Push to `main`.** If push is rejected, `git pull --rebase origin main` then retry. Pushing to `main` will prompt for permission — that is expected; ask the user to approve once.
4. **Wait for release-please.** Poll the latest run of `release-please.yml`:
   ```
   until gh -R iharob/kwhatstux run view <runId> --json status -q .status | grep -q completed; do sleep 4; done
   ```
   Then list open PRs and grab the `chore(main): release <ver>` PR number.
5. **Merge the release PR.** `gh -R iharob/kwhatstux pr merge <n> --squash --auto`. Wait for it to merge.
6. **Wait for the tag/release** to be cut by the second release-please run:
   ```
   until gh -R iharob/kwhatstux release view v<ver> --json tagName -q .tagName 2>/dev/null | grep -q v<ver>; do sleep 4; done
   ```
7. **Download the new tarball** to `/tmp/kwhatstux-<ver>.tar.gz` and compute `sha256sum`.
8. **Update `/data/AUR/kwhatstux/PKGBUILD`:**
   - Bump `pkgver=` to the new version.
   - Replace `sha256sums=(...)` with the freshly computed hash.
   - Leave `pkgrel=1` unless the user asks for a rebuild bump.
9. **Regenerate `.SRCINFO`:** `cd /data/AUR/kwhatstux && makepkg --printsrcinfo > .SRCINFO`.
10. **Clean stale build state** before rebuilding (CMake cache from previous `pkgver` will otherwise complain):
    ```
    rm -rf /data/AUR/kwhatstux/src/build /data/AUR/kwhatstux/src/kwhatstux-* /data/AUR/kwhatstux/pkg
    ```
11. **Build:** `cd /data/AUR/kwhatstux && makepkg -f`. Use `Bash` `timeout: 600000`.
12. **Report:** the resulting `kwhatstux-<ver>-1-x86_64.pkg.tar.zst` path and install hint:
    `sudo pacman -U /data/AUR/kwhatstux/kwhatstux-<ver>-1-x86_64.pkg.tar.zst`.

## Gotchas to watch for

- `release-please.yml` runs twice per release: once on the source-commit push (opens the release PR), once on the release-PR merge (cuts the tag). Don't confuse them.
- The PKGBUILD lives **outside** this git repo (`/data/AUR/kwhatstux/`); do not try to commit it from inside `/data/Code/kwhatstux`.
- The AUR repo isn't pushed automatically — if the user wants the AUR updated, ask before `git push`ing inside `/data/AUR/kwhatstux/`.
- `makepkg` from a previous version leaves an extracted source tree (`src/kwhatstux-<oldver>/`) and a CMake cache that pin the old source path. Always wipe `src/` before rebuilding a new version.
