#pragma once
// Hand-drawn chart widgets using QPainter — no external chart library needed.
#include <QWidget>
#include <QList>
#include <QString>
#include <QPair>
#include <QColor>

// ── Bar chart: one bar group per boat, one series per variable ──────────────
struct BarSeries {
    QString label;
    QList<double> values;   // one per boat
    QColor  colour;
};

class BarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarChartWidget(QWidget* parent = nullptr);
    void setTitle(const QString& t) { m_title = t; update(); }
    void setBoatNames(const QList<QString>& n) { m_boatNames = n; update(); }
    void setSeries(const QList<BarSeries>& s) { m_series = s; update(); }
    QSize sizeHint() const override { return {600, 280}; }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString          m_title;
    QList<QString>   m_boatNames;
    QList<BarSeries> m_series;
};

// ── Radar / spider chart: one polygon per boat ───────────────────────────────
struct RadarSeries {
    QString       boatName;
    QList<double> values;   // one per axis, normalised 0..1
    QColor        colour;
};

class RadarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit RadarChartWidget(QWidget* parent = nullptr);
    void setTitle(const QString& t)       { m_title = t; update(); }
    void setAxes(const QList<QString>& a) { m_axes = a; update(); }
    void setSeries(const QList<RadarSeries>& s) { m_series = s; update(); }
    QSize sizeHint() const override { return {400, 400}; }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString             m_title;
    QList<QString>      m_axes;
    QList<RadarSeries>  m_series;
};

// ── Heatmap: rows=rowers, cols=boats, cell=value ─────────────────────────────
class HeatmapWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeatmapWidget(QWidget* parent = nullptr);
    void setTitle(const QString& t)         { m_title = t; update(); }
    void setRowLabels(const QList<QString>& r) { m_rows = r; update(); }
    void setColLabels(const QList<QString>& c) { m_cols = c; update(); }
    // values[row][col]
    void setValues(const QList<QList<double>>& v) { m_values = v; update(); }
    void setLowColour(QColor c)  { m_low = c;  update(); }
    void setHighColour(QColor c) { m_high = c; update(); }
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString  m_title;
    QList<QString>  m_rows, m_cols;
    QList<QList<double>> m_values;
    QColor   m_low  = QColor(20, 50, 80);
    QColor   m_high = QColor(0, 200, 120);
};
