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
    m_refreshTimer->start(2000); // Trigger auto-refresh every 2 seconds by default
    QTimer::singleShot(100, this, &MainWindow::onRefresh);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    setWindowTitle("Hybrid Process Analytics Memory Orchestrator");
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

    QLabel* logo = new QLabel("HYBRID PROCESS ANALYTICS MEMORY ORCHESTRATOR");
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
    // Auto-refresh control (Internal)
    m_autoRefreshCheck = new QCheckBox();
    m_autoRefreshCheck->setChecked(true);
    m_autoRefreshCheck->setVisible(false);
    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, &MainWindow::onAutoRefreshToggled);

    headerLayout->addStretch();

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
    statusBar()->showMessage("Hybrid Process Analytics Memory Orchestrator — Ready");
}

void MainWindow::onRefresh() {
    QApplication::processEvents();

    m_analyzer->collectAndStore();

    int count = (int)m_analyzer->getAllProcesses().size();
    statusBar()->showMessage(QString("Collected %1 processes").arg(count));

    emit dataRefreshed();
}

void MainWindow::forceRefresh() {
    emit dataRefreshed();
}

void MainWindow::onAutoRefreshToggled(bool checked) {
    if (checked) {
        m_refreshTimer->start(1000);
    } else {
        m_refreshTimer->stop();
    }
}
