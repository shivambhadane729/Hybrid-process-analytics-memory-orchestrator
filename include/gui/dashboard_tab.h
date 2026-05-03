#ifndef DASHBOARD_TAB_H
#define DASHBOARD_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>

class Analyzer;

class DashboardTab : public QWidget {
    Q_OBJECT

public:
    explicit DashboardTab(Analyzer* analyzer, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onSearchChanged(const QString& text);

private:
    Analyzer*      m_analyzer;
    QTableWidget*  m_processTable;
    QLineEdit*     m_searchBox;
    QListWidget*   m_topKList;
    QListWidget*   m_wasteList;
    QListWidget*   m_keepList;
    QListWidget*   m_closeList;
    QLabel*        m_summaryLabel;

    QLabel*        m_totalLabel;
    QLabel*        m_hotLabel;
    QLabel*        m_warmLabel;
    QLabel*        m_coldLabel;

    void setupUI();
    void populateTable(const QString& filter = "");
    void populateSidePanels();
};

#endif // DASHBOARD_TAB_H
