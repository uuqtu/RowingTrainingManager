#include "rowertablemodel.h"

RowerTableModel::RowerTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void RowerTableModel::setRowers(const QList<Rower>& rowers) {
    beginResetModel(); m_rowers = rowers; endResetModel();
}
Rower RowerTableModel::rowerAt(int row) const {
    return (row >= 0 && row < m_rowers.size()) ? m_rowers.at(row) : Rower{};
}
int RowerTableModel::rowCount(const QModelIndex&) const { return m_rowers.size(); }
int RowerTableModel::columnCount(const QModelIndex&) const { return ColCount; }

QVariant RowerTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rowers.size()) return {};
    const Rower& r = m_rowers.at(index.row());

    if (role == Qt::CheckStateRole) {
        if (index.column() == ColCanSteer) return r.canSteer() ? Qt::Checked : Qt::Unchecked;
        if (index.column() == ColIsObmann) return r.isObmann() ? Qt::Checked : Qt::Unchecked;
    }
    if (role == Qt::TextAlignmentRole &&
        (index.column() == ColCanSteer || index.column() == ColIsObmann))
        return Qt::AlignCenter;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColId:                return r.id();
        case ColName:              return r.name();
        case ColSkill:             return Rower::skillToString(r.skill());
        case ColCompatibility:     return Rower::compatToString(r.compatibility());
        case ColCanSteer:          return {};   // rendered as checkbox
        case ColIsObmann:          return {};
        case ColPropulsionAbility: return Boat::propulsionTypeToString(r.propulsionAbility());
        case ColAge:               return Rower::ageBandToString(r.ageBand());
        case ColStrength:          return r.strength() > 0 ? QString::number(r.strength()) : QString("—");
        case ColAttr1:             return r.attr1();
        case ColAttr2:             return r.attr2();
        case ColAttr3:             return r.attr3();
        }
    }
    return {};
}

QVariant RowerTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColId:                return "ID";
        case ColName:              return "Name";
        case ColSkill:             return "Skill";
        case ColCompatibility:     return "Compat.";
        case ColCanSteer:          return "Foot Steerer";
        case ColIsObmann:          return "Obmann";
        case ColPropulsionAbility: return "Propulsion";
        case ColAge:               return "Age Band";
        case ColStrength:          return "Strength";
        case ColAttr1:             return "Attr 1";
        case ColAttr2:             return "Attr 2";
        case ColAttr3:             return "Attr 3";
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags RowerTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (index.column() == ColId) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == ColCanSteer || index.column() == ColIsObmann)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool RowerTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid()) return false;
    Rower& r = m_rowers[index.row()];

    if (role == Qt::CheckStateRole) {
        bool checked = (value.toInt() == Qt::Checked);
        if (index.column() == ColCanSteer) { r.setCanSteer(checked); }
        else if (index.column() == ColIsObmann) { r.setIsObmann(checked); }
        else return false;
        emit dataChanged(index, index);
        emit rowerChanged(index.row());
        return true;
    }
    if (role != Qt::EditRole) return false;

    switch (index.column()) {
    case ColName:              r.setName(value.toString()); break;
    case ColSkill:             r.setSkill(Rower::skillFromString(value.toString())); break;
    case ColCompatibility:     r.setCompatibility(Rower::compatFromString(value.toString())); break;
    case ColPropulsionAbility: r.setPropulsionAbility(Boat::propulsionTypeFromString(value.toString())); break;
    case ColStrength: r.setStrength(value.toInt()); break;
    case ColAge: {
        // Convert display string back to band int
        QString s = value.toString();
        if (s.isEmpty() || s == "(unknown)") r.setAgeBand(0);
        else r.setAgeBand(s.split("-").first().toInt());
        break;
    }
    case ColAttr1:             r.setAttr1(value.toInt()); break;
    case ColAttr2:             r.setAttr2(value.toInt()); break;
    case ColAttr3:             r.setAttr3(value.toInt()); break;
    default: return false;
    }
    emit dataChanged(index, index, {role});
    emit rowerChanged(index.row());
    return true;
}

void RowerTableModel::addRower(const Rower& rower) {
    beginInsertRows({}, m_rowers.size(), m_rowers.size());
    m_rowers.append(rower);
    endInsertRows();
}
void RowerTableModel::removeRower(int row) {
    if (row < 0 || row >= m_rowers.size()) return;
    beginRemoveRows({}, row, row);
    m_rowers.removeAt(row);
    endRemoveRows();
}
void RowerTableModel::updateRower(int row, const Rower& rower) {
    if (row < 0 || row >= m_rowers.size()) return;
    m_rowers[row] = rower;
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
}
