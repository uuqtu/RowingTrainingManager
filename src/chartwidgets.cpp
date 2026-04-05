#include "chartwidgets.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <cmath>

static const QColor kBg(10, 21, 32);
static const QColor kGrid(30, 53, 72);
static const QColor kText(140, 180, 216);
static const QColor kTextDim(80, 110, 140);

// ══ BarChartWidget ═══════════════════════════════════════════════════════════

BarChartWidget::BarChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(200);
}

void BarChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), kBg);

    if (m_boatNames.isEmpty() || m_series.isEmpty()) {
        p.setPen(kTextDim);
        p.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    const int nBoats   = m_boatNames.size();
    const int nSeries  = m_series.size();
    const int lpad = 52, rpad = 16, tpad = 36, bpad = 60;
    QRect plotArea(lpad, tpad, width()-lpad-rpad, height()-tpad-bpad);
    if (plotArea.width() < 10 || plotArea.height() < 10) return;

    // Title
    if (!m_title.isEmpty()) {
        p.setPen(kText);
        QFont tf = p.font(); tf.setBold(true); tf.setPointSize(10); p.setFont(tf);
        p.drawText(QRect(0,4,width(),24), Qt::AlignCenter, m_title);
    }

    // Find range
    double maxVal = 0;
    for (auto& s : m_series) for (double v : s.values) if (v > maxVal) maxVal = v;
    if (maxVal <= 0) maxVal = 1;

    // Grid lines
    p.setPen(QPen(kGrid, 1));
    QFont sf = p.font(); sf.setPointSize(8); sf.setBold(false); p.setFont(sf);
    for (int g = 0; g <= 4; ++g) {
        double frac = g / 4.0;
        int y = plotArea.bottom() - (int)(frac * plotArea.height());
        p.setPen(QPen(kGrid, 1));
        p.drawLine(plotArea.left(), y, plotArea.right(), y);
        p.setPen(kTextDim);
        p.drawText(QRect(0, y-8, lpad-4, 16), Qt::AlignRight|Qt::AlignVCenter,
                   QString::number(maxVal * frac, 'f', 1));
    }

    // Bars
    const int groupW = plotArea.width() / nBoats;
    const int gap = 3;
    const int barW = qMax(4, (groupW - (nSeries+1)*gap) / nSeries);

    for (int b = 0; b < nBoats; ++b) {
        int groupX = plotArea.left() + b * groupW;
        for (int s = 0; s < nSeries; ++s) {
            if (b >= m_series[s].values.size()) continue;
            double v = m_series[s].values[b];
            int bh = (int)(v / maxVal * plotArea.height());
            int bx = groupX + gap + s*(barW+gap);
            int by = plotArea.bottom() - bh;
            QColor c = m_series[s].colour;
            p.fillRect(bx, by, barW, bh, c.darker(110));
            p.setPen(c.lighter(150));
            p.drawRect(bx, by, barW, bh);
        }
        // Boat label
        p.setPen(kText);
        QFont bl = p.font(); bl.setPointSize(8); p.setFont(bl);
        QRect lr(groupX, plotArea.bottom()+4, groupW, 30);
        p.drawText(lr, Qt::AlignHCenter|Qt::TextWordWrap, m_boatNames[b]);
    }

    // Legend
    int lx = lpad, ly = height() - 20;
    for (int s = 0; s < nSeries; ++s) {
        p.fillRect(lx, ly+2, 12, 10, m_series[s].colour);
        p.setPen(kText);
        p.drawText(lx+16, ly+12, m_series[s].label);
        lx += QFontMetrics(p.font()).horizontalAdvance(m_series[s].label) + 32;
        if (lx > width() - 80) { lx = lpad; ly -= 16; }
    }
}

// ══ RadarChartWidget ══════════════════════════════════════════════════════════

RadarChartWidget::RadarChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 320);
}

void RadarChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), kBg);

    if (m_axes.isEmpty() || m_series.isEmpty()) {
        p.setPen(kTextDim);
        p.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    const int nAxes = m_axes.size();
    int cx = width()/2, cy = height()/2;
    int radius = qMin(width(), height())/2 - 60;
    if (radius < 20) return;

    // Title
    if (!m_title.isEmpty()) {
        QFont tf = p.font(); tf.setBold(true); tf.setPointSize(10); p.setFont(tf);
        p.setPen(kText);
        p.drawText(QRect(0,4,width(),22), Qt::AlignCenter, m_title);
    }

    auto axisPoint = [&](int axis, double r) -> QPointF {
        double angle = 2*M_PI * axis / nAxes - M_PI/2;
        return {cx + r*cos(angle), cy + r*sin(angle)};
    };

    // Grid rings
    p.setPen(QPen(kGrid, 1));
    for (int ring = 1; ring <= 4; ++ring) {
        double r = radius * ring / 4.0;
        QPainterPath path;
        path.moveTo(axisPoint(0, r));
        for (int i = 1; i <= nAxes; ++i) path.lineTo(axisPoint(i % nAxes, r));
        path.closeSubpath();
        p.drawPath(path);
    }

    // Axes and labels
    QFont af = p.font(); af.setPointSize(8); af.setBold(false); p.setFont(af);
    for (int i = 0; i < nAxes; ++i) {
        QPointF tip = axisPoint(i, radius);
        p.setPen(QPen(kGrid, 1, Qt::DashLine));
        p.drawLine(QPointF(cx, cy), tip);
        // Label
        QPointF lp = axisPoint(i, radius + 16);
        p.setPen(kText);
        QRect lr((int)lp.x()-40, (int)lp.y()-10, 80, 20);
        p.drawText(lr, Qt::AlignCenter, m_axes[i]);
    }

    // Data polygons
    for (int s = m_series.size()-1; s >= 0; --s) {
        const auto& ser = m_series[s];
        if (ser.values.size() < nAxes) continue;
        QPainterPath path;
        for (int i = 0; i <= nAxes; ++i) {
            int idx = i % nAxes;
            double r = qBound(0.0, ser.values[idx], 1.0) * radius;
            QPointF pt = axisPoint(idx, r);
            if (i == 0) path.moveTo(pt); else path.lineTo(pt);
        }
        path.closeSubpath();
        QColor fill = ser.colour; fill.setAlpha(50);
        p.fillPath(path, fill);
        p.setPen(QPen(ser.colour, 2));
        p.drawPath(path);
        // Dots
        for (int i = 0; i < nAxes; ++i) {
            double r = qBound(0.0, ser.values[i], 1.0) * radius;
            QPointF pt = axisPoint(i, r);
            p.setBrush(ser.colour);
            p.drawEllipse(pt, 4, 4);
        }
    }

    // Legend
    int lx = 8, ly = height()-20;
    for (auto& ser : m_series) {
        p.fillRect(lx, ly, 12, 12, ser.colour);
        p.setPen(kText);
        QFont lf=p.font(); lf.setPointSize(8); p.setFont(lf);
        p.drawText(lx+16, ly+11, ser.boatName);
        lx += QFontMetrics(p.font()).horizontalAdvance(ser.boatName) + 36;
        if (lx > width()-80) { lx=8; ly-=16; }
    }
}

// ══ HeatmapWidget ═════════════════════════════════════════════════════════════

HeatmapWidget::HeatmapWidget(QWidget* parent) : QWidget(parent) {}

QSize HeatmapWidget::sizeHint() const {
    int rows = m_rows.size(), cols = m_cols.size();
    return {qMax(400, cols*90+120), qMax(200, rows*28+60)};
}

void HeatmapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), kBg);

    int rows = m_rows.size(), cols = m_cols.size();
    if (rows==0 || cols==0 || m_values.isEmpty()) {
        p.setPen(kTextDim); p.drawText(rect(), Qt::AlignCenter, "No data"); return;
    }

    if (!m_title.isEmpty()) {
        QFont tf=p.font(); tf.setBold(true); tf.setPointSize(10); p.setFont(tf);
        p.setPen(kText);
        p.drawText(QRect(0,4,width(),22), Qt::AlignCenter, m_title);
    }

    const int rowH=28, tpad=34, lpad=140, cpad=4;
    int cellW = qMax(60, (width()-lpad-cpad) / cols);

    // Find min/max
    double mn=1e18, mx=-1e18;
    for (auto& row : m_values) for (double v : row) { mn=qMin(mn,v); mx=qMax(mx,v); }
    if (mx-mn < 1e-9) mx=mn+1;

    // Column headers
    QFont hf=p.font(); hf.setPointSize(8); hf.setBold(true); p.setFont(hf);
    for (int c=0; c<cols; ++c) {
        int x = lpad + c*cellW;
        p.setPen(kText);
        p.drawText(QRect(x, tpad-16, cellW, 14), Qt::AlignCenter, m_cols[c]);
    }

    for (int r=0; r<rows; ++r) {
        int y = tpad + r*rowH;
        // Row label
        QFont rf=p.font(); rf.setPointSize(9); rf.setBold(false); p.setFont(rf);
        p.setPen(kText);
        p.drawText(QRect(4, y+2, lpad-8, rowH-4), Qt::AlignRight|Qt::AlignVCenter, m_rows[r]);
        // Cells
        for (int c=0; c<cols && c<(r<m_values.size()?m_values[r].size():0); ++c) {
            double v = m_values[r][c];
            double t = (v - mn) / (mx - mn);
            QColor cell;
            cell.setRed  ((int)(m_low.red()   + t*(m_high.red()   - m_low.red())));
            cell.setGreen((int)(m_low.green()  + t*(m_high.green() - m_low.green())));
            cell.setBlue ((int)(m_low.blue()   + t*(m_high.blue()  - m_low.blue())));
            int x = lpad + c*cellW;
            p.fillRect(x+1, y+2, cellW-2, rowH-4, cell);
            p.setPen(t > 0.55 ? QColor(10,20,30) : kText);
            QFont vf=p.font(); vf.setPointSize(8); p.setFont(vf);
            p.drawText(QRect(x+1, y+2, cellW-2, rowH-4), Qt::AlignCenter,
                       QString::number(v,'f',2));
        }
    }
}
