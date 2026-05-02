#include "mainwindow.h"
#include "styles.h"
#include "dashboard_tab.h"
#include "storage_tab.h"
#include "ds_visualizer.h"
#include "fault_tab.h"
#include "../analyzer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStatusBar>
#include <QApplication>
#include <QFrame>
#include <QFont>

MainWindow::MainWindow(Analyzer* analyzer, QWidget* parent)
    : QMainWindow(parent), m_analyzer(analyzer)
{
    setupUI();
    QTimer::singleShot(100, this, &MainWindow::onRefresh);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    setWindowTitle("Adaptive Process Analyzer");
    resize(1440, 860);
    setMinimumSize(1000, 650);
    setStyleSheet(DS::globalStyleSheet());

    QWidget* central = new QWidget(this);
    QVBoxLayout* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header Bar ──
    QFrame* header = new QFrame();
    header->setFixedHeight(48);
    header->setStyleSheet(
        "QFrame { background: #FFFFFF; border-bottom: 1px solid #E2E8F0; }"
    );

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 0, 16, 0);

    QLabel* logo = new QLabel("ADAPTIVE PROCESS ANALYZER");
    logo->setStyleSheet(
        "font-size: 13px; font-weight: 700; letter-spacing: 1.5px;"
        "color: #1E293B; background: transparent; border: none;"
    );

    QLabel* subtitle = new QLabel("Memory Priority Engine");
    subtitle->setStyleSheet(
        "font-size: 11px; color: #94A3B8; background: transparent; border: none;"
        "margin-left: 8px;"
    );

    headerLayout->addWidget(logo);
    headerLayout->addWidget(subtitle);
    headerLayout->addStretch();

    // Refresh controls
    QPushButton* refreshBtn = new QPushButton("Refresh");
    refreshBtn->setFixedHeight(30);
    refreshBtn->setStyleSheet(
        "QPushButton { background: #10B981; color: white; border-radius: 4px;"
        "  font-weight: 600; font-size: 11px; padding: 0 16px; }"
        "QPushButton:hover { background: #34D399; }"
        "QPushButton:pressed { background: #059669; }"
    );
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefresh);

    m_autoRefreshCheck = new QCheckBox("Auto");
    m_autoRefreshCheck->setChecked(false);
    m_autoRefreshCheck->setStyleSheet(
        "QCheckBox { color: #64748B; font-size: 11px; background: transparent; border: none; }"
        "QCheckBox::indicator { width: 14px; height: 14px;"
        "  border: 1px solid #E2E8F0; border-radius: 3px; background: #FFFFFF; }"
        "QCheckBox::indicator:checked { background: #059669; border-color: #059669; }"
    );
    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, &MainWindow::onAutoRefreshToggled);

    m_liveIndicator = new QLabel(" ● LIVE ");
    m_liveIndicator->setStyleSheet(
        "color: #10B981; font-weight: 800; font-size: 10px;"
        "background: rgba(16, 185, 129, 0.15);"
        "border: 1px solid #10B981; border-radius: 4px; padding: 2px 6px;"
    );
    m_liveIndicator->setVisible(false);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet(
        "color: #10B981; font-weight: 600; font-size: 11px;"
        "background: rgba(16, 185, 129, 0.10);"
        "border: 1px solid rgba(16, 185, 129, 0.20);"
        "border-radius: 10px; padding: 3px 12px;"
    );

    headerLayout->addWidget(m_liveIndicator);
    headerLayout->addSpacing(8);
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addSpacing(8);
    headerLayout->addWidget(m_autoRefreshCheck);
    headerLayout->addSpacing(4);
    headerLayout->addWidget(refreshBtn);

    root->addWidget(header);

    // ── Tab Widget ──
    m_tabWidget = new QTabWidget();

    m_dashboardTab = new DashboardTab(m_analyzer, this);
    m_storageTab   = new StorageTab(m_analyzer, this);
    m_dsTab        = new DSVisualizerTab(m_analyzer, this);
    m_faultTab     = new FaultTab(m_analyzer, this);

    m_tabWidget->addTab(m_dashboardTab, "  Dashboard  ");
    m_tabWidget->addTab(m_storageTab,   "  Storage Engine  ");
    m_tabWidget->addTab(m_dsTab,        "  Data Structures  ");
    m_tabWidget->addTab(m_faultTab,     "  Fault Monitor  ");

    root->addWidget(m_tabWidget);
    setCentralWidget(central);

    // Connect refresh
    connect(this, &MainWindow::dataRefreshed, m_dashboardTab, &DashboardTab::refresh);
    connect(this, &MainWindow::dataRefreshed, m_storageTab,   &StorageTab::refresh);
    connect(this, &MainWindow::dataRefreshed, m_dsTab,        &DSVisualizerTab::refresh);
    connect(this, &MainWindow::dataRefreshed, m_faultTab,     &FaultTab::refresh);
    connect(m_storageTab, &StorageTab::requestRefresh, this, &MainWindow::forceRefresh);

    // Timer
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefresh);

    // Status bar
    statusBar()->showMessage("Adaptive Process Analyzer — Ready");
}

void MainWindow::onRefresh() {
    m_statusLabel->setText("Collecting...");
    m_statusLabel->setStyleSheet(
        "color: #F59E0B; font-weight: 600; font-size: 11px;"
        "background: rgba(245, 158, 11, 0.10);"
        "border: 1px solid rgba(245, 158, 11, 0.20);"
        "border-radius: 10px; padding: 3px 12px;"
    );
    QApplication::processEvents();

    m_analyzer->collectAndStore();

    int count = (int)m_analyzer->getAllProcesses().size();
    m_statusLabel->setText(QString("%1 processes").arg(count));
    m_statusLabel->setStyleSheet(
        "color: #059669; font-weight: 600; font-size: 11px;"
        "background: #D1FAE5; border: 1px solid #A7F3D0;"
        "border-radius: 10px; padding: 3px 12px;"
    );
    statusBar()->showMessage(QString("Collected %1 processes").arg(count));

    emit dataRefreshed();
}

void MainWindow::forceRefresh() {
    emit dataRefreshed();
}

void MainWindow::onAutoRefreshToggled(bool checked) {
    if (checked) {
        m_refreshTimer->start(1000);
        m_liveIndicator->setVisible(true);
    } else {
        m_refreshTimer->stop();
        m_liveIndicator->setVisible(false);
    }
}
