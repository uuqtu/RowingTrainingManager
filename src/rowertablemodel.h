#pragma once
#include <QAbstractTableModel>
#include <QList>
#include "rower.h"

class RowerTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColId = 0, ColName, ColSkill, ColCompatibility,
        ColCanSteer, ColIsObmann, ColPropulsionAbility,
        ColAge, ColStrength,
        ColStrokeLength, ColBodySize,
        ColAttrGrp1, ColAttrGrp2, ColAttrVal1, ColAttrVal2,
        ColCount
        // Whitelist/Blacklist (rower + boat) edited via dialog buttons only
    };

    explicit RowerTableModel(QObject* parent = nullptr);

    void setRowers(const QList<Rower>& rowers);
    QList<Rower> rowers() const { return m_rowers; }
    Rower rowerAt(int row) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    void addRower(const Rower& rower);
    void removeRower(int row);
    void updateRower(int row, const Rower& rower);

signals:
    void rowerChanged(int row);

private:
    QList<Rower> m_rowers;
};
