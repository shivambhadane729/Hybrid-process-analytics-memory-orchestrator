#include "fault_tab.h"
#include "styles.h"
#include "../analyzer.h"
#include "../process_collector.h"
#include "../fault_monitor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFont>
#include <QFrame>
#include <QSplitter>

FaultTab::FaultTab(Analyzer* analyzer, QWidget* parent)
    : QWidget(parent), m_analyzer(analyzer)
{
    setupUI();
}

void FaultTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // ── Top: Summary + Correlation ──
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    // Summary Table
    QWidget* summaryPanel = new QWidget();
    QVBoxLayout* summaryLayout = new QVBoxLayout(summaryPanel);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->addWidget(DS::makeSectionHeader("Fault Summary by Classification"));

    m_summaryTable = new QTableWidget();
    m_summaryTable->setColumnCount(6);
    m_summaryTable->setHorizontalHeaderLabels(
        {"Class", "Count", "Avg Faults", "Total Faults", "Avg Pagefile (KB)", "Avg Peak WS (KB)"});
    m_summaryTable->setRowCount(3);
    m_summaryTable->setMaximumHeight(130);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryTable->horizontalHeader()->setStretchLastSection(true);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->setShowGrid(false);
    summaryLayout->addWidget(m_summaryTable);

    topRow->addWidget(summaryPanel, 2);

    // Correlation Card
    corrCard->setStyleSheet(
        "QFrame { background: #FFFFFF; border: 1px solid #E2E8F0;"
        "  border-radius: 8px; }"
    );
    QVBoxLayout* corrLayout = new QVBoxLayout(corrCard);
    corrLayout->setSpacing(4);
    corrLayout->setContentsMargins(16, 14, 16, 14);

    QLabel* corrTitle = new QLabel("Fault-Score Correlation");
    corrTitle->setStyleSheet(
        "font-size: 10px; font-weight: 600; letter-spacing: 0.8px;"
        "color: #64748B; border: none; background: transparent;"
    );
    corrLayout->addWidget(corrTitle);

    QLabel* corrSubtitle = new QLabel("Pearson r");
    corrSubtitle->setStyleSheet(
        "font-size: 9px; color: #94A3B8; border: none; background: transparent;"
    );
    corrLayout->addWidget(corrSubtitle);

    m_correlationLabel = new QLabel("--");
    m_correlationLabel->setStyleSheet(
        "font-size: 36px; font-weight: 700; color: #6D28D9;"
        "padding: 4px 0; border: none; background: transparent;"
    );
    corrLayout->addWidget(m_correlationLabel);

    m_interpretLabel = new QLabel("");
    m_interpretLabel->setStyleSheet(
        "font-size: 11px; color: #64748B; padding: 2px 0;"
        "border: none; background: transparent;"
    );
    m_interpretLabel->setWordWrap(true);
    corrLayout->addWidget(m_interpretLabel);
    corrLayout->addStretch();

    topRow->addWidget(corrCard, 1);
    mainLayout->addLayout(topRow);

    // ── Bottom: Top Faulters ──
    mainLayout->addWidget(DS::makeSectionHeader("Top 10 Page Faulting Processes"));

    m_topFaultersTable = new QTableWidget();
    m_topFaultersTable->setColumnCount(5);
    m_topFaultersTable->setHorizontalHeaderLabels(
        {"Process", "Page Faults", "Pagefile (KB)", "Score", "Class"});
    m_topFaultersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_topFaultersTable->horizontalHeader()->setStretchLastSection(true);
    m_topFaultersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_topFaultersTable->verticalHeader()->setVisible(false);
    m_topFaultersTable->verticalHeader()->setDefaultSectionSize(26);
    m_topFaultersTable->setShowGrid(false);
    mainLayout->addWidget(m_topFaultersTable, 1);
}

void FaultTab::refresh() {
    if (!m_analyzer->hasData()) return;

    // Summary
    std::vector<FaultMonitor::FaultSummary> summaries = m_analyzer->getFaultSummaries();
    for (int row = 0; row < (int)summaries.size() && row < 3; row++) {
        const auto& fs = summaries[row];
        QColor color = DS::classColor(fs.classification);

        auto makeItem = [&](const QString& text) {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setForeground(color);
            item->setBackground(DS::classBg(fs.classification));
            return item;
        };

        QTableWidgetItem* clsItem = makeItem(QString::fromStdString(fs.classification));
        QFont bold; bold.setBold(true);
        clsItem->setFont(bold);
        m_summaryTable->setItem(row, 0, clsItem);
        m_summaryTable->setItem(row, 1, makeItem(QString::number(fs.processCount)));
        m_summaryTable->setItem(row, 2, makeItem(QString::number(fs.avgFaults, 'f', 0)));
        m_summaryTable->setItem(row, 3, makeItem(QString::number((qlonglong)fs.totalFaults)));
        m_summaryTable->setItem(row, 4, makeItem(QString::number(fs.avgPagefileKB, 'f', 0)));
        m_summaryTable->setItem(row, 5, makeItem(QString::number(fs.avgPeakWSKB, 'f', 0)));
    }

    // Correlation
    double corr = m_analyzer->getFaultCorrelation();
    m_correlationLabel->setText(QString::number(corr, 'f', 3));

    QString interp;
    if (corr > 0.3) {
        interp = "Positive — active processes cause more page faults. Memory-intensive workloads.";
        m_correlationLabel->setStyleSheet(
            "font-size: 36px; font-weight: 700; color: #D97706;"
            "padding: 4px 0; border: none; background: transparent;");
        m_interpretLabel->setStyleSheet(
            "font-size: 11px; color: #D97706; border: none; background: transparent;");
    } else if (corr < -0.3) {
        interp = "Negative — cold/inactive processes have more faults. Idle processes get swapped out.";
        m_correlationLabel->setStyleSheet(
            "font-size: 36px; font-weight: 700; color: #059669;"
            "padding: 4px 0; border: none; background: transparent;");
        m_interpretLabel->setStyleSheet(
            "font-size: 11px; color: #059669; border: none; background: transparent;");
    } else {
        interp = "Weak — no strong relationship between hotness score and page fault activity.";
        m_correlationLabel->setStyleSheet(
            "font-size: 36px; font-weight: 700; color: #6D28D9;"
            "padding: 4px 0; border: none; background: transparent;");
        m_interpretLabel->setStyleSheet(
            "font-size: 11px; color: #64748B; border: none; background: transparent;");
    }
    m_interpretLabel->setText(interp);

    // Top Faulters
    std::vector<ProcessData> faulters = m_analyzer->getTopFaulters(10);
    m_topFaultersTable->setRowCount((int)faulters.size());

    for (int row = 0; row < (int)faulters.size(); row++) {
        const ProcessData& p = faulters[row];
        QColor rowColor = DS::classColor(p.classification);

        auto makeItem = [&](const QString& text) {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setForeground(rowColor);
            item->setBackground(DS::classBg(p.classification));
            return item;
        };

        m_topFaultersTable->setItem(row, 0, makeItem(
            QString::fromStdString(ProcessCollector::cleanName(p.name))));

        QTableWidgetItem* faultItem = new QTableWidgetItem();
        faultItem->setData(Qt::DisplayRole, (qlonglong)p.pageFaultCount);
        faultItem->setForeground(rowColor);
        faultItem->setBackground(DS::classBg(p.classification));
        m_topFaultersTable->setItem(row, 1, faultItem);

        m_topFaultersTable->setItem(row, 2, makeItem(
            QString::number(p.pagefileUsageKB, 'f', 0)));
        m_topFaultersTable->setItem(row, 3, makeItem(
            QString::number(p.hotnessScore, 'f', 1)));

        QTableWidgetItem* clsItem = makeItem(QString::fromStdString(p.classification));
        QFont bold; bold.setBold(true);
        clsItem->setFont(bold);
        m_topFaultersTable->setItem(row, 4, clsItem);
    }
}
