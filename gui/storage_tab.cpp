#include "storage_tab.h"
#include "styles.h"
#include "../analyzer.h"
#include "../linux_process_collector.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QDateTime>
#include <QSplitter>

StorageTab::StorageTab(Analyzer* analyzer, QWidget* parent)
    : QWidget(parent), m_analyzer(analyzer)
{
    setupUI();
}

void StorageTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    QSplitter* topSplitter = new QSplitter(Qt::Horizontal);

    // ══════════════════════ LEFT PANEL ══════════════════════
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(6);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // ── Capacity Bars ──
    QGroupBox* barsBox = new QGroupBox("Storage Tier Capacity");
    QVBoxLayout* barsLayout = new QVBoxLayout(barsBox);
    barsLayout->setSpacing(6);

    auto makeTierRow = [&](const QString& label, QProgressBar*& bar, QLabel*& lbl,
                           const QColor& color) {
        QHBoxLayout* row = new QHBoxLayout();
        lbl = new QLabel(label);
        lbl->setFixedWidth(65);
        lbl->setStyleSheet(QString("font-weight: 700; color: %1; font-size: 11px;").arg(color.name()));
        bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setFixedHeight(20);
        bar->setStyleSheet(QString(
            "QProgressBar { background: #F1F5F9; border: 1px solid #E2E8F0; border-radius: 3px;"
            "  text-align: center; color: #1E293B; font-weight: 600; font-size: 10px; }"
            "QProgressBar::chunk { background: %1; border-radius: 2px; }"
        ).arg(color.name()));
        row->addWidget(lbl);
        row->addWidget(bar);
        barsLayout->addLayout(row);
    };

    makeTierRow("L1 Cache", m_l1Bar, m_l1Label, DS::hot());
    makeTierRow("L2 RAM",   m_l2Bar, m_l2Label, DS::warm());
    makeTierRow("L3 Disk",  m_l3Bar, m_l3Label, DS::cold());

    m_cacheStatsLabel = new QLabel("Cache Hit Rate: --");
    m_cacheStatsLabel->setStyleSheet(
        "font-size: 10px; color: #059669; padding: 3px 6px;"
        "background: #D1FAE5; border: 1px solid #A7F3D0; border-radius: 4px;"
    );
    barsLayout->addWidget(m_cacheStatsLabel);
    leftLayout->addWidget(barsBox);

    // ── Tier Lists ──
    auto makeTierSection = [&](const QString& title, QListWidget*& list, const QColor& color) {
        QLabel* header = new QLabel(title);
        header->setStyleSheet(QString("font-size: 11px; font-weight: 700; color: %1; padding: 1px 0;").arg(color.name()));
        leftLayout->addWidget(header);
        list = new QListWidget();
        list->setMaximumHeight(90);
        leftLayout->addWidget(list);
    };

    makeTierSection("L1 Cache (HOT)", m_l1List, DS::hot());
    makeTierSection("L2 RAM (WARM)",  m_l2List, DS::warm());
    makeTierSection("L3 Disk (COLD)", m_l3List, DS::cold());

    // ── Simulate Access ──
    QGroupBox* simBox = new QGroupBox("Simulate Access");
    QHBoxLayout* simLayout = new QHBoxLayout(simBox);
    simLayout->setSpacing(4);
    QLabel* pidLbl = new QLabel("PID:");
    pidLbl->setStyleSheet("font-weight: 600; font-size: 10px; color: #8B949E;");
    simLayout->addWidget(pidLbl);
    m_pidInput = new QSpinBox();
    m_pidInput->setRange(0, 999999);
    m_pidInput->setFixedWidth(85);
    simLayout->addWidget(m_pidInput);
    QPushButton* simBtn = new QPushButton("Go");
    simBtn->setFixedSize(45, 26);
    simBtn->setStyleSheet(
        "QPushButton { background: #6D28D9; font-size: 11px; padding: 0; border-radius: 4px; }"
        "QPushButton:hover { background: #7C3AED; }"
    );
    connect(simBtn, &QPushButton::clicked, this, &StorageTab::onSimulateAccess);
    simLayout->addWidget(simBtn);
    m_accessResult = new QLabel("");
    m_accessResult->setStyleSheet("font-size: 10px;");
    simLayout->addWidget(m_accessResult);
    simLayout->addStretch();
    leftLayout->addWidget(simBox);
    leftLayout->addStretch();

    topSplitter->addWidget(leftPanel);

    // ══════════════════════ RIGHT PANEL ══════════════════════
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    // ── Relocate Process ──
    QGroupBox* relocBox = new QGroupBox("Relocate Process");
    relocBox->setStyleSheet(
        "QGroupBox { border: 1px solid #C084FC;"
        "  background: #FAF5FF; border-radius: 8px;"
        "  margin-top: 16px; padding: 18px 10px 10px 10px; font-weight: 600; font-size: 12px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #9333EA; }"
    );
    QVBoxLayout* relocLayout = new QVBoxLayout(relocBox);
    relocLayout->setSpacing(6);

    QHBoxLayout* relocInputRow = new QHBoxLayout();
    QLabel* relocPidLbl = new QLabel("PID:");
    relocPidLbl->setStyleSheet("font-weight: 600; font-size: 11px; color: #8B949E;");
    relocInputRow->addWidget(relocPidLbl);
    m_relocPidInput = new QSpinBox();
    m_relocPidInput->setRange(0, 999999);
    m_relocPidInput->setFixedWidth(85);
    relocInputRow->addWidget(m_relocPidInput);

    QLabel* toLbl = new QLabel("To:");
    toLbl->setStyleSheet("font-weight: 600; font-size: 11px; color: #8B949E;");
    relocInputRow->addWidget(toLbl);
    m_targetLayerCombo = new QComboBox();
    m_targetLayerCombo->addItems({"L1_CACHE", "L2_RAM", "L3_DISK"});
    m_targetLayerCombo->setFixedWidth(110);
    m_targetLayerCombo->setStyleSheet(
        "QComboBox { background: #FFFFFF; color: #1E293B; border: 1px solid #E2E8F0;"
        "  border-radius: 4px; padding: 4px 8px; font-size: 11px; }"
        "QComboBox:focus { border-color: #6D28D9; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #FFFFFF; color: #1E293B;"
        "  border: 1px solid #E2E8F0; selection-background-color: rgba(109,40,217,0.12); }"
    );
    relocInputRow->addWidget(m_targetLayerCombo);

    QPushButton* previewBtn = new QPushButton("Preview");
    previewBtn->setFixedHeight(28);
    previewBtn->setStyleSheet(
        "QPushButton { background: #FFFFFF; color: #6D28D9; border: 1px solid #E2E8F0;"
        "  border-radius: 4px; font-size: 11px; font-weight: 600; padding: 0 14px; }"
        "QPushButton:hover { border-color: #6D28D9; background: #FFFFFF; }"
    );
    connect(previewBtn, &QPushButton::clicked, this, &StorageTab::onRelocatePreview);
    relocInputRow->addWidget(previewBtn);

    QPushButton* moveBtn = new QPushButton("Move");
    moveBtn->setFixedHeight(28);
    moveBtn->setStyleSheet(
        "QPushButton { background: #6D28D9; font-size: 11px; padding: 0 14px; border-radius: 4px; }"
        "QPushButton:hover { background: #7C3AED; }"
    );
    connect(moveBtn, &QPushButton::clicked, this, &StorageTab::onRelocateExecute);
    relocInputRow->addWidget(moveBtn);
    relocInputRow->addStretch();
    relocLayout->addLayout(relocInputRow);

    m_impactLabel = new QLabel("Enter a PID and target layer, then click Preview.");
    m_impactLabel->setWordWrap(true);
    m_impactLabel->setStyleSheet(
        "font-size: 11px; color: #64748B; padding: 6px 8px;"
        "background: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 4px;"
    );
    m_impactLabel->setMinimumHeight(60);
    relocLayout->addWidget(m_impactLabel);
    rightLayout->addWidget(relocBox);

    // ── Smart Suggestions ──
    rightLayout->addWidget(DS::makeSectionHeader("Smart Suggestions"));

    m_suggestionsTable = new QTableWidget();
    m_suggestionsTable->setColumnCount(5);
    m_suggestionsTable->setHorizontalHeaderLabels({"Process", "Current", "Suggested", "Score", "Reason"});
    m_suggestionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_suggestionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_suggestionsTable->horizontalHeader()->setStretchLastSection(true);
    m_suggestionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_suggestionsTable->verticalHeader()->setVisible(false);
    m_suggestionsTable->verticalHeader()->setDefaultSectionSize(24);
    m_suggestionsTable->setShowGrid(false);
    m_suggestionsTable->setMaximumHeight(160);
    m_suggestionsTable->setToolTip("Click a row to auto-fill relocation fields");
    connect(m_suggestionsTable, &QTableWidget::cellClicked, this, &StorageTab::onSuggestionClicked);
    rightLayout->addWidget(m_suggestionsTable);

    // ── Movement Log ──
    rightLayout->addWidget(DS::makeSectionHeader("Data Movement Log"));

    m_movementTable = new QTableWidget();
    m_movementTable->setColumnCount(6);
    m_movementTable->setHorizontalHeaderLabels({"Time", "PID", "Process", "From", "To", "Reason"});
    m_movementTable->horizontalHeader()->setStretchLastSection(true);
    m_movementTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_movementTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_movementTable->verticalHeader()->setVisible(false);
    m_movementTable->verticalHeader()->setDefaultSectionSize(24);
    m_movementTable->setShowGrid(false);
    rightLayout->addWidget(m_movementTable, 1);

    topSplitter->addWidget(rightPanel);
    topSplitter->setStretchFactor(0, 2);
    topSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(topSplitter);
}

void StorageTab::refresh() {
    if (!m_analyzer->hasData()) return;
    populateTiers();
    populateMovementLog();
    populateSuggestions();
}

void StorageTab::populateTiers() {
    const StorageEngine& eng = m_analyzer->getStorageEngine();
    int l1 = eng.getL1Count(), l2 = eng.getL2Count(), l3 = eng.getL3Count();
    int total = l1 + l2 + l3;
    if (total == 0) return;

    m_l1Bar->setMaximum(total);
    m_l1Bar->setValue(l1);
    m_l1Bar->setFormat(QString("L1: %1/%2 (%3%)").arg(l1).arg(total).arg(100 * l1 / total));

    m_l2Bar->setMaximum(total);
    m_l2Bar->setValue(l2);
    m_l2Bar->setFormat(QString("L2: %1/%2 (%3%)").arg(l2).arg(total).arg(100 * l2 / total));

    m_l3Bar->setMaximum(total);
    m_l3Bar->setValue(l3);
    m_l3Bar->setFormat(QString("L3: %1/%2 (%3%)").arg(l3).arg(total).arg(100 * l3 / total));

    m_cacheStatsLabel->setText(QString("Hit Rate: %1%  |  Hits: %2  |  Misses: %3  |  Total: %4")
        .arg(eng.getCacheHitRate(), 0, 'f', 1)
        .arg(eng.getCacheHits())
        .arg(eng.getCacheMisses())
        .arg(eng.getTotalAccesses()));

    auto fillList = [](QListWidget* list, const std::vector<ProcessData>& procs,
                       const QColor& color, int maxShow) {
        list->clear();
        int show = std::min(maxShow, (int)procs.size());
        for (int i = 0; i < show; i++) {
            QString text = QString("%1  |  Score: %2  |  %3 MB")
                .arg(QString::fromStdString(Analyzer::cleanName(procs[i].name)))
                .arg(procs[i].hotnessScore, 0, 'f', 1)
                .arg(procs[i].memoryMB, 0, 'f', 0);
            QListWidgetItem* item = new QListWidgetItem(text);
            item->setForeground(color);
            list->addItem(item);
        }
        if ((int)procs.size() > maxShow) {
            QListWidgetItem* more = new QListWidgetItem(
                QString("  +%1 more").arg(procs.size() - maxShow));
            more->setForeground(QColor("#484F58"));
            list->addItem(more);
        }
    };

    fillList(m_l1List, eng.getL1Processes(), DS::hot(), 6);
    fillList(m_l2List, eng.getL2Processes(), DS::warm(), 6);
    fillList(m_l3List, eng.getL3Processes(), DS::cold(), 6);
}

void StorageTab::populateMovementLog() {
    const auto& log = m_analyzer->getStorageEngine().getMovementLog();
    m_movementTable->setRowCount((int)log.size());

    int row = 0;
    for (auto it = log.rbegin(); it != log.rend(); ++it, ++row) {
        auto makeItem = [](const QString& text, const QColor& color) {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setForeground(color);
            return item;
        };

        QString timeText = QDateTime::fromSecsSinceEpoch((qint64)it->timestamp)
            .toString("HH:mm:ss");

        m_movementTable->setItem(row, 0, makeItem(timeText, DS::textMuted()));
        m_movementTable->setItem(row, 1, makeItem(QString::number(it->pid), DS::textSec()));
        m_movementTable->setItem(row, 2, makeItem(
            QString::fromStdString(Analyzer::cleanName(it->processName)), DS::text()));
        m_movementTable->setItem(row, 3, makeItem(
            QString::fromStdString(it->fromLayer), DS::layerColor(it->fromLayer)));
        m_movementTable->setItem(row, 4, makeItem(
            QString::fromStdString(it->toLayer), DS::layerColor(it->toLayer)));
        m_movementTable->setItem(row, 5, makeItem(
            QString::fromStdString(it->reason), DS::textMuted()));
    }
}

void StorageTab::populateSuggestions() {
    std::vector<PlacementSuggestion> suggestions = m_analyzer->getSmartSuggestions();
    int showCount = std::min(8, (int)suggestions.size());
    m_suggestionsTable->setRowCount(showCount);

    for (int i = 0; i < showCount; i++) {
        const auto& s = suggestions[i];

        auto makeItem = [](const QString& text, const QColor& color) {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setForeground(color);
            return item;
        };

        m_suggestionsTable->setItem(i, 0, makeItem(
            QString::fromStdString(Analyzer::cleanName(s.processName)), DS::text()));
        m_suggestionsTable->setItem(i, 1, makeItem(
            QString::fromStdString(s.currentLayer), DS::layerColor(s.currentLayer)));
        m_suggestionsTable->setItem(i, 2, makeItem(
            QString::fromStdString(s.suggestedLayer), DS::layerColor(s.suggestedLayer)));
        m_suggestionsTable->setItem(i, 3, makeItem(
            QString::number(s.hotnessScore, 'f', 1), DS::classColor(
                s.hotnessScore >= 70 ? "HOT" : (s.hotnessScore >= 40 ? "WARM" : "COLD"))));
        m_suggestionsTable->setItem(i, 4, makeItem(
            QString::fromStdString(s.reason), DS::textSec()));

        // Store PID in the first column's data for click handler
        m_suggestionsTable->item(i, 0)->setData(Qt::UserRole, s.pid);
        m_suggestionsTable->item(i, 2)->setData(Qt::UserRole,
            QString::fromStdString(s.suggestedLayer));
    }
}

void StorageTab::onSimulateAccess() {
    int pid = m_pidInput->value();
    ProcessData result;
    bool found = m_analyzer->simulateAccess(pid, result);

    if (found) {
        m_accessResult->setStyleSheet(
            "color: #10B981; font-size: 10px; font-weight: 600;"
            "background: rgba(16, 185, 129, 0.10);"
            "border: 1px solid rgba(16, 185, 129, 0.15);"
            "border-radius: 4px; padding: 2px 6px;"
        );
        m_accessResult->setText(QString("%1 -> %2")
            .arg(QString::fromStdString(Analyzer::cleanName(result.name)))
            .arg(QString::fromStdString(result.storageLayer)));
    } else {
        m_accessResult->setStyleSheet(
            "color: #EF4444; font-size: 10px; font-weight: 600;"
            "background: rgba(239, 68, 68, 0.10);"
            "border: 1px solid rgba(239, 68, 68, 0.15);"
            "border-radius: 4px; padding: 2px 6px;"
        );
        m_accessResult->setText("PID not found");
    }

    emit requestRefresh();
}

void StorageTab::onRelocatePreview() {
    int pid = m_relocPidInput->value();
    std::string target = m_targetLayerCombo->currentText().toStdString();

    RelocationImpact impact = m_analyzer->getRelocationImpact(pid, target);

    if (!impact.success) {
        m_impactLabel->setStyleSheet(
            "font-size: 11px; color: #EF4444; padding: 6px 8px;"
            "background: rgba(239, 68, 68, 0.06); border: 1px solid rgba(239, 68, 68, 0.20);"
            "border-radius: 4px;"
        );
        m_impactLabel->setText(QString::fromStdString(impact.errorMsg));
        return;
    }

    // Build impact display
    QString speedText;
    if (impact.speedupFactor > 1.0)
        speedText = QString("%1x faster").arg(impact.speedupFactor, 0, 'f', 0);
    else if (impact.speedupFactor < 1.0)
        speedText = QString("%1x slower").arg(1.0 / impact.speedupFactor, 0, 'f', 0);
    else
        speedText = "No change";

    QString latBefore, latAfter;
    if (impact.latencyBefore >= 1000000) latBefore = QString("%1 ms").arg(impact.latencyBefore / 1000000, 0, 'f', 0);
    else if (impact.latencyBefore >= 1000) latBefore = QString("%1 us").arg(impact.latencyBefore / 1000, 0, 'f', 0);
    else latBefore = QString("%1 ns").arg(impact.latencyBefore, 0, 'f', 0);

    if (impact.latencyAfter >= 1000000) latAfter = QString("%1 ms").arg(impact.latencyAfter / 1000000, 0, 'f', 0);
    else if (impact.latencyAfter >= 1000) latAfter = QString("%1 us").arg(impact.latencyAfter / 1000, 0, 'f', 0);
    else latAfter = QString("%1 ns").arg(impact.latencyAfter, 0, 'f', 0);

    QString info = QString(
        "%1 (PID %2, Score: %3)\n"
        "%4 -> %5  |  %6\n"
        "Latency: %7 -> %8  |  Cache Hit Rate: %9% -> %10%\n"
        "Source: %11/%12  |  Target: %13/%14  |  Verdict: %15"
    ).arg(QString::fromStdString(Analyzer::cleanName(impact.processName)))
     .arg(impact.pid).arg(impact.hotnessScore, 0, 'f', 1)
     .arg(QString::fromStdString(impact.fromLayer))
     .arg(QString::fromStdString(impact.toLayer))
     .arg(impact.isPromotion ? "PROMOTION" : "DEMOTION")
     .arg(latBefore).arg(latAfter)
     .arg(impact.hitRateBefore, 0, 'f', 1).arg(impact.hitRateAfter, 0, 'f', 1)
     .arg(impact.fromTierCountBefore).arg(impact.fromTierCapacity)
     .arg(impact.toTierCountBefore).arg(impact.toTierCapacity)
     .arg(QString::fromStdString(impact.verdict));

    // Color based on verdict
    QString borderColor, bgColor, textColor;
    if (impact.verdict == "Recommended") {
        borderColor = "rgba(16, 185, 129, 0.30)";
        bgColor = "rgba(16, 185, 129, 0.06)";
        textColor = "#10B981";
    } else if (impact.verdict == "Not Recommended") {
        borderColor = "rgba(239, 68, 68, 0.30)";
        bgColor = "rgba(239, 68, 68, 0.06)";
        textColor = "#EF4444";
    } else {
        borderColor = "rgba(245, 158, 11, 0.30)";
        bgColor = "rgba(245, 158, 11, 0.06)";
        textColor = "#F59E0B";
    }

    m_impactLabel->setStyleSheet(QString(
        "font-size: 11px; color: %1; padding: 6px 8px;"
        "background: %2; border: 1px solid %3; border-radius: 4px;"
    ).arg(textColor).arg(bgColor).arg(borderColor));
    m_impactLabel->setText(info);
}

void StorageTab::onRelocateExecute() {
    int pid = m_relocPidInput->value();
    std::string target = m_targetLayerCombo->currentText().toStdString();
    std::string errorMsg;

    bool ok = m_analyzer->moveProcess(pid, target, errorMsg);

    if (ok) {
        m_impactLabel->setStyleSheet(
            "font-size: 11px; color: #10B981; padding: 6px 8px;"
            "background: rgba(16, 185, 129, 0.08); border: 1px solid rgba(16, 185, 129, 0.20);"
            "border-radius: 4px;"
        );
        m_impactLabel->setText(QString("Relocated PID %1 to %2 successfully.")
            .arg(pid).arg(m_targetLayerCombo->currentText()));

        emit requestRefresh();
    } else {
        m_impactLabel->setStyleSheet(
            "font-size: 11px; color: #EF4444; padding: 6px 8px;"
            "background: rgba(239, 68, 68, 0.06); border: 1px solid rgba(239, 68, 68, 0.20);"
            "border-radius: 4px;"
        );
        m_impactLabel->setText(QString("Failed: %1").arg(QString::fromStdString(errorMsg)));
    }
}

void StorageTab::onSuggestionClicked(int row, int) {
    QTableWidgetItem* pidItem = m_suggestionsTable->item(row, 0);
    QTableWidgetItem* layerItem = m_suggestionsTable->item(row, 2);
    if (!pidItem || !layerItem) return;

    int pid = pidItem->data(Qt::UserRole).toInt();
    QString targetLayer = layerItem->data(Qt::UserRole).toString();

    m_relocPidInput->setValue(pid);
    int idx = m_targetLayerCombo->findText(targetLayer);
    if (idx >= 0) m_targetLayerCombo->setCurrentIndex(idx);

    // Auto-preview
    onRelocatePreview();
}
