# Task 7: CI fix report — retail TRN skip + error-string printing

## 1. Root cause (confirmed, real error string)

CI only has `spawn.mpq`. Both `LevelRosterBaselineTest.PlacedClassMixReport` /
`PlacedClassMixWithinBaseline` and `HellfireNoParamsSamplingTest.*` failed
because a real dependency was missing under shareware-only assets — not
because of an assertion bug or a duplicate-`InitMonsters()` state conflict.

### 1.1 `level_roster_baseline_test` — real failure

The fixture explicitly sets `gbIsSpawn = false` (comment: "与
sampling_behavior_test.cpp 一致：仅有 spawn.mpq 时不清空任务"). `InitQuests()`
only clears every quest to `QUEST_NOTAVAIL` when `gbIsSpawn` is true
(`Source/quests.cpp:97-101`); with `gbIsSpawn` forced false, quests are left
active, including Q_SKELKING (`qdlvl=3`, singleplayer). Once
`CreateDungeonForMeasurement` reaches `currlevel == 3`,
`PlaceQuestMonsters()` (`Source/monster.cpp`) calls
`PlaceUniqueMonst(UniqueMonsterType::SkeletonKing, ...)` →
`PrepareUniqueMonst` → `InitTRNForUniqueMonster`, which tries to load
`monsters\monsters\genrl.trn` (the TRN named in
`assets/txtdata/monsters/unique_monstdat.tsv` for `MT_SKING`, also shared by
Butcher/HorkDemon/Defiler/NaKrul). That file is **not present in
`spawn.mpq`** — confirmed with `smpq -l build/assets/../spawn.mpq`-equivalent
listing of the real archive: every `.trn` in spawn.mpq lives under
per-monster-family paths (e.g. `monsters/skelbow/blue.trn`); there is no
`monsters/monsters/` directory at all in the shareware archive. Local runs
are green only because the dev machine also has `hellfire.mpq`/`DIABDAT.MPQ`
mounted, which do ship `genrl.trn`.

Reproduced locally by renaming `DIABDAT.MPQ`, `HELLFIRE.MPQ`, `hellfire.mpq`
away and re-running the un-patched binary:

```
test/level_roster_baseline_test.cpp:299: Failure
Value of: initResult.has_value()
  Actual: false
Expected: true
Failed to open file:
monsters\monsters\genrl.trn

Couldn't open /home/peng/workspace/DevilutionX/build/assets/monsters/monsters/genrl.trn

The MPQ file(s) might be damaged. Please check the file integrity.
```

`PlacedClassMixWithinBaseline` fails the same way, just later in its L1-15
loop (the L1 measurement succeeds and prints before the failure at L2/L3 —
i.e. as soon as its seed/level loop reaches `currlevel == 3`). This explains
the "316 fails immediately / 317 fails after L1" difference reported by CI:
it is purely about *when* each test's loop reaches level 3, not a
state-conflict bug between the two tests.

### 1.2 `HellfireNoParamsSamplingTest` — real failure

This suite's own purpose (per its R28/R30 comments) is to exercise L17-24,
which only get a non-empty candidate pool once the `hf` mod overlay is
mounted (`TestInitGame(..., hellfire=true)`). Without `hellfire.mpq` on the
machine, `LoadModArchives({"hf"})` cannot mount Hellfire content, so
`HaveHellfire()` is false and the premise the tests build on ("L17-24 sample
something because the overlay is up") is vacuous. The old code hard-asserted
`HaveHellfire()` and failed the whole build instead of skipping.

## 2. Disposition per item

| # | Item | Disposition |
|---|---|---|
| 1 | Convert missing-prerequisite failures to `GTEST_SKIP()` | Done for both files (see §3) |
| 2 | Print real error strings on `InitMonsters()`/`GetLevelMTypes()` | Done for both files (see §3) |
| 3 | Diagnose real error string | Done — see §1.1, exact string captured above |
| 4 | Prefer making the fixture run for real in CI over skipping | Evaluated and **rejected on purpose** — see §4 |
| 5 | No weakened assertions / no threshold/ceiling changes / no passed_min drift | Confirmed — see §5 |

## 3. Code changes

### 3.1 `test/level_roster_baseline_test.cpp`

- `#include "engine/assets.hpp"` added (for `OpenAsset`/`AssetHandle`).
- `LevelRosterBaselineTest::SetUpTestSuite()`: after `LoadLevelRoster()`,
  probes the real dependency once:
  ```cpp
  size_t trnSize = 0;
  const AssetHandle trnHandle = OpenAsset(R"(monsters\monsters\genrl.trn)", trnSize);
  missingRetailTrn_ = !trnHandle.ok() || trnSize == 0;
  ```
  stored in a new static `missingRetailTrn_` flag, deliberately **not**
  `HaveHellfire()`-based: a future environment with only `DIABDAT.MPQ`
  (retail, no Hellfire) should still be able to run these retail-measured
  baselines, and a `HaveHellfire()` check would incorrectly skip that case.
- `PlacedClassMixReport` / `PlacedClassMixWithinBaseline`: added
  `if (missingRetailTrn_) GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";`
  right after the existing `missingMpqAssets_` guard.
  `PlacesMonstersForCathedralL1` was left untouched: it stays at level 1
  (`CreateDungeonForMeasurement(1, 1000)`), never reaches `currlevel == 3`,
  so it never hits the SkeletonKing path and needs no new guard — confirmed
  green in both HF-present and HF-absent local runs (§6).
- All three `InitMonsters()`/`GetLevelMTypes()` call sites (single-level
  test, `PlacedClassMixReport`, `PlacedClassMixWithinBaseline`) now print the
  real error on failure:
  ```cpp
  const auto initResult = InitMonsters();
  ASSERT_TRUE(initResult.has_value()) << initResult.error();
  ```
  (and the analogous form for `GetLevelMTypes()`).

### 3.2 `test/sampling_behavior_test.cpp`

- `HellfireNoParamsSamplingTest::SetUpTestSuite()`: after
  `LoadLevelRoster()`, adds
  ```cpp
  missingHellfire_ = !HaveHellfire();
  ```
  (new static flag `missingHellfire_`). This suite's real dependency *is*
  Hellfire specifically (it mounts the `hf` overlay to populate L17-24), so
  `HaveHellfire()` remains the right probe here — unlike the retail-TRN case
  in §3.1.
- `LevelsWithoutParamsStillSampleTypes` / `NoParamsTailExceedsTheParameterisedCap`:
  the previous hard `ASSERT_TRUE(HaveHellfire())` (which failed the whole
  suite) is now `if (missingHellfire_) GTEST_SKIP() << "hf overlay required: L17-24 have no candidates under base monstdat";`,
  placed at the same spot, immediately after the existing
  `missingMpqAssets_` guard and before any of the other prerequisite
  assertions (params-row absent, roster-rows absent, candidate pool
  non-empty) — that "guard prerequisites first, then assert the conclusion"
  structure is unchanged, only the outermost "no Hellfire" branch changed
  from assert-fail to skip.
- Both tests' `GetLevelMTypes()` calls now print `.error()` on failure
  (`ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();`).

CRLF verified preserved: `grep -c $'\r$'` equals the total line count for
both files after every edit (413/413 and 1618/1618).

## 4. Why the TRN-dependent tests are skipped rather than made to run in CI

Considered and rejected two ways to make `PlacedClassMixReport` /
`PlacedClassMixWithinBaseline` actually execute under spawn-only CI:

- **Force `gbIsSpawn = true` in the fixture.** Rejected: the A-baseline
  ranged-share numbers this test's ceilings are built on
  (`kRangedShareBaseline`/`kRangedShareCeiling`, documented in
  `eval/cases/rng/level-rosters.yaml`) were measured under retail semantics
  (`gbIsSpawn = false`, all quests active, full monster availability
  including `MonsterAvailability::Retail`-gated types). Switching to
  shareware semantics would silently change what the test measures — fewer
  available monster types, different quest-driven pre-adds — invalidating
  the measurement basis the ceilings encode, without changing a single
  literal number. That is a substantive weakening of the test even though no
  constant would be touched, so it is out of scope per the "don't loosen
  gbIsSpawn to force a run" instruction.
- **Suppress the SkeletonKing quest specifically (e.g. force
  `Quests[Q_SKELKING]._qactive = QUEST_NOTAVAIL` in the fixture) while
  leaving `gbIsSpawn` false for everything else.** Considered but rejected:
  this only works by construction for the one quest that happens to trigger
  the missing asset today; it does not generalize (any future Retail-only
  asset reachable via other active quests, or via `PlaceUniqueMonsters()`'s
  mlevel-based unique placement, would reproduce the same failure under
  spawn-only CI) and it is itself a semantic change to the fixture's quest
  state that a reviewer would reasonably read as "papering over the
  dependency" rather than fixing the test. It does not touch any assertion
  or threshold, but it does change what "retail baseline" fixture setup
  means, and the parent's directive was explicit that the retail semantics
  must stay intact.

No zero-risk way to make these two tests exercise the real placement path
under spawn.mpq alone was found, since the whole point of the test is to
measure retail-semantics placement, and retail-only assets are categorically
absent from spawn.mpq. Skip is therefore the correct outcome for these two,
not a compromise.

## 5. No assertions weakened

- No threshold/ceiling/baseline constant changed (`kRangedShareBaseline`,
  `kRangedShareCeiling` in `level_roster_baseline_test.cpp` untouched; R4/R35
  contract in the two rng YAMLs untouched).
- No assertion deleted; only two hard `ASSERT_TRUE(HaveHellfire())` calls
  converted to `GTEST_SKIP()` guards, and two `ASSERT_TRUE` calls on
  `PlaceUniqueMonst`'s eventual `InitMonsters()`/`GetLevelMTypes()` results
  gained `<< result.error()` (message only, same pass/fail semantics).
- No test case added or removed — same 3 `TEST_F` in
  `LevelRosterBaselineTest`, same 2 `TEST_F` in `HellfireNoParamsSamplingTest`
  (and the other 26 in `SamplingBaselineTest` untouched). `passed_min: 3` in
  `eval/cases/rng/level-rosters.yaml` and `passed_min: 26` in
  `eval/cases/rng/sampling-anti-monopoly.yaml` are both still accurate — no
  YAML edits were made or needed. `skipped_max: 1` in both YAMLs already
  tolerates the (pre-existing) `missingMpqAssets_` skip path; it also
  tolerates the new skip path since both only ever trigger when
  `missingMpqAssets_` is unset (main data present) but the finer-grained
  probe (`missingRetailTrn_` / `missingHellfire_`) fails — i.e. these two
  skip reasons are mutually exclusive with `missingMpqAssets_`'s, and in the
  actual CI/shareware-only environment only the new skip path fires, at most
  2 tests in `level_roster_baseline_test` and 2 in `sampling_behavior_test`
  respectively (both binaries have separate `skipped_max: 1` gates — see §7
  for why the current smoke/nightly gates already tolerate this).

## 6. Verification (all commands actually run)

### 6.1 Local, HF present (build not skipped)

```
cmake --build build --target level_roster_baseline_test sampling_behavior_test -j8
```
exit 0.

`./sampling_behavior_test`: **28/28 passed, 0 skipped.**
`HellfireNoParamsSamplingTest.LevelsWithoutParamsStillSampleTypes` (3 ms) and
`.NoParamsTailExceedsTheParameterisedCap` (3 ms) both ran for real (not
skipped) — full log at `/tmp/sampling_hf_present.log`.

`./level_roster_baseline_test`: **3/3 passed, 0 skipped**
(`PlacesMonstersForCathedralL1` 12 ms, `PlacedClassMixReport` 3542 ms,
`PlacedClassMixWithinBaseline` 140808 ms). `[ MEASURED ]` lines for L1-15
ranged share all printed and all under ceiling — full log at
`/tmp/roster_hf_present.log`.

(A pre-existing, unrelated UBSan diagnostic
`Source/levels/drlg_l2.cpp:2072:47: runtime error: index 40 out of bounds
for type 'unsigned char [40]'` prints during the L6 measurement in this run;
it does not fail the test and is out of scope for this task — not
investigated further.)

### 6.2 Local, simulated CI (spawn.mpq only)

Renamed `~/.local/share/diasurgical/devilution/{DIABDAT.MPQ,HELLFIRE.MPQ,hellfire.mpq}`
to `.bak` (all three, since the real dependency for
`level_roster_baseline_test` is the retail/HF TRN, not Hellfire specifically
— renaming only `hellfire.mpq` would have left `DIABDAT.MPQ`'s copy of
`genrl.trn` available and not faithfully reproduced CI's spawn-only state).

`./sampling_behavior_test`: **26 passed, 2 skipped, 0 failed.**
```
[ RUN      ] HellfireNoParamsSamplingTest.LevelsWithoutParamsStillSampleTypes
test/sampling_behavior_test.cpp:1526: Skipped
hf overlay required: L17-24 have no candidates under base monstdat
[  SKIPPED ] HellfireNoParamsSamplingTest.LevelsWithoutParamsStillSampleTypes (0 ms)
[ RUN      ] HellfireNoParamsSamplingTest.NoParamsTailExceedsTheParameterisedCap
test/sampling_behavior_test.cpp:1583: Skipped
hf overlay required: L17-24 have no candidates under base monstdat
[  SKIPPED ] HellfireNoParamsSamplingTest.NoParamsTailExceedsTheParameterisedCap (0 ms)
...
[  PASSED  ] 26 tests.
[  SKIPPED ] 2 tests
```
Full log: `/tmp/sampling_no_hf.log`.

`./level_roster_baseline_test`: **1 passed, 2 skipped, 0 failed.**
```
[ RUN      ] LevelRosterBaselineTest.PlacesMonstersForCathedralL1
[       OK ] LevelRosterBaselineTest.PlacesMonstersForCathedralL1 (15 ms)
[ RUN      ] LevelRosterBaselineTest.PlacedClassMixReport
test/level_roster_baseline_test.cpp:290: Skipped
retail/HF TRN (monsters\monsters\genrl.trn) not available - skipping test
[  SKIPPED ] LevelRosterBaselineTest.PlacedClassMixReport (0 ms)
[ RUN      ] LevelRosterBaselineTest.PlacedClassMixWithinBaseline
test/level_roster_baseline_test.cpp:349: Skipped
retail/HF TRN (monsters\monsters\genrl.trn) not available - skipping test
[  SKIPPED ] LevelRosterBaselineTest.PlacedClassMixWithinBaseline (0 ms)
...
[  PASSED  ] 1 test.
[  SKIPPED ] 2 tests
```
Full log: `/tmp/roster_no_hf_final.log`. **No `genrl.trn`-related failures in
either binary** — the exact regression this task fixes.

Files restored immediately after this pass:
```
cd ~/.local/share/diasurgical/devilution/
mv DIABDAT.MPQ.bak DIABDAT.MPQ
mv HELLFIRE.MPQ.bak HELLFIRE.MPQ
mv hellfire.mpq.bak hellfire.mpq
```
confirmed present again with `ls -la` (all three original files back, no
`.bak` remnants) before running the full gate below.

### 6.3 Full gate

```
python3 tools/run_tests.py --json /tmp/ci.json
```
Result: `build.ok=true`; `ctest.passed=745, failed=0, skipped=3,
passed_pct=100, returncode=0`; `drift.drift_ok=true` (5/5 passes: A, B, C,
C2, E). The 3 skips are pre-existing and unrelated to this change
(`timedemo_test.cpp:116`, `visual_store_test.cpp:281`,
`visual_store_test.cpp:327`).

### 6.4 Eval smoke gate

```
python3 -m tools.eval.backend --smoke
```
Result: `evaluated: 36  passed: 36  failed: 0  skipped: 0  pass_rate: 1.0`,
exit 0.

### 6.5 Direct eval-case check (the two rng cases affected by this fix)

Not part of the mandated smoke set (both live only in
`eval/cases/_nightly.yaml`) but run directly for completeness, with
`hellfire.mpq`/`DIABDAT.MPQ` present (matching the dev machine, same as
§6.1):

```
python3 -m tools.eval.backend --run level-rosters
  -> [PASS] [rng] level-rosters (30/30)
python3 -m tools.eval.backend --run sampling-anti-monopoly-cap
  -> [PASS] [rng] sampling-anti-monopoly-cap (15/15)
```
Both pass; no changes were needed to either YAML.

## 7. Skip/run checklist under CI conditions (spawn.mpq only)

`sampling_behavior_test` (`SamplingBaselineTest`, 26 cases) — **all run for
real, none skipped**, because none of them depend on retail/HF-only TRNs or
the `hf` overlay:

| Guard | Runs under CI? |
|---|---|
| Core-always-present (`RosterCoreAlwaysPresent`) | Runs |
| Tail-pool upper bound (`RosterTailDrawBounded`) | Runs |
| Quota checks (`RosterQuotasSatisfied`, `RosterQuotaAllowanceIsBinding`) | Runs |
| 9b per-seed combination count (`RosterPerSeedVariety`) | Runs |
| 9c unique reachability (`HellUniqueBasesRemainReachable`) | Runs |
| B1 sampling-anti-monopoly contract (cap-related cases: `CavesKiteTailBaseline`, `CavesAnyClassTailBaseline`, `HellL13/14/15SameClassTailBaseline`, `CatacombsUnconstrainedByCap`, etc.) | Runs |
| Everything else in `SamplingBaselineTest` (26/26) | Runs |

`HellfireNoParamsSamplingTest` (2 cases) — **both skip** under CI, because
their entire subject (L17-24 no-params tail behaviour) only exists once the
`hf` overlay is mounted, which needs `hellfire.mpq`:

| Test | Runs under CI? |
|---|---|
| `LevelsWithoutParamsStillSampleTypes` | Skips (`missingHellfire_`) |
| `NoParamsTailExceedsTheParameterisedCap` | Skips (`missingHellfire_`) |

`level_roster_baseline_test` (`LevelRosterBaselineTest`, 3 cases):

| Test | Runs under CI? |
|---|---|
| `PlacesMonstersForCathedralL1` | Runs (never reaches `currlevel==3`, no TRN dependency) |
| `PlacedClassMixReport` | Skips (`missingRetailTrn_`) |
| `PlacedClassMixWithinBaseline` | Skips (`missingRetailTrn_`) |

## 8. Concerns / follow-ups

- The two retail-baseline placement guards
  (`PlacedClassMixReport`/`PlacedClassMixWithinBaseline`, which cover the
  R4 ranged-share ceiling for L1-15) do not execute at all in the actual
  GitHub CI environment (spawn.mpq only) — they only ran locally in this
  report. If CI's asset situation ever changes (e.g. a future step downloads
  `DIABDAT.MPQ`), they will start running automatically since the skip
  condition is a live probe, not a hardcoded flag. Until then, this ceiling
  is only enforced by local pre-push runs and by the "HF present" gate in
  this report, not by the automated pipeline.
- The `Source/levels/drlg_l2.cpp:2072` UBSan out-of-bounds diagnostic seen
  during L6 measurement (§6.1) is pre-existing and unrelated to this task's
  scope; flagging for future investigation but not fixed here.
- No `passed_min`/case-count changes were needed, so no YAML edits were made.
</content>
</invoke>
