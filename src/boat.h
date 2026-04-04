#pragma once
#include <QString>

enum class BoatType { Gig, Racing };
enum class SteeringType { Steered, NonSteered };

// Boats: strictly Scull or Sweep.
// Rowers: Scull, Sweep, or Both (can do either style).
enum class PropulsionType { Skull, Riemen, Both };

class Boat {
public:
    Boat();
    Boat(int id, const QString& name, BoatType type, int capacity,
         SteeringType steering, PropulsionType propulsion);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    BoatType boatType() const { return m_boatType; }
    void setBoatType(BoatType t) { m_boatType = t; }

    int capacity() const { return m_capacity; }
    void setCapacity(int c) { m_capacity = c; }

    SteeringType steeringType() const { return m_steeringType; }
    void setSteeringType(SteeringType s) { m_steeringType = s; }

    PropulsionType propulsionType() const { return m_propulsionType; }
    void setPropulsionType(PropulsionType p) { m_propulsionType = p; }

    // String helpers
    static QString boatTypeToString(BoatType t);
    static BoatType boatTypeFromString(const QString& s);
    static QString steeringTypeToString(SteeringType s);
    static SteeringType steeringTypeFromString(const QString& s);
    static QString propulsionTypeToString(PropulsionType p);
    static PropulsionType propulsionTypeFromString(const QString& s);

private:
    int m_id = -1;
    QString m_name;
    BoatType m_boatType = BoatType::Gig;
    int m_capacity = 1;
    SteeringType m_steeringType = SteeringType::Steered;
    PropulsionType m_propulsionType = PropulsionType::Skull;
};
