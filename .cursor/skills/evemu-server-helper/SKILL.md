---
name: evemu-server-helper
description: Troubleshoots EVEmu server issues and executes the repo workflow for build failures, login black screen triage, database migration/seeding, and safe git commit/push steps. Use when the user reports EVEmu runtime errors, Docker/CMake build errors, chat/market/wallet regressions, or asks to commit/push/deploy fixes.
---

# EVEmu Server Helper

## Purpose
Handle common EVEmu maintenance loops quickly:
- Diagnose and fix C++ build/compiler errors.
- Triage black-screen/login/session issues.
- Apply DB migration and market seeding fixes.
- Commit and push cleanly when requested.

## Workflow
1. Confirm current state:
   - Check `git status`.
   - Identify whether issue is code, DB/data, or deployment mismatch.
2. Reproduce or inspect evidence:
   - For build errors: read exact compiler block and fix the first real error.
   - For runtime issues: request and inspect relevant `docker logs server` window.
3. Apply minimal fix:
   - Prefer targeted code edits with clear guards/fallbacks.
   - Avoid broad refactors while stabilizing production behavior.
4. Validate:
   - Run lint diagnostics on touched files.
   - Re-run the relevant build/test command when feasible.
5. Ship:
   - Commit only when user asks.
   - Push only when user asks.

## EVEmu-Specific Triage Rules
- For login black screen:
  - Verify latest commit is actually deployed.
  - Verify character location/session state consistency.
  - Verify bubble/set-state path can complete (no invalid position/null bubble path).
- For lookup/search regressions:
  - Match client argument semantics exactly (group IDs vs bool flags).
  - Query correct map/static tables for region/constellation/system/station lookups.
- For skill queue timing issues:
  - Align SP accumulation cadence with end-time logic.
  - Prefer completing when either end-time passed or SP target reached.
- For DB/schema issues:
  - Use idempotent migrations (`INSERT ... WHERE NOT EXISTS` style where appropriate).
  - Provide deterministic commands for migrate + seed + restart.

## Commit/Push Policy
- Do not commit unless explicitly requested.
- Do not push unless explicitly requested.
- Keep commit messages short and purpose-driven.
- Never use destructive git commands unless explicitly approved.

## Output Style
- Keep responses concise and action-oriented.
- Lead with concrete diagnosis and next command/change.
- If blocked, request exact missing artifact (specific log range or command output).
