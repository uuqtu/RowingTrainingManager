#include "assignment.h"

Assignment::Assignment() : m_createdAt(QDateTime::currentDateTime()) {}

Assignment::Assignment(int id, const QString& name, const QDateTime& createdAt)
    : m_id(id), m_name(name), m_createdAt(createdAt) {}

void Assignment::assignRowerToBoat(int boatId, int rowerId) {
    m_boatRowerMap[boatId].append(rowerId);
}

void Assignment::clearBoat(int boatId) {
    m_boatRowerMap.remove(boatId);
}
