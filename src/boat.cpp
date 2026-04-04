#include "boat.h"

Boat::Boat() {}

Boat::Boat(int id, const QString& name, BoatType type, int capacity,
           SteeringType steering, PropulsionType propulsion)
    : m_id(id), m_name(name), m_boatType(type), m_capacity(capacity),
      m_steeringType(steering), m_propulsionType(propulsion) {}

QString Boat::boatTypeToString(BoatType t) {
    return t == BoatType::Gig ? "Gig" : "Racing";
}

BoatType Boat::boatTypeFromString(const QString& s) {
    return s == "Gig" ? BoatType::Gig : BoatType::Racing;
}

QString Boat::steeringTypeToString(SteeringType s) {
    return s == SteeringType::Steered ? "Foot-Steered" : "Hand-Steered";
}

SteeringType Boat::steeringTypeFromString(const QString& s) {
    if (s == "Steered" || s == "Foot-Steered") return SteeringType::Steered;
    return SteeringType::NonSteered;
}

QString Boat::propulsionTypeToString(PropulsionType p) {
    switch (p) {
    case PropulsionType::Skull:  return "Scull";
    case PropulsionType::Riemen: return "Sweep";
    case PropulsionType::Both:   return "Both";
    }
    return "Both";
}

PropulsionType Boat::propulsionTypeFromString(const QString& s) {
    if (s == "Skull" || s == "Scull")  return PropulsionType::Skull;
    if (s == "Riemen" || s == "Sweep") return PropulsionType::Riemen;
    return PropulsionType::Both;
}
