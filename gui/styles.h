#ifndef STYLES_H
#define STYLES_H

#include <QString>
#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QFont>
#include <string>

// ============================================================
//  DESIGN SYSTEM — Tokens, colors, and reusable widget helpers
// ============================================================
namespace DS {

    // ── Background & Surface ──
    inline QColor bg()            { return QColor("#FFFFFF"); }
    inline QColor surface()       { return QColor("#FFFFFF"); }
    inline QColor surfaceRaised() { return QColor("#FFFFFF"); }
    inline QColor border()        { return QColor("#CBD5E1"); }
    inline QColor borderLight()   { return QColor("#94A3B8"); }

    // ── Accent ──
    inline QColor accent()        { return QColor("#6D28D9"); }
    inline QColor accentLight()   { return QColor("#8B5CF6"); }
    inline QColor accentDim()     { return QColor("#EDE9FE"); }
    inline QColor green()         { return QColor("#059669"); }
    inline QColor greenDim()      { return QColor("#D1FAE5"); }

    // ── Classification ──
    inline QColor hot()           { return QColor("#E11D48"); }
    inline QColor warm()          { return QColor("#D97706"); }
    inline QColor cold()          { return QColor("#2563EB"); }

    inline QColor hotBg()         { return QColor(225, 29, 72, 25); }
    inline QColor warmBg()        { return QColor(217, 119, 6, 22); }
    inline QColor coldBg()        { return QColor(37, 99, 235, 22); }

    // ── Text ──
    inline QColor text()          { return QColor("#1E293B"); }
    inline QColor textSec()       { return QColor("#475569"); }
    inline QColor textMuted()     { return QColor("#94A3B8"); }

    // ── Classification lookup ──
    inline QColor classColor(const std::string& cls) {
        if (cls == "HOT")  return hot();
        if (cls == "WARM") return warm();
        return cold();
    }
    inline QColor classBg(const std::string& cls) {
        if (cls == "HOT")  return hotBg();
        if (cls == "WARM") return warmBg();
        return coldBg();
    }
    inline QColor layerColor(const std::string& layer) {
        if (layer == "L1_CACHE") return hot();
        if (layer == "L2_RAM")   return warm();
        return cold();
    }

    // ── Font helpers ──
    inline QFont font(int size = 12, int weight = QFont::Normal) {
        QFont f("Segoe UI", size);
        f.setWeight((QFont::Weight)weight);
        return f;
    }
    inline QFont fontBold(int size = 12) { return font(size, QFont::Bold); }

    // ── Global Stylesheet ──
    inline QString globalStyleSheet() {
        return R"(
            * { font-family: "Segoe UI", sans-serif; }

            QMainWindow { background-color: #FFFFFF; }

            QTabWidget::pane {
                border: 1px solid #E2E8F0;
                background-color: #FFFFFF;
                border-radius: 0;
                top: -1px;
            }
            QTabBar { qproperty-drawBase: 0; }
            QTabBar::tab {
                background: transparent;
                color: #64748B;
                padding: 10px 24px;
                margin-right: 2px;
                border: none;
                border-bottom: 2px solid transparent;
                font-weight: 600;
                font-size: 12px;
            }
            QTabBar::tab:selected {
                color: #1E293B;
                border-bottom: 2px solid #6D28D9;
                background: rgba(109, 40, 217, 0.05);
            }
            QTabBar::tab:hover:!selected {
                color: #475569;
                border-bottom: 2px solid rgba(109, 40, 217, 0.20);
            }

            QTableWidget {
                background-color: #FFFFFF;
                color: #1E293B;
                gridline-color: #F1F5F9;
                border: 1px solid #E2E8F0;
                border-radius: 6px;
                selection-background-color: rgba(109, 40, 217, 0.15);
                selection-color: #1E293B;
                font-size: 12px;
                alternate-background-color: #FFFFFF;
            }
            QHeaderView::section {
                background: #F1F5F9;
                color: #64748B;
                padding: 8px 10px;
                border: none;
                border-bottom: 1px solid #E2E8F0;
                border-right: 1px solid #E2E8F0;
                font-weight: 600;
                font-size: 11px;
                text-transform: uppercase;
            }

            QPushButton {
                background: #6D28D9;
                color: white;
                border: none;
                padding: 8px 20px;
                border-radius: 6px;
                font-weight: 600;
                font-size: 12px;
            }
            QPushButton:hover { background: #7C3AED; }
            QPushButton:pressed { background: #5B21B6; }

            QGroupBox {
                color: #1E293B;
                border: 1px solid #E2E8F0;
                border-radius: 8px;
                margin-top: 16px;
                padding: 20px 12px 12px 12px;
                font-weight: 600;
                font-size: 12px;
                background-color: #FFFFFF;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 6px;
                color: #64748B;
            }

            QLabel { color: #1E293B; }

            QSpinBox, QLineEdit {
                background-color: #FFFFFF;
                color: #1E293B;
                border: 1px solid #E2E8F0;
                padding: 6px 10px;
                border-radius: 6px;
            }
            QSpinBox:focus, QLineEdit:focus {
                border: 1px solid #6D28D9;
            }

            QProgressBar {
                background-color: #F1F5F9;
                border: 1px solid #E2E8F0;
                border-radius: 4px;
                text-align: center;
                color: #1E293B;
                font-weight: 600;
                font-size: 11px;
                min-height: 22px;
            }
            QProgressBar::chunk { border-radius: 3px; }

            QListWidget {
                background-color: #FFFFFF;
                color: #1E293B;
                border: 1px solid #E2E8F0;
                border-radius: 6px;
                font-size: 12px;
                padding: 4px;
                outline: none;
            }
            QListWidget::item {
                padding: 4px 8px;
                border-radius: 4px;
                margin: 1px 2px;
            }
            QListWidget::item:selected { background: rgba(109, 40, 217, 0.12); color: #1E293B; }
            QListWidget::item:hover { background: rgba(109, 40, 217, 0.06); }

            QCheckBox { color: #475569; font-size: 12px; spacing: 6px; }
            QCheckBox::indicator {
                width: 16px; height: 16px;
                border: 2px solid #E2E8F0;
                border-radius: 4px;
                background: #FFFFFF;
            }
            QCheckBox::indicator:checked { background: #6D28D9; border-color: #6D28D9; }

            QScrollBar:vertical {
                background: transparent; width: 8px; margin: 0;
            }
            QScrollBar::handle:vertical {
                background: #E2E8F0; border-radius: 4px; min-height: 24px;
            }
            QScrollBar::handle:vertical:hover { background: #CBD5E1; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
            QScrollBar:horizontal {
                background: transparent; height: 8px; margin: 0;
            }
            QScrollBar::handle:horizontal {
                background: #E2E8F0; border-radius: 4px; min-width: 24px;
            }
            QScrollBar::handle:horizontal:hover { background: #CBD5E1; }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
            QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }

            QGraphicsView {
                background-color: #FFFFFF;
                border: 1px solid #E2E8F0;
                border-radius: 6px;
            }

            QStatusBar {
                background: #FFFFFF;
                color: #64748B;
                font-size: 11px;
                border-top: 1px solid #E2E8F0;
                padding: 2px 12px;
            }

            QToolTip {
                background: #FFFFFF;
                color: #1E293B;
                border: 1px solid #6D28D9;
                border-radius: 4px;
                padding: 6px 10px;
                font-size: 12px;
            }

            QSplitter::handle {
                background: #E2E8F0;
            }
            QSplitter::handle:horizontal { width: 1px; }
            QSplitter::handle:vertical { height: 1px; }
        )";
    }

    // ── Reusable Widget Factories ──

    // Create a metric card with title, value label, and accent color
    inline QFrame* makeMetricCard(const QString& title, QLabel*& valueLabel,
                                   const QColor& accentColor) {
        QFrame* card = new QFrame();
        card->setStyleSheet(QString(
            "QFrame {"
            "  background: #FFFFFF;"
            "  border: 1px solid #E2E8F0;"
            "  border-left: 3px solid %1;"
            "  border-radius: 6px;"
            "}"
        ).arg(accentColor.name()));

        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setContentsMargins(14, 10, 14, 10);
        layout->setSpacing(2);

        QLabel* titleLbl = new QLabel(title);
        titleLbl->setStyleSheet(
            "font-size: 10px; font-weight: 600; letter-spacing: 0.8px;"
            "color: #64748B; border: none; background: transparent;"
        );

        valueLabel = new QLabel("--");
        valueLabel->setStyleSheet(QString(
            "font-size: 22px; font-weight: 700; color: %1;"
            "border: none; background: transparent;"
        ).arg(accentColor.name()));

        layout->addWidget(titleLbl);
        layout->addWidget(valueLabel);
        return card;
    }

    // Create a section header label
    inline QLabel* makeSectionHeader(const QString& text) {
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet(
            "font-size: 13px; font-weight: 700; color: #1E293B;"
            "padding: 4px 0; border: none;"
        );
        return lbl;
    }

    // Create a sub-label
    inline QLabel* makeSubLabel(const QString& text) {
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet(
            "font-size: 11px; color: #64748B; border: none;"
        );
        return lbl;
    }

    // Create a status badge
    inline QLabel* makeStatusBadge(const QString& text, const QColor& color) {
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet(QString(
            "color: %1; font-weight: 600; font-size: 11px;"
            "background: rgba(%2, %3, %4, 0.12);"
            "border: 1px solid rgba(%2, %3, %4, 0.25);"
            "border-radius: 10px; padding: 3px 12px;"
        ).arg(color.name())
         .arg(color.red()).arg(color.green()).arg(color.blue()));
        return lbl;
    }
}

#endif // STYLES_H
