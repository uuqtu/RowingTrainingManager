#pragma once
#include <QObject>
#include <QList>
#include <QMap>
#include <QPair>
#include <functional>
#include <vector>
#include "boat.h"
#include "rower.h"
#include "assignment.h"

struct ScoringPriority {
    enum Factor { Skill, Compatibility, Propulsion, StrokeLength, BodySize };
    QList<Factor> order = { Factor::Skill, Factor::Compatibility, Factor::Propulsion };
    bool trainingMode = false;
    bool crazyMode    = false;
    // Co-occurrence: pair(minId,maxId) -> count of times they shared a boat
    QMap<QPair<int,int>, int> coOccurrence;

    // Expert-adjustable scoring constants (defaults match hardcoded originals)
    double whitelistBonus         = 5.0;
    double coOccurrenceFactor     = 1.5;
    double obmannBonus            = 20.0;
    double racingBeginnerPenalty  = 8.0;
    double strengthVarianceWeight = 0.3;
    double compatSpecialSpecial   = 2.0;
    double compatSpecialSelected  = 4.0;
    double strokeSmallGap1        = 3.0;
    double strokeSmallGap2        = 12.0;
    double strokeLargePerGap      = 2.5;
    double bodySmallGap1          = 1.5;
    double bodySmallGap2          = 8.0;
    double bodyLargePerGap        = 1.0;
    double grpAttrBonus           = 3.0;
    double valAttrVarianceWeight  = 0.4;
    // Generator search depth
    int    fillBoatAttempts       = 600;
    int    passAttempts           = 15;
    // Maximise learning: reward mixed skill levels within each boat
    bool   maximizeLearning       = false;
    // Ignore list constraints (show warnings instead of hard-blocking)
    bool   ignoreBlacklist        = false;
    bool   ignoreBoatBlacklist    = false;
    bool   ignoreBoatWhitelist    = false;
    int    racingMinSkill         = 4;  // minimum skillToInt() for Racing boats (Intermediate=4)

    double weightFor(Factor f) const {
        int idx = order.indexOf(f);
        if (idx < 0) return 0.0;
        if (idx < (int)rankWeights.size()) return rankWeights[idx];
        static const double fallback[] = { 4.0, 2.0, 1.0, 0.5, 0.5 };
        return (idx < 5) ? fallback[idx] : 0.0;
    }
    std::vector<double> rankWeights = { 4.0, 2.0, 1.0, 0.5, 0.5 };
    static QString factorName(Factor f) {
        switch (f) {
        case Skill:         return "Skill";
        case Compatibility: return "Compatibility";
        case Propulsion:    return "Propulsion match";
        case StrokeLength:  return "Stroke Length";
        case BodySize:      return "Body Size";
        }
        return {};
    }
};

// All intermediate scoring values for one team, produced by scoreTeamDetailed().
struct ScoreDetail {
    // ── Boat-level values ───────────────────────────────────────────────
    int    boatId          = -1;
    double totalScore      = 0.0;
    double avgSkill        = 0.0;   // mean of skillToInt() across team
    double skillBalance    = 0.0;   // −variance/N; 0 = perfectly balanced
    double compatPenalty   = 0.0;   // sum of pairwise compat penalties (before wCompat)
    double avgProp         = 0.0;   // mean propulsion match score [0..1]
    double strengthVariance= 0.0;   // variance of strength values (cap>2)
    double racingBegPenalty= 0.0;   // soft penalty for beginners in Racing
    double obmannBonus     = 0.0;   // flat bonus if ≥1 Obmann in team
    double strokePenalty   = 0.0;   // pairwise stroke-length penalty
    double bodyPenalty     = 0.0;   // pairwise body-size penalty
    double grpBonus        = 0.0;   // group-attr matching bonus
    double valPenalty      = 0.0;   // val-attr variance penalty
    double coOccurrencePenalty = 0.0; // co-occurrence history penalty
    double wSkill          = 0.0;   // applied weight for Skill
    double wCompat         = 0.0;   // applied weight for Compatibility
    double wProp           = 0.0;   // applied weight for Propulsion
    bool   trainingMode    = false;
    bool   crazyMode       = false;

    // ── Per-rower values ─────────────────────────────────────────────────
    struct RowerDetail {
        int    rowerId     = -1;
        int    skillInt    = 0;     // 1=Student .. 4=Professional
        double propScore   = 0.0;   // 1.0/0.5/0.0 for this rower
        int    strength    = 0;     // 0=not set
        int    strokeLength= 0;     // 0=unknown
        int    bodySize    = 0;     // 0=unknown
        int    attrGrp1    = 0;
        int    attrGrp2    = 0;
        int    attrVal1    = 0;
        int    attrVal2    = 0;
        double whitelistContrib = 0.0; // bonus this rower contributes (his direction)
        double coOccContrib     = 0.0; // penalty from co-occurrence pairs involving this rower
        QString compatTier;         // "Infinite"/"Normal"/"Special"/"Selected"
        bool   isObmann    = false;
        bool   canSteer    = false;
    };
    QList<RowerDetail> rowers;
};

struct GeneratorResult {
    bool success = false;
    QString errorMessage;
    Assignment assignment;
};

class AssignmentGenerator : public QObject {
    Q_OBJECT
public:
    explicit AssignmentGenerator(QObject* parent = nullptr);

    // Set the directory where solver logs are written (created if absent).
    // Call before generate(). If empty, no log is written.
    void setLogDir(const QString& dir) { m_logDir = dir; }

    // Optional progress callback: called with 0..100 during generate().
    // Use a std::function so it can be set from any thread context.
    void setProgressCallback(std::function<void(int)> cb) { m_progressCb = cb; }

    GeneratorResult generate(
        const QList<Boat>& selectedBoats,
        const QList<Rower>& selectedRowers,
        const QString& assignmentName,
        const ScoringPriority& priority = {}
    );

    // Compute full scoring detail for a committed team (called after generation).
private:
    QString m_logDir;
    std::function<void(int)> m_progressCb;  // optional 0..100 progress callback
public:
    QList<ScoreDetail> computeScoreDetails(
        const Assignment& assignment,
        const QList<Boat>& boats,
        const QList<Rower>& allRowers,
        const ScoringPriority& priority
    ) const;

    // Context for the diagnostic — filled in by the caller (onGenerate)
    struct DiagContext {
        // All rowers (including those claimed by groups) for full picture
        QList<Rower> allSelectedRowers;
        QList<Boat>  allSelectedBoats;
        // Groups as text summaries: "GroupName: Rower1, Rower2 -> BoatName"
        QStringList  groupSummaries;
        // Steering-only summaries: "Name -> BoatName (or any)"
        QStringList  steeringOnlySummaries;
        // Equipment limits (0 = unlimited)
        int maxSkullPairs  = 0;
        int maxRiemenPairs = 0;
    };

    // Returns a detailed human-readable diagnostic explaining why generation
    // failed (or what constraints are active). Called after a failed generate().
    QString diagnose(
        const QList<Boat>& freeBoats,
        const QList<Rower>& freeRowers,
        const ScoringPriority& priority,
        const DiagContext& ctx
    ) const;

private:
    bool violatesConstraints(const Rower& rower, const QList<int>& team,
                             const Boat& boat, const QList<Rower>& allRowers,
                             bool relaxCompat,
                             const ScoringPriority* priority = nullptr) const;
    bool teamHasSteerer(const QList<int>& team, const QList<Rower>& allRowers) const;
    double scoreTeam(const QList<int>& team, const Boat& boat,
                     const QList<Rower>& allRowers,
                     const ScoringPriority& priority) const;
    bool fillBoat(const Boat& boat,
                  QList<int>& availableRowerIds,
                  const QList<Rower>& allRowers,
                  const ScoringPriority& priority,
                  bool relaxCompat,
                  QList<int>& outTeam,
                  int maxAttempts = -1) const;  // -1 = use priority.fillBoatAttempts
};
