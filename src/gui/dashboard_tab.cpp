#include "dashboard_tab.h"
#include "styles.h"
#include "../analyzer.h"
#include "../linux_process_collector.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <algorithm>

DashboardTab::DashboardTab(Analyzer* analyzer, QWidget* parent)
    : QWidget(parent), m_analyzer(analyzer)
{
    setupUI();
}

void DashboardTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // ── Metric Cards Row ──
    QHBoxLayout* metrics = new QHBoxLayout();
    metrics->setSpacing(8);
    metrics->addWidget(DS::makeMetricCard("TOTAL PROCESSES", m_totalLabel, DS::accent()));
    metrics->addWidget(DS::makeMetricCard("HOT", m_hotLabel, DS::hot()));
    metrics->addWidget(DS::makeMetricCard("WARM", m_warmLabel, DS::warm()));
    metrics->addWidget(DS::makeMetricCard("COLD", m_coldLabel, DS::cold()));
    mainLayout->addLayout(metrics);

    // ── Summary + Search Row ──
    QHBoxLayout* controlRow = new QHBoxLayout();
    m_summaryLabel = new QLabel("Waiting for data...");
    m_summaryLabel->setStyleSheet("font-size: 11px; color: #64748B; padding: 2px 4px;");

    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search processes...");
    m_searchBox->setFixedWidth(220);
    m_searchBox->setFixedHeight(28);
    m_searchBox->setStyleSheet(
        "QLineEdit { background: #FFFFFF; color: #1E293B; border: 1px solid #E2E8F0;"
        "  border-radius: 4px; padding: 2px 8px; font-size: 11px; }"
        "QLineEdit:focus { border-color: #6D28D9; }"
    );
    connect(m_searchBox, &QLineEdit::textChanged, this, &DashboardTab::onSearchChanged);

    controlRow->addWidget(m_summaryLabel);
    controlRow->addStretch();
    controlRow->addWidget(m_searchBox);
    mainLayout->addLayout(controlRow);

    // ── Main Content: Table + Side Panel ──
    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    // Left: Process Table
    m_processTable = new QTableWidget();
    m_processTable->setColumnCount(7);
    m_processTable->setHorizontalHeaderLabels(
        {"PID", "Process", "CPU %", "Memory (MB)", "Page Faults", "Score", "Class"});
    m_processTable->setAlternatingRowColors(true);
    m_processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_processTable->setSortingEnabled(true);
    m_processTable->horizontalHeader()->setStretchLastSection(true);
    m_processTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_processTable->verticalHeader()->setDefaultSectionSize(26);
    m_processTable->verticalHeader()->setVisible(false);
    m_processTable->setShowGrid(false);

    splitter->addWidget(m_processTable);

    // Right: Side Panels
    QWidget* sidePanel = new QWidget();
    QVBoxLayout* sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setSpacing(6);
    sideLayout->setContentsMargins(0, 0, 0, 0);

    // Top K
    sideLayout->addWidget(DS::makeSectionHeader("Top 5 Hottest"));
    m_topKList = new QListWidget();
    m_topKList->setMaximumHeight(140);
    sideLayout->addWidget(m_topKList);

    // Memory Waste
    sideLayout->addWidget(DS::makeSectionHeader("Memory Waste"));
    m_wasteList = new QListWidget();
    m_wasteList->setMaximumHeight(120);
    sideLayout->addWidget(m_wasteList);

    // Keep High
    sideLayout->addWidget(DS::makeSectionHeader("Keep High Priority"));
    m_keepList = new QListWidget();
    m_keepList->setMaximumHeight(110);
    sideLayout->addWidget(m_keepList);

    // Deprioritize
    sideLayout->addWidget(DS::makeSectionHeader("Deprioritize"));
    m_closeList = new QListWidget();
    m_closeList->setMaximumHeight(110);
    sideLayout->addWidget(m_closeList);

    sideLayout->addStretch();
    splitter->addWidget(sidePanel);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);
}

void DashboardTab::onSearchChanged(const QString& text) {
    if (m_analyzer->hasData())
        populateTable(text);
}

void DashboardTab::refresh() {
    if (!m_analyzer->hasData()) return;
    populateTable(m_searchBox->text());
    populateSidePanels();
}

void DashboardTab::populateTable(const QString& filter) {
    std::vector<ProcessData> all = m_analyzer->getAllProcesses();
    std::sort(all.begin(), all.end(), [](const ProcessData& a, const ProcessData& b) {
        return a.hotnessScore > b.hotnessScore;
    });

    // Count totals (before filtering)
    int hotC = 0, warmC = 0, coldC = 0;
    double totalMem = 0;
    for (auto& p : all) {
        if (p.classification == "HOT") hotC++;
        else if (p.classification == "WARM") warmC++;
        else coldC++;
        totalMem += p.memoryMB;
    }

    m_totalLabel->setText(QString::number(all.size()));
    m_hotLabel->setText(QString::number(hotC));
    m_warmLabel->setText(QString::number(warmC));
    m_coldLabel->setText(QString::number(coldC));

    m_summaryLabel->setText(QString("Total Memory: %1 MB  |  Score Range: %2 - %3")
        .arg(totalMem, 0, 'f', 0)
        .arg(all.empty() ? 0 : all.back().hotnessScore, 0, 'f', 1)
        .arg(all.empty() ? 0 : all.front().hotnessScore, 0, 'f', 1));

    // Apply search filter
    std::vector<ProcessData> filtered;
    if (filter.isEmpty()) {
        filtered = all;
    } else {
        QString lowerFilter = filter.toLower();
        for (auto& p : all) {
            QString name = QString::fromStdString(p.name).toLower();
            if (name.contains(lowerFilter) || QString::number(p.pid).contains(lowerFilter))
                filtered.push_back(p);
        }
    }

    m_processTable->setSortingEnabled(false);
    m_processTable->setRowCount((int)filtered.size());

    for (int row = 0; row < (int)filtered.size(); row++) {
        const ProcessData& p = filtered[row];
        bool isLastAction = (p.pid == m_analyzer->lastActionPid);
        QColor rowBg = isLastAction ? QColor(255, 215, 0, 80) : DS::classBg(p.classification);

        auto makeItem = [&](const QString& text) -> QTableWidgetItem* {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setBackground(rowBg);
            if (isLastAction) {
                item->setForeground(Qt::black);
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            return item;
        };

        auto makeNumItem = [&](double val, int prec = 1) -> QTableWidgetItem* {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setData(Qt::DisplayRole, QString::number(val, 'f', prec));
            item->setData(Qt::UserRole, val);
            item->setBackground(rowBg);
            if (isLastAction) {
                item->setForeground(Qt::black);
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            return item;
        };

        m_processTable->setItem(row, 0, makeItem(QString::number(p.pid)));
        m_processTable->setItem(row, 1, makeItem(QString::fromStdString(
            Analyzer::cleanName(p.name))));
        m_processTable->setItem(row, 2, makeNumItem(p.cpuPercent, 2));
        m_processTable->setItem(row, 3, makeNumItem(p.memoryMB, 1));

        QTableWidgetItem* faultItem = new QTableWidgetItem();
        faultItem->setData(Qt::DisplayRole, (qlonglong)p.pageFaultCount);
        faultItem->setBackground(rowBg);
        if (isLastAction) {
            faultItem->setForeground(Qt::black);
            QFont f = faultItem->font(); f.setBold(true); faultItem->setFont(f);
        }
        m_processTable->setItem(row, 4, faultItem);

        m_processTable->setItem(row, 5, makeNumItem(p.hotnessScore, 1));

        QTableWidgetItem* classItem = makeItem(QString::fromStdString(p.classification));
        classItem->setForeground(isLastAction ? Qt::black : DS::classColor(p.classification));
        QFont bold;
        bold.setBold(true);
        bold.setPointSize(9);
        classItem->setFont(bold);
        m_processTable->setItem(row, 6, classItem);
        
        if (isLastAction) {
            m_processTable->scrollToItem(m_processTable->item(row, 0));
        }
    }

    m_processTable->setSortingEnabled(true);
}

void DashboardTab::populateSidePanels() {
    // Top K
    m_topKList->clear();
    std::vector<ProcessData> topK = m_analyzer->getTopK(5);
    for (int i = 0; i < (int)topK.size(); i++) {
        QString rank = (i < 3) ? QString("#%1 ").arg(i + 1) : QString("  %1. ").arg(i + 1);
        QString text = QString("%1%2  —  %3")
            .arg(rank)
            .arg(QString::fromStdString(Analyzer::cleanName(topK[i].name)))
            .arg(topK[i].hotnessScore, 0, 'f', 1);
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setForeground(DS::classColor(topK[i].classification));
        QFont f;
        f.setBold(i < 3);
        item->setFont(f);
        m_topKList->addItem(item);
    }

    // Memory Waste
    m_wasteList->clear();
    std::vector<ProcessData> waste = m_analyzer->getMemoryWaste();
    for (int i = 0; i < std::min(6, (int)waste.size()); i++) {
        QString text = QString("%1  —  %2 MB wasted (Score: %3)")
            .arg(QString::fromStdString(Analyzer::cleanName(waste[i].name)))
            .arg(waste[i].memoryMB, 0, 'f', 0)
            .arg(waste[i].hotnessScore, 0, 'f', 1);
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setForeground(DS::warm());
        m_wasteList->addItem(item);
    }
    if (waste.empty()) {
        QListWidgetItem* item = new QListWidgetItem("No significant waste detected");
        item->setForeground(DS::green());
        m_wasteList->addItem(item);
    }

    // Recommendations
    m_keepList->clear();
    m_closeList->clear();
    std::vector<ProcessData> keepHigh, deprioritize;
    m_analyzer->getRecommendations(keepHigh, deprioritize);

    for (int i = 0; i < std::min(6, (int)keepHigh.size()); i++) {
        QString text = QString("%1  (Score: %2)")
            .arg(QString::fromStdString(Analyzer::cleanName(keepHigh[i].name)))
            .arg(keepHigh[i].hotnessScore, 0, 'f', 1);
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setForeground(DS::green());
        m_keepList->addItem(item);
    }

    for (int i = 0; i < std::min(6, (int)deprioritize.size()); i++) {
        QString text = QString("%1  (%2 MB)")
            .arg(QString::fromStdString(Analyzer::cleanName(deprioritize[i].name)))
            .arg(deprioritize[i].memoryMB, 0, 'f', 0);
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setForeground(DS::hot());
        m_closeList->addItem(item);
    }
}
