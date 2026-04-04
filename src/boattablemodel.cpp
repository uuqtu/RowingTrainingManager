#include "boattablemodel.h"
#include <QStringList>

static QStringList boatTypeOptions() { return {"Gig", "Racing"}; }
static QStringList steeringOptions() { return {"Foot-Steered", "Hand-Steered"}; }
static QStringList propulsionOptions() { return {"Scull", "Sweep"}; }

BoatTableModel::BoatTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void BoatTableModel::setBoats(const QList<Boat>& boats) {
    beginResetModel();
    m_boats = boats;
    endResetModel();
}

Boat BoatTableModel::boatAt(int row) const {
    if (row >= 0 && row < m_boats.size())
        return m_boats.at(row);
    return {};
}

int BoatTableModel::rowCount(const QModelIndex&) const { return m_boats.size(); }
int BoatTableModel::columnCount(const QModelIndex&) const { return ColCount; }

QVariant BoatTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_boats.size())
        return {};
    const Boat& b = m_boats.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColId:         return b.id();
        case ColName:       return b.name();
        case ColType:       return Boat::boatTypeToString(b.boatType());
        case ColCapacity:   return b.capacity();
        case ColSteering:   return Boat::steeringTypeToString(b.steeringType());
        case ColPropulsion: return Boat::propulsionTypeToString(b.propulsionType());
        }
    }
    return {};
}

QVariant BoatTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColId:         return "ID";
        case ColName:       return "Name";
        case ColType:       return "Type";
        case ColCapacity:   return "Capacity";
        case ColSteering:   return "Steering";
        case ColPropulsion: return "Propulsion";
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags BoatTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (index.column() == ColId) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool BoatTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || role != Qt::EditRole) return false;
    Boat& b = m_boats[index.row()];
    switch (index.column()) {
    case ColName:       b.setName(value.toString()); break;
    case ColType:       b.setBoatType(Boat::boatTypeFromString(value.toString())); break;
    case ColCapacity: {
        int cap = value.toInt();
        b.setCapacity(qBound(1, cap, 5));
        break;
    }
    case ColSteering:   b.setSteeringType(Boat::steeringTypeFromString(value.toString())); break;
    case ColPropulsion: b.setPropulsionType(Boat::propulsionTypeFromString(value.toString())); break;
    default: return false;
    }
    emit dataChanged(index, index, {role});
    emit boatChanged(index.row());
    return true;
}

void BoatTableModel::addBoat(const Boat& boat) {
    beginInsertRows({}, m_boats.size(), m_boats.size());
    m_boats.append(boat);
    endInsertRows();
}

void BoatTableModel::removeBoat(int row) {
    if (row < 0 || row >= m_boats.size()) return;
    beginRemoveRows({}, row, row);
    m_boats.removeAt(row);
    endRemoveRows();
}

void BoatTableModel::updateBoat(int row, const Boat& boat) {
    if (row < 0 || row >= m_boats.size()) return;
    m_boats[row] = boat;
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
}
