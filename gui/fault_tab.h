#ifndef FAULT_TAB_H
#define FAULT_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>

class Analyzer;

class FaultTab : public QWidget {
    Q_OBJECT

public:
    explicit FaultTab(Analyzer* analyzer, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    Analyzer*      m_analyzer;
    QTableWidget*  m_summaryTable;
    QTableWidget*  m_topFaultersTable;
    QLabel*        m_correlationLabel;
    QLabel*        m_interpretLabel;

    void setupUI();
};

#endif // FAULT_TAB_H
