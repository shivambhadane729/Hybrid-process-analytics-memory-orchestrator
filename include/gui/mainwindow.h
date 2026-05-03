#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>
#include <QLabel>
#include <QCheckBox>

class Analyzer;
class DashboardTab;
class StorageTab;
class DSVisualizerTab;
class FaultTab;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Analyzer* analyzer, QWidget* parent = nullptr);
    ~MainWindow();

signals:
    void dataRefreshed();

public slots:
    void onRefresh();
    void forceRefresh();

private slots:
    void onAutoRefreshToggled(bool checked);

private:
    Analyzer*        m_analyzer;
    QTabWidget*      m_tabWidget;
    QTimer*          m_refreshTimer;
    QCheckBox*       m_autoRefreshCheck;

    DashboardTab*    m_dashboardTab;
    StorageTab*      m_storageTab;
    DSVisualizerTab* m_dsTab;
    FaultTab*        m_faultTab;

    void setupUI();
};

#endif // MAINWINDOW_H
