#pragma once
#include <QObject>
#include <QList>
#include <QMap>
#include <QPair>
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

struct GeneratorResult {
    bool success = false;
    QString errorMessage;
    Assignment assignment;
};

class AssignmentGenerator : public QObject {
    Q_OBJECT
public:
    explicit AssignmentGenerator(QObject* parent = nullptr);

    GeneratorResult generate(
        const QList<Boat>& selectedBoats,
        const QList<Rower>& selectedRowers,
        const QString& assignmentName,
        const ScoringPriority& priority = {}
    );

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
                             bool relaxCompat) const;
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
