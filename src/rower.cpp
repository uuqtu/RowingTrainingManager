#include "rower.h"
#include <QStringList>

Rower::Rower() : m_whitelist({}), m_blacklist({}) {}

Rower::Rower(int id, const QString& name, SkillLevel skill, CompatibilityTier compat)
    : m_id(id), m_name(name), m_skill(skill), m_compatibility(compat) {}

void Rower::addToWhitelist(int id) { if (!m_whitelist.contains(id)) m_whitelist.append(id); }
void Rower::removeFromWhitelist(int id) { m_whitelist.removeAll(id); }
void Rower::addToBlacklist(int id)  { if (!m_blacklist.contains(id))  m_blacklist.append(id); }
void Rower::removeFromBlacklist(int id)  { m_blacklist.removeAll(id); }

bool Rower::canRowPropulsion(PropulsionType boatPropulsion) const {
    if (boatPropulsion == PropulsionType::Both) return true;
    if (m_propulsionAbility == PropulsionType::Both) return true;
    return m_propulsionAbility == boatPropulsion;
}

// ── Skill ────────────────────────────────────────────────────────
QString Rower::skillToString(SkillLevel s) {
    switch (s) {
    case SkillLevel::Student:      return "Student";
    case SkillLevel::Beginner:     return "Beginner";
    case SkillLevel::Experienced:  return "Experienced";
    case SkillLevel::Professional: return "Professional";
    }
    return "Beginner";
}
SkillLevel Rower::skillFromString(const QString& s) {
    if (s == "Student")      return SkillLevel::Student;
    if (s == "Experienced")  return SkillLevel::Experienced;
    if (s == "Professional") return SkillLevel::Professional;
    return SkillLevel::Beginner;
}
int Rower::skillToInt(SkillLevel s) {
    return static_cast<int>(s) + 1;  // Student=1 … Professional=4
}

// ── Compatibility ────────────────────────────────────────────────
QString Rower::compatToString(CompatibilityTier c) {
    switch (c) {
    case CompatibilityTier::Infinite:  return "Infinite";
    case CompatibilityTier::Normal:    return "Normal";
    case CompatibilityTier::Special:   return "Special";
    case CompatibilityTier::Selected:  return "Selected";
    }
    return "Normal";
}
CompatibilityTier Rower::compatFromString(const QString& s) {
    if (s == "Infinite") return CompatibilityTier::Infinite;
    if (s == "Special")  return CompatibilityTier::Special;
    if (s == "Selected") return CompatibilityTier::Selected;
    return CompatibilityTier::Normal;
}

// Soft compatibility penalty between two rowers (0 = ideal, higher = worse).
// The generator uses this in scoring; it does NOT hard-reject combinations
// unless a retry with relaxed constraints also fails.
double Rower::compatPenalty(CompatibilityTier a, CompatibilityTier b) {
    using CT = CompatibilityTier;
    // Infinite and Normal are always fine with everyone
    if (a == CT::Infinite || b == CT::Infinite) return 0.0;
    if (a == CT::Normal   || b == CT::Normal)   return 0.0;
    // Special + Special: small penalty (prefer not, but acceptable)
    if (a == CT::Special  && b == CT::Special)  return 2.0;
    // Special + Selected: larger penalty
    if ((a == CT::Special && b == CT::Selected) ||
        (a == CT::Selected && b == CT::Special)) return 4.0;
    // Selected + Selected: fine
    return 0.0;
}

// ── Lists ────────────────────────────────────────────────────────
QString Rower::listToString(const QList<int>& list) {
    if (list.isEmpty()) return QStringLiteral("");
    QStringList parts;
    for (int id : list) parts << QString::number(id);
    return parts.join(",");
}
QList<int> Rower::listFromString(const QString& s) {
    QList<int> result;
    if (s.trimmed().isEmpty()) return result;
    for (const QString& part : s.split(",", Qt::SkipEmptyParts)) {
        bool ok; int id = part.trimmed().toInt(&ok);
        if (ok) result.append(id);
    }
    return result;
}

QString Rower::ageBandToString(int b) {
    if (b <= 0) return "";
    return QString("%1-%2").arg(b).arg(b + 10);
}
QStringList Rower::ageBandOptions() {
    return {"(unknown)", "20-30", "30-40", "40-50", "50-60", "60-70", "70-80", "80-90"};
}

