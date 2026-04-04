#include "assignmentgenerator.h"
#include <QRandomGenerator>
#include <QMap>
#include <cmath>

AssignmentGenerator::AssignmentGenerator(QObject* parent) : QObject(parent) {}

bool AssignmentGenerator::teamHasSteerer(const QList<int>& team,
                                          const QList<Rower>& allRowers) const {
    for (int id : team)
        for (const Rower& r : allRowers)
            if (r.id() == id && r.canSteer()) return true;
    return false;
}

// ---------------------------------------------------------------
bool AssignmentGenerator::violatesConstraints(const Rower& rower,
                                               const QList<int>& team,
                                               const Boat& boat,
                                               const QList<Rower>& allRowers,
                                               bool relaxCompat) const {
    // 1. Blacklist (bidirectional)
    for (int blackId : rower.blacklist())
        if (team.contains(blackId)) return true;
    for (int memberId : team)
        for (const Rower& r : allRowers)
            if (r.id() == memberId && r.blacklist().contains(rower.id()))
                return true;

    // 2. Propulsion
    if (boat.propulsionType() != PropulsionType::Both)
        if (!rower.canRowPropulsion(boat.propulsionType()))
            return true;

    // 3. Compatibility (soft, checked only when not relaxed)
    if (!relaxCompat) {
        using CT = CompatibilityTier;
        CT rc = rower.compatibility();
        for (int memberId : team) {
            for (const Rower& r : allRowers) {
                if (r.id() != memberId) continue;
                CT mc = r.compatibility();
                bool hard = ((rc == CT::Special && mc == CT::Selected) ||
                             (rc == CT::Selected && mc == CT::Special));
                if (hard) return true;
                break;
            }
        }

        // 4. Skill / boat-type: Students and Beginners should not go in Racing boats.
        //    Treated as a hard constraint in the strict pass; relaxed in later passes.
        if (boat.boatType() == BoatType::Racing) {
            if (rower.skill() == SkillLevel::Student ||
                rower.skill() == SkillLevel::Beginner)
                return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------
double AssignmentGenerator::scoreTeam(const QList<int>& team,
                                       const Boat& boat,
                                       const QList<Rower>& allRowers,
                                       const ScoringPriority& priority) const {
    if (team.isEmpty()) return 0.0;

    // Collect rower data
    QList<int> skillVals;
    double compatPenaltyTotal = 0.0;
    double propScore = 0.0;
    double whitelistBonus = 0.0;
    double attrVariance = 0.0;

    QList<const Rower*> members;
    for (int id : team)
        for (const Rower& r : allRowers)
            if (r.id() == id) { members << &r; break; }

    for (const Rower* r : members) {
        skillVals << Rower::skillToInt(r->skill());
        for (int wId : r->whitelist())
            if (team.contains(wId)) whitelistBonus += 5.0;

        if (boat.propulsionType() != PropulsionType::Both) {
            if (r->propulsionAbility() == boat.propulsionType()) propScore += 1.0;
            else if (r->propulsionAbility() == PropulsionType::Both)  propScore += 0.5;
        } else { propScore += 1.0; }
    }

    // Pairwise compat penalty
    for (int i = 0; i < members.size(); ++i)
        for (int j = i + 1; j < members.size(); ++j)
            compatPenaltyTotal += Rower::compatPenalty(members[i]->compatibility(),
                                                        members[j]->compatibility());

    // Skill balance: average and variance
    double avgSkill = 0;
    for (int v : skillVals) avgSkill += v;
    avgSkill /= skillVals.size();
    double var = 0;
    for (int v : skillVals) var += (v - avgSkill) * (v - avgSkill);
    double skillBalance = -var / skillVals.size();   // higher = more balanced

    // Attribute matching: minimise variance across attr1/2/3 (0 = not set, skip)
    for (int attrIdx = 0; attrIdx < 3; ++attrIdx) {
        QList<int> vals;
        for (const Rower* r : members) {
            int v = (attrIdx == 0) ? r->attr1() : (attrIdx == 1) ? r->attr2() : r->attr3();
            if (v > 0) vals << v;
        }
        if (vals.size() < 2) continue;
        double avg = 0; for (int v : vals) avg += v; avg /= vals.size();
        double atv = 0; for (int v : vals) atv += (v - avg)*(v - avg);
        attrVariance += atv / vals.size();
    }

    double avgProp = propScore / members.size();

    // Strength balancing: for boats with capacity > 2, penalise variance in strength.
    // Strength 0 means "not set" and is excluded from calculation.
    double strengthVariance = 0.0;
    if (boat.capacity() > 2) {
        QList<int> svals;
        for (const Rower* r : members)
            if (r->strength() > 0) svals << r->strength();
        if (svals.size() >= 2) {
            double avg = 0; for (int v : svals) avg += v; avg /= svals.size();
            double sv = 0; for (int v : svals) sv += (v-avg)*(v-avg);
            strengthVariance = sv / svals.size();
        }
    }

    // Soft penalty for Student/Beginner rowers in Racing boats.
    // Applied in all modes (including relaxed passes) so the generator
    // still prefers to avoid the combination even when hard constraints are off.
    double racingBeginnerPenalty = 0.0;
    if (boat.boatType() == BoatType::Racing) {
        for (const Rower* r : members) {
            if (r->skill() == SkillLevel::Student || r->skill() == SkillLevel::Beginner)
                racingBeginnerPenalty += 8.0;   // large enough to strongly discourage
        }
    }

    if (priority.trainingMode) {
        return -attrVariance - strengthVariance * 0.3 + whitelistBonus + avgProp * 3.0 - racingBeginnerPenalty;
    }

    double wSkill  = priority.weightFor(ScoringPriority::Skill);
    double wCompat = priority.weightFor(ScoringPriority::Compatibility);
    double wProp   = priority.weightFor(ScoringPriority::Propulsion);

    return wSkill  * (avgSkill + skillBalance)
         + wCompat * (-compatPenaltyTotal)
         + wProp   * avgProp * 3.0
         - attrVariance * 0.5
         - strengthVariance * 0.3
         + whitelistBonus
         - racingBeginnerPenalty;
}

// ---------------------------------------------------------------
bool AssignmentGenerator::fillBoat(const Boat& boat,
                                    QList<int>& availableRowerIds,
                                    const QList<Rower>& allRowers,
                                    const ScoringPriority& priority,
                                    bool relaxCompat,
                                    QList<int>& outTeam,
                                    int maxAttempts) const {
    int capacity = boat.capacity();
    if (availableRowerIds.size() < capacity) return false;

    // Foot-Steered boats need a dedicated steerer; Hand-Steered do not
    bool needsSteerer = (boat.steeringType() == SteeringType::Steered);
    if (needsSteerer) {
        bool has = false;
        for (int id : availableRowerIds)
            for (const Rower& r : allRowers)
                if (r.id() == id && r.canSteer()) { has = true; break; }
        if (!has) return false;
    }

    QMap<int, const Rower*> rowerMap;
    for (const Rower& r : allRowers) rowerMap[r.id()] = &r;

    QList<int> bestTeam;
    double bestScore = -1e18;
    bool found = false;

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        QList<int> shuffled = availableRowerIds;
        for (int i = shuffled.size() - 1; i > 0; --i) {
            int j = QRandomGenerator::global()->bounded(i + 1);
            shuffled.swapItemsAt(i, j);
        }

        QList<int> candidate;
        // Pre-seed steerer on even attempts
        if (needsSteerer && (attempt % 2 == 0)) {
            QList<int> steerers;
            for (int id : shuffled)
                if (rowerMap.contains(id) && rowerMap[id]->canSteer())
                    steerers.append(id);
            if (!steerers.isEmpty()) {
                int pick = steerers[QRandomGenerator::global()->bounded(steerers.size())];
                if (!violatesConstraints(*rowerMap[pick], candidate, boat, allRowers, relaxCompat))
                    candidate.append(pick);
            }
        }

        for (int id : shuffled) {
            if (candidate.size() >= capacity) break;
            if (candidate.contains(id) || !rowerMap.contains(id)) continue;
            if (!violatesConstraints(*rowerMap[id], candidate, boat, allRowers, relaxCompat))
                candidate.append(id);
        }

        if (candidate.size() < capacity) continue;
        if (needsSteerer && !teamHasSteerer(candidate, allRowers)) continue;

        double score = scoreTeam(candidate, boat, allRowers, priority);
        if (!found || score > bestScore) {
            bestScore = score; bestTeam = candidate; found = true;
        }
    }

    if (!found) return false;
    outTeam = bestTeam;
    for (int id : bestTeam) availableRowerIds.removeAll(id);
    return true;
}

// ---------------------------------------------------------------
GeneratorResult AssignmentGenerator::generate(
    const QList<Boat>& selectedBoats,
    const QList<Rower>& selectedRowers,
    const QString& assignmentName,
    const ScoringPriority& priority)
{
    GeneratorResult result;
    int totalCapacity = 0;
    for (const Boat& b : selectedBoats) totalCapacity += b.capacity();
    if (selectedRowers.size() < totalCapacity) {
        result.errorMessage = QString("Not enough rowers: need %1, have %2.")
                                  .arg(totalCapacity).arg(selectedRowers.size());
        return result;
    }

    // ── Crazy mode: pure random shuffle, zero constraints ─────────
    if (priority.crazyMode) {
        QList<int> pool;
        for (const Rower& r : selectedRowers) pool.append(r.id());
        // Shuffle
        for (int i = pool.size() - 1; i > 0; --i) {
            int j = QRandomGenerator::global()->bounded(i + 1);
            pool.swapItemsAt(i, j);
        }
        Assignment a;
        a.setName(assignmentName);
        a.setCreatedAt(QDateTime::currentDateTime());
        int idx = 0;
        for (const Boat& boat : selectedBoats)
            for (int s = 0; s < boat.capacity() && idx < pool.size(); ++s, ++idx)
                a.assignRowerToBoat(boat.id(), pool[idx]);
        result.success    = true;
        result.assignment = a;
        result.errorMessage = "Note: crazy mode — completely random, no constraints applied.";
        return result;
    }

    QList<int> available;
    for (const Rower& r : selectedRowers) available.append(r.id());

    // Three passes: strict compat → relaxed compat → fully relaxed
    for (int pass = 0; pass < 3; ++pass) {
        bool relaxCompat = (pass >= 1);
        bool relaxAll    = (pass == 2);
        bool anySuccess  = false;
        Assignment bestAssignment;

        for (int fa = 0; fa < 15; ++fa) {
            QList<int> avail = available;
            Assignment attempt;
            attempt.setName(assignmentName);
            attempt.setCreatedAt(QDateTime::currentDateTime());
            bool ok = true;

            for (const Boat& boat : selectedBoats) {
                QList<int> team;
                // On full-relax pass, also skip steerer requirement if no steerer found
                bool useRelax = relaxCompat || relaxAll;
                if (!fillBoat(boat, avail, selectedRowers, priority, useRelax, team)) {
                    ok = false; break;
                }
                for (int rid : team) attempt.assignRowerToBoat(boat.id(), rid);
            }
            if (ok) { anySuccess = true; bestAssignment = attempt; break; }
        }

        if (anySuccess) {
            result.success    = true;
            result.assignment = bestAssignment;
            if (pass == 1)
                result.errorMessage = "Note: some compatibility constraints were relaxed.";
            else if (pass == 2)
                result.errorMessage = "Note: all compatibility constraints were ignored — no valid strict assignment found.";
            return result;
        }
    }

    result.errorMessage =
        "Could not generate a valid assignment.\n"
        "Possible causes:\n"
        "• Not enough rowers with the required propulsion ability\n"
        "• No steerer available for a non-steered boat\n"
        "• Blacklist constraints make assignment impossible";
    return result;
}

// ---------------------------------------------------------------
QString AssignmentGenerator::diagnose(
    const QList<Boat>& freeBoats,
    const QList<Rower>& freeRowers,
    const ScoringPriority& priority,
    const DiagContext& ctx) const
{
    // Use "all" lists if provided by caller, otherwise fall back to free lists
    const QList<Rower>& allRowers = ctx.allSelectedRowers.isEmpty() ? freeRowers : ctx.allSelectedRowers;
    const QList<Boat>&  allBoats  = ctx.allSelectedBoats.isEmpty()  ? freeBoats  : ctx.allSelectedBoats;

    QString out;
    out += "=== GENERATION DIAGNOSTIC ===\n\n";

    // ── Overall configuration ────────────────────────────────────
    int totalCap = 0;
    for (const Boat& b : allBoats) totalCap += b.capacity();
    int freeCap = 0;
    for (const Boat& b : freeBoats) freeCap += b.capacity();

    out += QString("All boats:      %1  (total seats: %2)\n").arg(allBoats.size()).arg(totalCap);
    out += QString("All rowers:     %1\n").arg(allRowers.size());
    out += QString("Free boats:     %1  (seats to fill by generator: %2)\n")
               .arg(freeBoats.size()).arg(freeCap);
    out += QString("Free rowers:    %1  (available to generator)\n").arg(freeRowers.size());

    if (freeRowers.size() != freeCap)
        out += QString("!! SEAT MISMATCH: generator has %1 rower(s) for %2 seat(s)\n")
                   .arg(freeRowers.size()).arg(freeCap);
    else
        out += "Seat balance:   OK (free rowers == free seats)\n";

    out += "\n";

    // ── Mode flags ───────────────────────────────────────────────
    if (priority.trainingMode) out += "[Training mode — skill/compat scoring disabled]\n";
    if (priority.crazyMode)    out += "[Crazy mode — all constraints disabled]\n";
    out += "\n";

    // ── Equipment limits ─────────────────────────────────────────
    if (ctx.maxSkullPairs > 0 || ctx.maxRiemenPairs > 0) {
        out += "--- Equipment limits ---\n";
        if (ctx.maxSkullPairs  > 0) out += QString("  Skull pairs available:  %1\n").arg(ctx.maxSkullPairs);
        if (ctx.maxRiemenPairs > 0) out += QString("  Riemen pairs available: %1\n").arg(ctx.maxRiemenPairs);
        // Tally used
        int usedS = 0, usedR = 0;
        for (const Boat& b : allBoats) {
            if (b.propulsionType() == PropulsionType::Skull)  usedS += b.capacity() / 2;
            if (b.propulsionType() == PropulsionType::Riemen) usedR += b.capacity() / 2;
        }
        if (ctx.maxSkullPairs  > 0) out += QString("  Skull pairs used by selected boats:  %1\n").arg(usedS);
        if (ctx.maxRiemenPairs > 0) out += QString("  Riemen pairs used by selected boats: %1\n").arg(usedR);
        out += "\n";
    }

    // ── Groups ───────────────────────────────────────────────────
    if (!ctx.groupSummaries.isEmpty()) {
        out += "--- Groups (rowers consumed by groups are NOT available to free boats) ---\n";
        for (const QString& s : ctx.groupSummaries)
            out += "  " + s + "\n";
        out += "\n";

        // List which rowers are in groups vs free
        QStringList groupRowerNames, freeRowerNames;
        for (const Rower& r : allRowers) {
            bool inFree = false;
            for (const Rower& fr : freeRowers) if (fr.id() == r.id()) { inFree = true; break; }
            if (inFree) freeRowerNames << r.name();
            else groupRowerNames << r.name();
        }
        if (!groupRowerNames.isEmpty())
            out += QString("  In groups (not available to generator): %1\n")
                       .arg(groupRowerNames.join(", "));
        out += QString("  Available to generator:                  %1\n")
                   .arg(freeRowerNames.isEmpty() ? "(none)" : freeRowerNames.join(", "));
        out += "\n";
    }

    // ── Steering-only people ─────────────────────────────────────
    if (!ctx.steeringOnlySummaries.isEmpty()) {
        out += "--- Steering-only people (do not occupy seats) ---\n";
        for (const QString& s : ctx.steeringOnlySummaries)
            out += "  " + s + "\n";
        out += "\n";
    }

    // ── Per-boat analysis (free boats only — what the generator handles) ──
    out += "--- Free boats (handled by generator) ---\n\n";
    for (const Boat& boat : freeBoats) {
        out += QString("  Boat: %1 [%2 | Cap:%3 | %4 | %5]\n")
                   .arg(boat.name())
                   .arg(Boat::boatTypeToString(boat.boatType()))
                   .arg(boat.capacity())
                   .arg(Boat::steeringTypeToString(boat.steeringType()))
                   .arg(Boat::propulsionTypeToString(boat.propulsionType()));

        // Foot-Steered = needs a foot-steerer person; Hand-Steered = no steerer needed
        bool needsSteerer = (boat.steeringType() == SteeringType::Steered);
        if (needsSteerer) {
            QStringList qualified;
            for (const Rower& r : freeRowers)
                if (r.canSteer()) qualified << r.name();
            if (qualified.isEmpty()) {
                out += "    !! BLOCKER: Foot-Steered boat requires a foot-steerer,\n";
                out += "       but NONE of the free rowers have [Foot Steerer] ability.\n";
                // Check if steerers exist but are locked in groups
                QStringList lockedSteerers;
                for (const Rower& r : allRowers) {
                    if (!r.canSteer()) continue;
                    bool isFree = false;
                    for (const Rower& fr : freeRowers) if (fr.id() == r.id()) { isFree = true; break; }
                    if (!isFree) lockedSteerers << r.name();
                }
                if (!lockedSteerers.isEmpty())
                    out += QString("       Foot-steerers locked in groups: %1\n"
                                   "       FIX: Remove them from their group or pin the group\n"
                                   "       to a different boat so they become free.\n")
                               .arg(lockedSteerers.join(", "));
                else
                    out += "       No steerers in entire selection — add a rower with [Steerer] ability.\n";
            } else {
                out += QString("    Steerer required — free rowers who qualify: %1\n")
                           .arg(qualified.join(", "));
            }
        }

        if (boat.boatType() == BoatType::Racing && !priority.trainingMode && !priority.crazyMode) {
            QStringList beginners;
            for (const Rower& r : freeRowers)
                if (r.skill() == SkillLevel::Student || r.skill() == SkillLevel::Beginner)
                    beginners << QString("%1(%2)").arg(r.name()).arg(Rower::skillToString(r.skill()));
            if (!beginners.isEmpty())
                out += QString("    Racing boat: %1 will be excluded in pass 0 "
                               "(Student/Beginner). They may be placed in pass 2/3.\n")
                           .arg(beginners.join(", "));
        }

        out += "    Free-rower eligibility:\n";
        int eligible = 0;
        for (const Rower& r : freeRowers) {
            QStringList issues;
            if (boat.propulsionType() != PropulsionType::Both && !r.canRowPropulsion(boat.propulsionType()))
                issues << QString("propulsion: rower=%1 boat=%2")
                              .arg(Boat::propulsionTypeToString(r.propulsionAbility()))
                              .arg(Boat::propulsionTypeToString(boat.propulsionType()));
            if (!priority.trainingMode && !priority.crazyMode &&
                boat.boatType() == BoatType::Racing &&
                (r.skill() == SkillLevel::Student || r.skill() == SkillLevel::Beginner))
                issues << QString("skill=%1 (needs Experienced/Professional for Racing)")
                              .arg(Rower::skillToString(r.skill()));
            QStringList blNames;
            for (int bid : r.blacklist())
                for (const Rower& o : freeRowers)
                    if (o.id() == bid) blNames << o.name();
            if (!blNames.isEmpty()) issues << "blacklisted with: " + blNames.join(", ");

            if (!priority.trainingMode && !priority.crazyMode) {
                using CT = CompatibilityTier;
                for (const Rower& o : freeRowers) {
                    if (o.id() == r.id()) continue;
                    bool hard = ((r.compatibility() == CT::Special && o.compatibility() == CT::Selected) ||
                                 (r.compatibility() == CT::Selected && o.compatibility() == CT::Special));
                    if (hard) issues << QString("compat-conflict with %1 (%2 vs %3)")
                                            .arg(o.name())
                                            .arg(Rower::compatToString(r.compatibility()))
                                            .arg(Rower::compatToString(o.compatibility()));
                }
            }

            bool propOk = !(boat.propulsionType() != PropulsionType::Both && !r.canRowPropulsion(boat.propulsionType()));
            bool skillOk = priority.trainingMode || priority.crazyMode ||
                           !(boat.boatType() == BoatType::Racing &&
                             (r.skill() == SkillLevel::Student || r.skill() == SkillLevel::Beginner));
            if (propOk && skillOk) eligible++;

            if (issues.isEmpty())
                out += QString("      OK  %1  [Sk:%2 Co:%3 Steer:%4]\n")
                           .arg(r.name())
                           .arg(Rower::skillToString(r.skill()))
                           .arg(Rower::compatToString(r.compatibility()))
                           .arg(r.canSteer() ? "yes" : "no");
            else
                out += QString("      !!  %1 — %2\n").arg(r.name()).arg(issues.join(" | "));
        }

        if (eligible < boat.capacity())
            out += QString("    RESULT: Only %1/%2 free rowers eligible in strict pass.\n"
                           "            Generator will try relaxed passes (2+3).\n")
                       .arg(eligible).arg(boat.capacity());
        else if (needsSteerer) {
            QStringList steerers;
            for (const Rower& r : freeRowers) if (r.canSteer()) steerers << r.name();
            if (steerers.isEmpty())
                out += "    RESULT: IMPOSSIBLE — eligible rowers exist but none can steer.\n";
            else
                out += QString("    RESULT: %1 eligible, steerers available — should work.\n")
                           .arg(eligible);
        } else {
            out += QString("    RESULT: %1 eligible rower(s) for %2 seat(s) — should work.\n")
                       .arg(eligible).arg(boat.capacity());
        }
        out += "\n";
    }

    // ── Whitelist & Blacklist among free rowers ───────────────────
    bool hasWL = false, hasBL = false;
    for (const Rower& r : freeRowers) {
        if (!r.whitelist().isEmpty()) hasWL = true;
        if (!r.blacklist().isEmpty()) hasBL = true;
    }
    if (hasWL) {
        out += "--- Whitelist (free rowers) ---\n";
        for (const Rower& r : freeRowers) {
            if (r.whitelist().isEmpty()) continue;
            QStringList partners;
            for (int wid : r.whitelist()) {
                bool found = false;
                for (const Rower& o : freeRowers) if (o.id() == wid) { found = true; partners << o.name(); break; }
                if (!found) {
                    // check if locked in group
                    for (const Rower& o : allRowers) if (o.id() == wid) { partners << o.name() + " (in group!)"; break; }
                }
            }
            out += QString("  %1 must row WITH: %2\n").arg(r.name()).arg(partners.join(", "));
        }
        out += "\n";
    }
    if (hasBL) {
        out += "--- Blacklist conflicts among free rowers ---\n";
        bool any = false;
        for (const Rower& r : freeRowers)
            for (int bid : r.blacklist())
                for (const Rower& o : freeRowers)
                    if (o.id() == bid) { out += QString("  %1 must NEVER be with: %2\n").arg(r.name()).arg(o.name()); any = true; }
        if (!any) out += "  (no conflicts within free rowers)\n";
        out += "\n";
    }

    out += "=== END DIAGNOSTIC ===\n";
    return out;
}