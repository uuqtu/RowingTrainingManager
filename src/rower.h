#pragma once
#include <QString>
#include <QList>
#include "boat.h"

enum class SkillLevel { Student = 0, Beginner, Experienced, Professional };
enum class CompatibilityTier { Infinite = 0, Normal, Special, Selected };

// StrokeLength: 0=unknown, 1=Short, 2=Medium, 3=Long
// BodySize:     0=unknown, 1=Small, 2=Medium, 3=Tall
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

    int ageBand() const { return m_ageBand; }
    void setAgeBand(int b) { m_ageBand = b; }
    static QString ageBandToString(int b);
    static QStringList ageBandOptions();

    bool canSteer()  const { return m_canSteer; }
    void setCanSteer(bool v) { m_canSteer = v; }

    bool isObmann()  const { return m_isObmann; }
    void setIsObmann(bool v) { m_isObmann = v; }

    PropulsionType propulsionAbility() const { return m_propulsionAbility; }
    void setPropulsionAbility(PropulsionType p) { m_propulsionAbility = p; }

    // Stroke length: 0=unknown, 1=Short, 2=Medium, 3=Long
    int strokeLength() const { return m_strokeLength; }
    void setStrokeLength(int v) { m_strokeLength = qBound(0, v, 3); }
    static QStringList strokeLengthOptions() { return {"(unknown)", "Short", "Medium", "Long"}; }
    static QString strokeLengthToString(int v) {
        switch(v) { case 1: return "Short"; case 2: return "Medium"; case 3: return "Long"; }
        return "";
    }

    // Body size: 0=unknown, 1=Small, 2=Medium, 3=Tall
    int bodySize() const { return m_bodySize; }
    void setBodySize(int v) { m_bodySize = qBound(0, v, 3); }
    static QStringList bodySizeOptions() { return {"(unknown)", "Small", "Medium", "Tall"}; }
    static QString bodySizeToString(int v) {
        switch(v) { case 1: return "Small"; case 2: return "Medium"; case 3: return "Tall"; }
        return "";
    }

    // Strength (0=not set, 1-10)
    int strength() const { return m_strength; }
    void setStrength(int v) { m_strength = qBound(0, v, 10); }

    // Group attrs: 0=not set, 1-10. People with same value are preferred together.
    int attrGrp1() const { return m_attrGrp1; }  void setAttrGrp1(int v) { m_attrGrp1 = qBound(0, v, 10); }
    int attrGrp2() const { return m_attrGrp2; }  void setAttrGrp2(int v) { m_attrGrp2 = qBound(0, v, 10); }

    // Balance attrs: 0=not set, 1-10. Average across boats is balanced.
    int attrVal1() const { return m_attrVal1; }  void setAttrVal1(int v) { m_attrVal1 = qBound(0, v, 10); }
    int attrVal2() const { return m_attrVal2; }  void setAttrVal2(int v) { m_attrVal2 = qBound(0, v, 10); }

    // Rower whitelist / blacklist (other rower IDs)
    QList<int> whitelist() const { return m_whitelist; }
    void setWhitelist(const QList<int>& wl) { m_whitelist = wl; }
    void addToWhitelist(int id);
    void removeFromWhitelist(int id);

    QList<int> blacklist() const { return m_blacklist; }
    void setBlacklist(const QList<int>& bl) { m_blacklist = bl; }
    void addToBlacklist(int id);
    void removeFromBlacklist(int id);

    // Boat whitelist / blacklist (boat IDs)
    QList<int> boatWhitelist() const { return m_boatWhitelist; }
    void setBoatWhitelist(const QList<int>& wl) { m_boatWhitelist = wl; }

    QList<int> boatBlacklist() const { return m_boatBlacklist; }
    void setBoatBlacklist(const QList<int>& bl) { m_boatBlacklist = bl; }

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

    int  m_strokeLength = 0;  // 1=Short 2=Medium 3=Long
    int  m_bodySize     = 0;  // 1=Small 2=Medium 3=Tall
    int  m_strength     = 0;
    int  m_attrGrp1     = 0;
    int  m_attrGrp2     = 0;
    int  m_attrVal1     = 0;
    int  m_attrVal2     = 0;

    QList<int> m_whitelist;
    QList<int> m_blacklist;
    QList<int> m_boatWhitelist;
    QList<int> m_boatBlacklist;
};
