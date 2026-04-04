#pragma once
#include <QObject>
#include <QList>
#include "boat.h"
#include "rower.h"
#include "assignment.h"

struct ScoringPriority {
    enum Factor { Skill, Compatibility, Propulsion };
    QList<Factor> order = { Factor::Skill, Factor::Compatibility, Factor::Propulsion };
    bool trainingMode = false;   // if true, skill+compat are ignored entirely
    bool crazyMode    = false;   // if true, everything is ignored — pure random distribution

    double weightFor(Factor f) const {
        int idx = order.indexOf(f);
        if (idx < 0) return 0.0;
        static const double weights[] = { 4.0, 2.0, 1.0 };
        return (idx < 3) ? weights[idx] : 0.0;
    }
    static QString factorName(Factor f) {
        switch (f) {
        case Skill:         return "Skill";
        case Compatibility: return "Compatibility";
        case Propulsion:    return "Propulsion match";
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
                  int maxAttempts = 600) const;
};
