#pragma once
#include <QAbstractTableModel>
#include <QList>
#include "boat.h"

class BoatTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColId = 0, ColName, ColType, ColCapacity, ColSteering, ColPropulsion,
        ColCount
    };

    explicit BoatTableModel(QObject* parent = nullptr);

    void setBoats(const QList<Boat>& boats);
    QList<Boat> boats() const { return m_boats; }
    Boat boatAt(int row) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    void addBoat(const Boat& boat);
    void removeBoat(int row);
    void updateBoat(int row, const Boat& boat);

signals:
    void boatChanged(int row);

private:
    QList<Boat> m_boats;
};
