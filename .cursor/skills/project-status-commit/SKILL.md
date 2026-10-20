---
name: project-status-commit
description: >-
  Pre-commit and status ritual for the CLIENT_BASE project. Use before every
  git commit, when the user asks for status, or when ending a work session on
  this feature branch.
---

# Project status & pre-commit

## Status ritual

1. Read `.cursor/client-base-project/KANBAN.md` and `TODO.md`.
2. Summarize for the user: Done / In progress / Blocked / Next.
3. Diff knowledge base vs code — fix doc drift immediately.

## Before `git commit`

Checklist (all must pass or be explicitly waived in KNOWLEDGE_BASE):

- [ ] `KANBAN.md` columns updated
- [ ] `TODO.md` updated
- [ ] `KNOWLEDGE_BASE.md` has new findings/decisions from this chunk
- [ ] `README.md` / `use-cases.md` still accurate
- [ ] Unit tests added or updated for behavior changes
- [ ] `trunk fmt` on touched sources
- [ ] No upstream-forbidden root docs; no secrets
- [ ] Commit message explains *why*

Then commit only when the user asked for a commit.
