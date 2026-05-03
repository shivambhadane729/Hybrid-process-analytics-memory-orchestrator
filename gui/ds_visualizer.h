#ifndef DS_VISUALIZER_H
#define DS_VISUALIZER_H

#include <QWidget>
#include <QTabWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QPushButton>

class Analyzer;

class DSVisualizerTab : public QWidget {
    Q_OBJECT

public:
    explicit DSVisualizerTab(Analyzer* analyzer, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void zoomIn();
    void zoomOut();
    void zoomFit();

private:
    Analyzer*        m_analyzer;
    QTabWidget*      m_subTabs;

    QGraphicsView*   m_heapView;
    QGraphicsScene*  m_heapScene;
    QGraphicsView*   m_rbView;
    QGraphicsScene*  m_rbScene;
    QGraphicsView*   m_skipView;
    QGraphicsScene*  m_skipScene;
    QGraphicsView*   m_lruView;
    QGraphicsScene*  m_lruScene;
    QGraphicsView*   m_segView;
    QGraphicsScene*  m_segScene;
    QGraphicsView*   m_fenwickView;
    QGraphicsScene*  m_fenwickScene;
    
    QFrame*          m_impactPanel;
    QLabel*          m_impactTitle;
    QLabel*          m_impactStats;

    void setupUI();
    void drawHeap();
    void drawRBTree();
    void drawSkipList();
    void drawLRU();
    void drawSegmentTree();
    void drawFenwickTree();

    void drawNode(QGraphicsScene* scene, double x, double y, double r,
                  const QString& label, const QString& sub, const QColor& fill,
                  const QColor& outline = QColor(48, 54, 61), double outlineWidth = 1.5);
    void drawEdge(QGraphicsScene* scene, double x1, double y1, double x2, double y2);

    QGraphicsView* currentView();
};

#endif // DS_VISUALIZER_H
