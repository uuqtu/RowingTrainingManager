#pragma once
#include <QString>
#include <QList>
#include "boat.h"

enum class SkillLevel { Student = 0, Beginner, Experienced, Professional };
enum class CompatibilityTier { Infinite = 0, Normal, Special, Selected };

// Age band lower bounds: 0=unknown, 20, 30, 40, 50, 60, 70, 80
static const QList<int> kAgeBands = {0, 20, 30, 40, 50, 60, 70, 80};

class Rower {
public:
    Rower();
    Rower(int id, const QString& name, SkillLevel skill, CompatibilityTier compat);

    int     id()   const { return m_id; }
    void    setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void    setName(const QString& n) { m_name = n; }

    SkillLevel skill() const { return m_skill; }
    void setSkill(SkillLevel s) { m_skill = s; }

    CompatibilityTier compatibility() const { return m_compatibility; }
    void setCompatibility(CompatibilityTier c) { m_compatibility = c; }

    // Age band (lower bound of decade, e.g. 30 = "30-40", 0 = unknown)
    int ageBand() const { return m_ageBand; }
    void setAgeBand(int b) { m_ageBand = b; }
    // Returns "20-30", "30-40" etc., or "" if unknown
    static QString ageBandToString(int b);
    static QStringList ageBandOptions();   // for combo delegate

    bool canSteer()  const { return m_canSteer; }
    void setCanSteer(bool v) { m_canSteer = v; }

    bool isObmann()  const { return m_isObmann; }
    void setIsObmann(bool v) { m_isObmann = v; }

    PropulsionType propulsionAbility() const { return m_propulsionAbility; }
    void setPropulsionAbility(PropulsionType p) { m_propulsionAbility = p; }

    int attr1() const { return m_attr1; }  void setAttr1(int v) { m_attr1 = qBound(0, v, 10); }
    int attr2() const { return m_attr2; }  void setAttr2(int v) { m_attr2 = qBound(0, v, 10); }
    int attr3() const { return m_attr3; }  void setAttr3(int v) { m_attr3 = qBound(0, v, 10); }
    // Strength (0=not set, 1-10). Balanced across teams when boat capacity > 2.
    int strength() const { return m_strength; }  void setStrength(int v) { m_strength = qBound(0, v, 10); }

    QList<int> whitelist() const { return m_whitelist; }
    void setWhitelist(const QList<int>& wl) { m_whitelist = wl; }
    void addToWhitelist(int id);
    void removeFromWhitelist(int id);

    QList<int> blacklist() const { return m_blacklist; }
    void setBlacklist(const QList<int>& bl) { m_blacklist = bl; }
    void addToBlacklist(int id);
    void removeFromBlacklist(int id);

    bool canRowPropulsion(PropulsionType boatPropulsion) const;

    static QString listToString(const QList<int>& list);
    static QList<int> listFromString(const QString& s);

    static QString skillToString(SkillLevel s);
    static SkillLevel skillFromString(const QString& s);
    static int skillToInt(SkillLevel s);

    static QString compatToString(CompatibilityTier c);
    static CompatibilityTier compatFromString(const QString& s);

    static double compatPenalty(CompatibilityTier a, CompatibilityTier b);

private:
    int  m_id   = -1;
    QString m_name;
    SkillLevel        m_skill         = SkillLevel::Beginner;
    CompatibilityTier m_compatibility = CompatibilityTier::Normal;
    int            m_ageBand          = 0;
    bool           m_canSteer         = false;
    bool           m_isObmann         = false;
    PropulsionType m_propulsionAbility = PropulsionType::Both;
    int  m_attr1 = 0, m_attr2 = 0, m_attr3 = 0, m_strength = 0;
    QList<int> m_whitelist;
    QList<int> m_blacklist;
};
