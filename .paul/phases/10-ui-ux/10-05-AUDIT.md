# Enterprise Plan Audit Report

**Plan:** .paul/phases/10-ui-ux/10-05-PLAN.md
**Audited:** 2026-06-17
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. Plan is structurally sound and consistent with Phase 10 patterns (timer gating, UiCommand routing, RAII attachment ordering). Two gaps prevent safe implementation: a compile-breaking include omission (M1) and a misleading comment that will cause future developers to misunderstand the APVTS contract (SR1). The mute/solo state drift is an accepted v1 limitation but was undocumented (SR2). All three addressed; plan is now APPLY-ready.

Would I sign this? After applied upgrades — yes.

---

## 2. What Is Solid

- **Timer gate pattern (visibilityChanged + stopTimer in dtor)** — mirrors 10-03 M2/M3 exactly. MixScreen destructor stops timer before member destruction; no timer-fires-on-dead-object risk.
- **SliderAttachment destruction order** — `attachments_` declared after `knobs_` in FxScreen; `master_attachment_` declared after `master_fader_` in MixScreen. Reverse destruction (RAII) correctly destroys attachments before their sliders. No dangling-reference-in-dtor hazard.
- **UiCommand routing for all lane mutations** — all per-lane writes (gain, pan, mute, solo) push through `push_ui_command()`. No direct mutation of audio-thread-owned PerfState. Consistent with Phase 10-01 single-writer contract.
- **APVTS SliderAttachment for master_gain** — correct; master_gain is APVTS-backed. Attachment handles audio-thread sync automatically; no timer needed for master.
- **Explicit `#include <algorithm>` / `<cmath>`** — M1 pattern from 10-04 preemptively applied. Good.
- **Meter decay pattern** — lane_meter_ decays naturally to 0 when stopped (velocities→0). No explicit is-timer-running check needed. Clean.
- **APVTS param IDs match plugin_processor.cpp** — all 6 FX param IDs (filter_cutoff, filter_res, reverb_mix, delay_time, delay_mix, sidechain_threshold) and master_gain verified against `create_parameter_layout()`. No ID drift.
- **dontSendNotification in constructor** — lane_faders/pans setValue called with dontSendNotification before onValueChange is set. No spurious command push at construction time.
- **`autonomous: false`** — human verify checkpoint correctly present given visual layout complexity.

---

## 3. Enterprise Gaps Identified

### Gap 1: Missing `#include "plugin_processor.h"` in fx_screen.cpp and mix_screen.cpp [COMPILE-BLOCKING]

Both headers forward-declare `BaqueProcessor`. Forward declarations satisfy pointer/reference declarations but do NOT allow method invocation. The .cpp files call `proc.getAPVTS()` (fx_screen.cpp) and `proc_.push_ui_command()`, `proc_.ui_snapshot()`, `proc.getAPVTS()` (mix_screen.cpp). Without the full header, all these calls fail to compile.

The plan lists `#include <algorithm>` and `#include <cmath>` explicitly (applying the M1 pattern from prior audits) but omits the most important include. This is silent and will produce cryptic "member of forward-declared class" errors on all three platforms.

Pattern precedent: `perform_screen.h` includes `../plugin_processor.h` directly. The 10-05 plan uses forward-declaration in headers (better compile times), but the .cpp files MUST complete the type.

### Gap 2: Incorrect rationale — "apvts_ is currently private with no accessor" [MISLEADING DOCUMENTATION]

`apvts_` is declared PUBLIC in plugin_processor.h at line 64. The getter `getAPVTS()` is additive (named accessor, message-thread semantics), not a capability unlock. The stated reason will mislead future readers into thinking this added new access, potentially prompting incorrect changes (e.g., removing the getter thinking it's redundant, or accessing apvts_ directly from audio thread assuming it was never protected).

### Gap 3: mute_state_[]/solo_state_[] state drift after preset load [UNDOCUMENTED KNOWN LIMITATION]

`mute_state_[]` and `solo_state_[]` are message-thread-local bool arrays. After `setStateInformation` (preset load), the engine's PerfState mute/solo is restored, but MixScreen's arrays stay at false. Buttons will show incorrect state until the user clicks them.

Root cause: PerfState mute/solo is not exposed in UiStateSnapshot. This is a known v1 gap (no write path from engine→UI for toggle state). Acceptable in v1 only if documented explicitly; without documentation, every future developer will investigate this as a bug.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | `#include "plugin_processor.h"` missing in fx_screen.cpp — BaqueProcessor forward-declared in header; .cpp calls getAPVTS() which requires full definition. Compile-blocking on all 3 platforms. | Task 1, fx_screen.cpp implementation bullet | Added `#include "fx_screen.h"` then `#include "plugin_processor.h"` as first include step with audit comment explaining why |
| 1 | Same issue in mix_screen.cpp — calls push_ui_command(), ui_snapshot(), getAPVTS(). | Task 2, mix_screen.cpp implementation bullet | Added `#include "mix_screen.h"` then `#include "plugin_processor.h"` as first include step with audit comment |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Task 1 comment says "apvts_ is currently private with no accessor" — factually wrong. apvts_ is public at plugin_processor.h:64. Misleads future readers about what the getter adds. | Task 1, getAPVTS() bullet | Replaced rationale: getter is a named message-thread-scoped accessor for clarity, matching Phase 10 accessor pattern; does NOT unlock new access |
| 2 | mute_state_[]/solo_state_[] diverge from engine PerfState after preset load — undocumented known limitation. Will look like a bug. | Task 2, MixScreen header declaration block + Boundaries | Added class-level comment to mix_screen.h specifying the limitation and deferral. Added SCOPE LIMIT entry explicitly re-deferring APVTS mute/solo to 10-07 (10-01 had deferred "→ 10-05" but UiCommand is correct for live control) |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | APVTS mute/solo DAW automation (deferred from 10-01 audit "→ 10-05") — not addressed in this plan | UiCommand is the correct live-UI pattern. APVTS mute/solo params are needed only for DAW automation recording, which belongs in the Phase 10 DoD plan (10-07) alongside full automation story |
| 2 | `master_fader_.setValue(0.8, dontSendNotification)` is redundant — SliderAttachment overwrites value on construction from APVTS state | Not harmful; dontSendNotification prevents spurious command push; SliderAttachment will correct to APVTS value anyway. Self-correcting |
| 3 | 16-strip visual layout tightness — ~70px per lane strip at 1200px with pan/fader/M/S — may feel cramped | Human verify checkpoint (Task 4) is blocking and covers this; functional correctness not at risk |

---

## 5. Audit & Compliance Readiness

**Evidence produced:** 3 tests (FX1, MIX1, MIX2) — paint smoke + timer gate. Sufficient for build-path verification. Human verify checkpoint is blocking before UNIFY.

**Silent failures prevented:** M1 (compile error caught before APPLY). SR2 (state drift documented — won't be silently treated as bug in Phase 11/12).

**Post-incident reconstruction:** UiCommand queue is the sole mutation path for per-lane state. All lane changes are traceable to push_ui_command() calls. Master gain is APVTS-backed (DAW automation log). Mute/solo is UiCommand-only in v1 — no automation trail (known, documented in SR2).

**Ownership:** single-writer contract maintained. Audio thread owns PerfState; UI writes via queue. No new single-writer violations introduced.

---

## 6. Final Release Bar

**Must be true before this plan ships:**
- Both .cpp files include `plugin_processor.h` (M1)
- 234 tests pass on all 3 platforms (no CI gate skipped)
- Human verify checkpoint approved (visual layout correct)

**Risks remaining if shipped:**
- mute/solo UI state diverges after preset load (documented, acceptable in v1)
- No DAW automation for mute/solo (re-deferred to 10-07 — acceptable)
- 16-strip layout may feel cramped at minimum window size (800px) — human verify must test at minimum size

**Sign-off:** Conditionally acceptable → upgraded. Post-audit PLAN.md ready for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
