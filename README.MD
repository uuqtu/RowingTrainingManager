# Rowing Manager

A Qt6/C++ desktop application for managing rowing club session assignments.
Boats are filled automatically while respecting skill levels, compatibility,
propulsion ability, blacklists, groups, and equipment availability.

---

## Part 1 — User Guide

### Getting started

Open the application. The main window has seven tabs: **Boats**, **Rowers**,
**Assignments**, **Distance**, **Dist. Detail**, **Statistics**, and **Options**.
Work through them from left to right the first time you set up a club.

---

### Boats tab

Add every boat the club owns with **＋ Add Boat** or **Ctrl+N**.

| Column | What to enter |
|---|---|
| Name | Free text, e.g. "DF" |
| Type | Gig or Racing |
| Cap | Number of rowing seats (1–5). Does **not** count the foot-steerer. |
| Steering | **Foot-Steered** = a dedicated foot-steerer sits in the boat. **Hand-Steered** = rowers steer themselves, no steerer needed. |
| Propulsion | **Scull** = two oars per rower. **Sweep** = one oar per rower. |

Default for new boats: Foot-Steered, Scull.

---

### Rowers tab

Add every club member with **＋ Add Rower** or **Ctrl+N**.

| Column | What to enter |
|---|---|
| Name | Full name |
| Skill | Student → Beginner → Experienced → Professional |
| Compat | See compatibility section below |
| Foot Steerer | ✓ if this person can operate foot-steering |
| Obmann | ✓ if this person is eligible to lead a boat |
| Propulsion | Scull / Sweep / Both |
| Age Band | Decade range (20-30, 30-40, …). Used for role weighting. |
| Strength | 0 (not set) to 10. Used to balance boat strength. |
| Attr 1/2/3 | Custom numeric attributes 0–10 (technique, fitness, etc.) |

**Compatibility tiers:**
- **Infinite** — rows happily with anyone, never penalised
- **Normal** — standard; no penalty with anyone
- **Special** — mild preference to stay with Normal/Infinite; soft penalty with other Specials, hard block with Selected
- **Selected** — hard block with Special in strict mode; fine with everyone else

**Whitelist / Blacklist** — click the buttons in the toolbar.
A whitelist gives a bonus score when those people end up in the same boat.
A blacklist is a hard constraint: those two people will never be in the same boat.

---

### Options tab

**Equipment limits** — enter the number of oars physically available.
Set 0 for no limit. These limits are enforced globally: if your selected boats
would need more scull oars than are available, the assignment dialog blocks
generation and shows the shortfall.

**Sick / Unavailable** — check the box next to any rower who is absent today.
Sick rowers are excluded from all generation and selection lists until unchecked.
They still appear in Statistics and Distance history.

---

### Creating an assignment

Click **🎲 New** on the Assignments tab. A dialog opens with four tabs.

**Tab 1 — Groups**
Create named groups of people who must share a boat. Optionally pin the group
to a specific boat. If you pin a group to a boat, the group label shows
`(members/capacity)` so you can see how many seats remain for the generator to fill.
You cannot add more members to a group than its pinned boat can hold.

**Tab 2 — Boats & Rowers**
Check which boats and rowers to include. Enable **"Select boats automatically"**
to let the application pick boats that exactly match the number of checked rowers,
respecting equipment limits.

**Tab 3 — Priority**
Drag the three factors (Skill, Compatibility, Propulsion) into the order that
matters most today. The top item is weighted 4×, second 2×, third 1×.

- **Training mode** — ignores Skill and Compatibility entirely; only Propulsion,
  Whitelist, and Attribute closeness are used.
- **Crazy mode** — ignores everything. Pure random shuffle. Fun at parties.

**Tab 4 — Options**
Add **Steering-only people**: riders who sit in a boat but do not occupy a rowing
seat. They are not counted against capacity. You can pin them to a specific boat.

Click **Check** to see any issues before generating.
Click **Generate**. If it fails, a full diagnostic appears in the preview panel
explaining exactly why.
Click **Accept & Save** to store the result.

---

### Distance tab

Select an assignment from the dropdown. Enter the kilometres rowed by any one
person in a boat — the application auto-fills the same value for all other rowers
**in the same boat** (not the whole assignment, since different boats may cover
different distances). You can then adjust individual values freely.

---

### Statistics tab

Shows lifetime totals and "last 3 sessions" counts for Obmann duty, Steering duty,
and total kilometres rowed. Rowers with 3 or more appearances in either role in
the last three sessions are flagged as "overused" — the generator automatically
reduces their weighting for those roles next time.

---

### Printing

Select an assignment and click **Print**. The application auto-detects a connected
Epson receipt printer via USB. The small **x** spinbox to the left defaults to the
number of boats in the assignment (one slip per boat crew). You can reduce it.
If no printer is found, a dialog explains the reason.

---

---

## Part 2 — Developer & Expert Reference

### Architecture overview

```
main.cpp
└── MainWindow  (mainwindow.h/.cpp)
    ├── BoatTableModel   (boattablemodel.h/.cpp)
    ├── RowerTableModel  (rowertablemodel.h/.cpp)
    ├── DatabaseManager  (databasemanager.h/.cpp)
    ├── AssignmentDialog (assignmentdialog.h/.cpp)
    │   └── AssignmentGenerator (assignmentgenerator.h/.cpp)
    └── PrinterDevice    (printerdevice.h/.cpp)
```

Data classes: `Boat`, `Rower`, `Assignment` (with `SavedGroup`).
All persistence is SQLite via Qt's `QSqlDatabase`. The DB file is created
in the application's working directory as `rowing_manager.db`.

---

### Data model

#### Boat

```cpp
enum class BoatType    { Gig, Racing };
enum class SteeringType{ Steered,    // Foot-Steered — needs a canSteer person
                         NonSteered  // Hand-Steered — no steerer needed
                       };
enum class PropulsionType { Skull,   // stored as "Scull" in display
                            Riemen,  // stored as "Sweep" in display
                            Both };
```

`steeringTypeFromString()` accepts both old names (`"Steered"`, `"Non-Steered"`)
and new names (`"Foot-Steered"`, `"Hand-Steered"`) for DB backward compatibility.
`propulsionTypeFromString()` likewise accepts `"Skull"/"Scull"` and `"Riemen"/"Sweep"`.

Default new boat: `SteeringType::Steered`, `PropulsionType::Skull`.

#### Rower

Fields relevant to generation logic:

| Field | Type | Generation role |
|---|---|---|
| `skill` | `SkillLevel` (0–3) | Scoring, Racing-boat hard constraint |
| `compatibility` | `CompatibilityTier` (0–3) | Hard constraint & soft penalty |
| `canSteer` | bool | Required for Foot-Steered boats |
| `isObmann` | bool | Preferred for Obmann role (not enforced as hard constraint) |
| `propulsionAbility` | `PropulsionType` | Hard constraint |
| `ageBand` | int (lower decade) | Obmann / Steerer role weighting |
| `strength` | int 0–10 | Strength-variance balancing score |
| `attr1/2/3` | int 0–10 | Attribute-variance balancing score |
| `whitelist` | `QList<int>` rowerId | Bonus +5.0 per matched pair in same boat |
| `blacklist` | `QList<int>` rowerId | Hard bidirectional constraint |

Skill integer mapping: Student=1, Beginner=2, Experienced=3, Professional=4.

#### Assignment

`boatRowerMap`: `QMap<int boatId, QList<int> rowerIds>` — the core result.
`groups`: `QList<SavedGroup>` — persisted dialog group state for double-click restore.
`checkedBoatIds / checkedRowerIds` — which items were checked in Tab 2 at generation time.
`priorityOrder` — serialised priority list including `"__training__"` or `"__crazy__"` tokens.

---

### Database schema

```sql
boats           (id, name, type, capacity, steering_type, propulsion_type)
rowers          (id, name, skill, compatibility, can_steer, is_obmann,
                 propulsion_ability, attr1, attr2, attr3, age_band, strength,
                 whitelist, blacklist)
assignments     (id, name, created_at)
assignment_entries (id, assignment_id, boat_id, rower_id)
assignment_details (assignment_id, details_json)  -- full dialog state for restore
assignment_roles   (id, assignment_id, boat_id, rower_id, role)
                   -- role: "obmann" | "steering" | "obmann_steering"
rower_distances    (id, assignment_id, rower_id, km)
sick_rowers        (rower_id)
```

All new columns are added via `ALTER TABLE ADD COLUMN` (idempotent, errors ignored)
so existing databases upgrade silently. String parsers always accept both old and
new display names.

---

### Generation algorithm

#### Entry point: `AssignmentGenerator::generate()`

1. Validates that `selectedRowers.size() >= totalCapacity`. If not, returns immediately with an error.

2. **Crazy mode** short-circuit: Fisher-Yates shuffle of all rower IDs, then
   fill boats in declaration order by capacity. No constraints checked. Returns success immediately.

3. **Normal mode — three-pass graceful degradation:**

   | Pass | `relaxCompat` | Effect |
   |---|---|---|
   | 0 | false | All constraints active including compat hard-block and Racing/Beginner hard-block |
   | 1 | true | Compatibility hard-block and Racing/Beginner constraint lifted; blacklist and propulsion still active |
   | 2 | true (relaxAll) | Same as pass 1; steerer requirement also skipped if no steerer available |

   Each pass runs up to **15 full-assignment attempts**. Each attempt calls
   `fillBoat()` for every boat in declaration order. If all boats fill successfully,
   the attempt is recorded as success and the pass ends. The 15-attempt loop exists
   because `fillBoat` is stochastic (random shuffles) — multiple attempts improve
   the chance of finding a feasible + high-scoring solution.

#### `fillBoat()` — inner loop

For a single boat with `capacity` seats:

1. Pre-check: if `availableRowerIds.size() < capacity`, return false immediately.

2. If the boat is **Foot-Steered**, confirm at least one available rower has
   `canSteer == true`. If not, return false immediately (unless pass 2, where this
   is skipped via `relaxAll`).

3. Run **600 shuffle attempts** (`maxAttempts = 600`):
   - Fisher-Yates shuffle the available rower ID list.
   - On even-numbered attempts, pre-seed the candidate list with a randomly chosen
     `canSteer` rower (for Foot-Steered boats). This biases half the attempts toward
     guaranteeing a steerer is included early.
   - Greedily add rowers from the shuffled list as long as `!violatesConstraints()`.
   - If `candidate.size() < capacity` after the full list is exhausted, discard and retry.
   - If Foot-Steered and no steerer ended up in the candidate, discard and retry.
   - Score the candidate via `scoreTeam()`. Keep the highest-scoring valid team seen
     across all 600 attempts.

4. Commit the best team: remove those IDs from `availableRowerIds`, write to `outTeam`.

#### `violatesConstraints()` — hard rules

Applied per-rower being considered for addition to a growing team:

| Rule | Always active | Notes |
|---|---|---|
| **Blacklist** | Yes | Bidirectional: rejected if *either* party has the other on their blacklist |
| **Propulsion** | Yes | Rower's `propulsionAbility` must match boat's `propulsionType` (unless boat is `Both`) |
| **Compatibility hard-block** | Pass 0 only | `Special` + `Selected` pair → reject |
| **Racing/Beginner block** | Pass 0 only | `Student` or `Beginner` skill → reject for `Racing` boats |

#### `scoreTeam()` — scoring formula

All terms are summed; higher = better. The generator keeps the highest-scoring
team seen across all 600 attempts.

**Normal mode:**

```
score = wSkill  × (avgSkill + skillBalance)
      + wCompat × (-compatPenaltyTotal)
      + wProp   × avgProp × 3.0
      - attrVariance × 0.5
      - strengthVariance × 0.3
      + whitelistBonus
      - racingBeginnerPenalty
```

**Training mode** (skill and compat weights ignored):

```
score = -attrVariance
      - strengthVariance × 0.3
      + whitelistBonus
      + avgProp × 3.0
      - racingBeginnerPenalty
```

**Term definitions:**

| Term | How computed |
|---|---|
| `avgSkill` | Mean of `skillToInt()` across team members (Student=1 … Professional=4) |
| `skillBalance` | `-variance(skillVals) / N` — negative variance, so less spread = less penalty |
| `wSkill / wCompat / wProp` | `4.0 / 2.0 / 1.0` for rank 1/2/3 in the priority list |
| `compatPenaltyTotal` | Sum of pairwise `compatPenalty(a, b)` for all pairs in the team |
| `avgProp` | Mean per-rower propulsion match score: 1.0 (exact), 0.5 (Both), 0.0 (mismatch) |
| `attrVariance` | Sum of variance of attr1, attr2, attr3 independently; 0-valued attrs excluded |
| `strengthVariance` | Variance of `strength` values across team; strength=0 excluded; only for capacity > 2 |
| `whitelistBonus` | +5.0 per whitelist pair that ends up in the same team |
| `racingBeginnerPenalty` | +8.0 per Student/Beginner rower in a Racing boat (applied in all passes) |

**Compatibility penalty table** (`compatPenalty(a, b)`):

| Pair | Penalty |
|---|---|
| Either is Infinite | 0.0 |
| Either is Normal | 0.0 |
| Special + Special | 2.0 |
| Special + Selected | 4.0 (also a hard block in pass 0) |
| Selected + Selected | 0.0 |

The `wCompat × (-compatPenaltyTotal)` term means a higher penalty reduces the
score — the generator prefers teams with lower total compatibility tension.

---

### Role assignment (Obmann / Steering)

Roles are computed **once** on first display of an assignment and persisted
to the `assignment_roles` table. Subsequent single-clicks or double-clicks
read the saved roles and never re-randomise.

Roles are only assigned for boats with `capacity > 2`. Boats with 1 or 2 seats
skip role designation entirely.

**Obmann scoring** (higher = preferred):

```
obmannScore(rid) = ageBand × 0.5 - recentObmannCount × 3.0
```
- Returns −1e9 if `isObmann == false` (unless no qualified candidates exist,
  in which case anyone is picked as a last resort).
- `recentObmannCount` = number of times this rower was Obmann in the last 3 assignments
  (from `assignment_roles` via `loadStats()`).

**Steerer scoring** (higher = preferred, for Foot-Steered boats only):

```
steerScore(rid) = (100 - effectiveBand) × 0.3 - recentSteeringCount × 3.0
```
- Returns −1e9 if `canSteer == false`.
- `effectiveBand` = `ageBand` if set, else 50 (treated as mid-range unknown).
- Younger = higher score (smaller band = larger `100 - band`).

A single person can hold both Obmann and Steerer roles simultaneously. In that case
the DB stores `"obmann_steering"` and the display shows `[Obmann] [Steering]` on one line.

---

### Check logic (`runChecks()`)

Runs when the user presses **Check** and also at the start of **Generate**
(capacity mismatch blocks generation; all others are warnings).

Order of checks:
1. Group capacity overflow (pinned groups with more members than the boat holds) — **hard block in Generate**
2. Equipment limits: `neededScullOars > maxScullOars` or `neededSweepOars > maxSweepOars` — **hard block in Generate and disables Generate button**
3. Seat count: `rowers != totalCapacity` — **hard block in Generate**
4. Obmann availability: 0 qualified Obmann rowers for boats needing one — **warning only**, not a block
5. Foot-steerer availability: fewer `canSteer` rowers than Foot-Steered boats with capacity > 2 — **warning**, not a block (generator will try relaxed passes)
6. Propulsion: per-boat check for enough capable rowers — **warning**
7. Racing/Beginner: more Racing seats than Experienced/Professional rowers — **warning**

---

### Auto-select boats algorithm

When "Select boats automatically" is checked in Tab 2, `autoSelectBoats()` runs
whenever the rower selection changes:

1. Compute `needed = selectedRowerCount - pinnedGroupCapacity`.
2. Uncheck all free (non-pinned) boats.
3. **Pass 0** — select boats where `boat.capacity() <= remaining` (no overshoot).
   Within each pass, respect scull/sweep equipment limits per-rower.
4. **Pass 1** — if `needed` is still unmet after pass 0, allow boats that would
   overshoot (best-effort fill).

---

### Diagnostic system (`diagnose()`)

Called when generation fails. Produces a plain-text report covering:

- Seat balance (free rowers vs free seats)
- Equipment limits and how much is consumed
- Groups: which rowers are consumed vs which are free
- Steering-only people
- Per-boat analysis:
  - Foot-steerer requirement and which free rowers qualify
  - If no steerer is free, which steerers are locked in groups with actionable fix hint
  - Per-rower eligibility with specific rejection reasons (propulsion, skill, blacklist, compat)
  - CRITICAL notice if eligible rower count < seat count
- Whitelist chains (including "NOT in selection" warnings)
- Blacklist conflicts within the free-rower pool

---

### Sick rower flow

1. User checks rower in **Options → Sick / Unavailable**.
2. `m_db->setSickRower(rowerId, true)` writes to `sick_rowers` table.
3. `m_sickRowerIds` list is updated in memory.
4. On **New Assignment** or **Edit Assignment**, the healthy rower list is built as:
   ```cpp
   for (const Rower& r : m_rowers)
       if (!m_sickRowerIds.contains(r.id())) healthyRowers << r;
   ```
   Only `healthyRowers` is passed to `AssignmentDialog`. Sick rowers never appear
   in the generator's selection lists.

---

### Distance pre-fill rule

In `onDistanceChanged()`, when a km value is entered for rower R in assignment A:

- All other rowers in the **same boat** within the same assignment whose current km is 0 are filled with the same value.
- Rowers in *other* boats within the same assignment are **not** touched (different boats may have rowed different distances).
- Rowers already having a non-zero km value are never overwritten.

Pre-fill key: `Qt::UserRole + 2` stores the `boatId` on each distance table row.

---

### Printer integration

`PrinterDevice` uses `libusb-1.0` for direct USB bulk transfer to Epson ESC/POS printers.

**Connection sequence:**
1. Try each entry in `KNOWN_PRINTERS[]` (Epson VID `0x04B8`, various PIDs) via `libusb_open_device_with_vid_pid`.
2. If none match, scan all USB devices for known vendor IDs and then check for interface class `0x07` (printer), `0xFF` (vendor-specific), or `0x00` (device-level class).
3. For the matched device, call `libusb_claim_interface()` on the first interface that exposes a bulk-OUT endpoint. On Linux, detach the kernel driver first if active.
4. Send ESC/POS init (`ESC @`) to reset the printer.

**Print flow:**
- Umlaut substitution (ä→ae, ö→oe, etc.) before encoding to Latin-1.
- Append 4 trailing newlines for paper feed.
- Send `GS V 0` (full cut) at the end.
- `onPrintAssignment()` loops `copies` times, calling `printText()` once per copy.

**Copies default:** `m_printCopiesSpinBox` defaults to `boatRowerMap().size()` (number of boats in the assignment) whenever an assignment is selected.

---

### Build system

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
```

Nix shell: `pkgs.libusb1` in `buildInputs` provides both the shared library and
`libusb-1.0/libusb.h`. The `pkg-config` wrapper finds it automatically.

On Linux, USB printer access requires either:
```
# /etc/udev/rules.d/99-epson-printer.rules
SUBSYSTEM=="usb", ATTRS{idVendor}=="04b8", MODE="0666"
```
...followed by `udevadm control --reload && udevadm trigger`, or running the application as root.
