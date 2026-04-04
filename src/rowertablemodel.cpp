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
        case ColCanSteer:          return {};
        case ColIsObmann:          return {};
        case ColPropulsionAbility: return Boat::propulsionTypeToString(r.propulsionAbility());
        case ColAge:               return Rower::ageBandToString(r.ageBand());
        case ColStrength:          return r.strength() > 0 ? QString::number(r.strength()) : QString("—");
        case ColStrokeLength:      return Rower::strokeLengthToString(r.strokeLength());
        case ColBodySize:          return Rower::bodySizeToString(r.bodySize());
        case ColAttr3:             return r.attr3() > 0 ? QString::number(r.attr3()) : QString("—");
        case ColAttrGrp1:          return r.attrGrp1() > 0 ? QString::number(r.attrGrp1()) : QString("—");
        case ColAttrGrp2:          return r.attrGrp2() > 0 ? QString::number(r.attrGrp2()) : QString("—");
        case ColAttrVal1:          return r.attrVal1() > 0 ? QString::number(r.attrVal1()) : QString("—");
        case ColAttrVal2:          return r.attrVal2() > 0 ? QString::number(r.attrVal2()) : QString("—");
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
        case ColStrokeLength:      return "Stroke Length";
        case ColBodySize:          return "Body Size";
        case ColAttr3:             return "Attr 3";
        case ColAttrGrp1:          return "Grp Attr 1";
        case ColAttrGrp2:          return "Grp Attr 2";
        case ColAttrVal1:          return "Val Attr 1";
        case ColAttrVal2:          return "Val Attr 2";
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

static int strokeLengthFromString(const QString& s) {
    if (s == "Short")  return 1;
    if (s == "Medium") return 2;
    if (s == "Long")   return 3;
    return 0;
}
static int bodySizeFromString(const QString& s) {
    if (s == "Small")  return 1;
    if (s == "Medium") return 2;
    if (s == "Tall")   return 3;
    return 0;
}

bool RowerTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid()) return false;
    Rower& r = m_rowers[index.row()];

    if (role == Qt::CheckStateRole) {
        bool checked = (value.toInt() == Qt::Checked);
        if (index.column() == ColCanSteer)  { r.setCanSteer(checked); }
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
    case ColAge: {
        QString s = value.toString();
        if (s.isEmpty() || s == "(unknown)") r.setAgeBand(0);
        else r.setAgeBand(s.split("-").first().toInt());
        break;
    }
    case ColStrength:     r.setStrength(value.toInt());   break;
    case ColStrokeLength: r.setStrokeLength(strokeLengthFromString(value.toString())); break;
    case ColBodySize:     r.setBodySize(bodySizeFromString(value.toString())); break;
    case ColAttr3:        r.setAttr3(value.toInt());      break;
    case ColAttrGrp1:     r.setAttrGrp1(value.toInt());  break;
    case ColAttrGrp2:     r.setAttrGrp2(value.toInt());  break;
    case ColAttrVal1:     r.setAttrVal1(value.toInt());   break;
    case ColAttrVal2:     r.setAttrVal2(value.toInt());   break;
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
