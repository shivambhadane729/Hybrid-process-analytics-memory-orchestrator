#ifndef STORAGE_TAB_H
#define STORAGE_TAB_H

#include <QWidget>
#include <QProgressBar>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>

class Analyzer;

class StorageTab : public QWidget {
    Q_OBJECT

public:
    explicit StorageTab(Analyzer* analyzer, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void requestRefresh();

private slots:
    void onSimulateAccess();
    void onRelocatePreview();
    void onRelocateExecute();
    void onSuggestionClicked(int row, int col);

private:
    Analyzer*      m_analyzer;

    // Tier bars
    QProgressBar*  m_l1Bar;
    QProgressBar*  m_l2Bar;
    QProgressBar*  m_l3Bar;
    QLabel*        m_l1Label;
    QLabel*        m_l2Label;
    QLabel*        m_l3Label;

    // Process lists
    QListWidget*   m_l1List;
    QListWidget*   m_l2List;
    QListWidget*   m_l3List;

    // Cache stats
    QLabel*        m_cacheStatsLabel;

    // Movement log
    QTableWidget*  m_movementTable;

    // Simulate access
    QSpinBox*      m_pidInput;
    QLabel*        m_accessResult;

    // Relocation
    QSpinBox*      m_relocPidInput;
    QComboBox*     m_targetLayerCombo;
    QLabel*        m_impactLabel;

    // Smart Suggestions
    QTableWidget*  m_suggestionsTable;

    void setupUI();
    void populateTiers();
    void populateMovementLog();
    void populateSuggestions();
};

#endif // STORAGE_TAB_H
