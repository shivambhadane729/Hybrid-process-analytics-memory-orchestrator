#include "ds_visualizer.h"
#include "styles.h"
#include "../analyzer.h"
#include "../linux_process_collector.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>
#include <algorithm>

DSVisualizerTab::DSVisualizerTab(Analyzer* analyzer, QWidget* parent)
    : QWidget(parent), m_analyzer(analyzer)
{
    setupUI();
}

void DSVisualizerTab::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── Zoom Controls ──
    QHBoxLayout* toolbar = new QHBoxLayout();

    QLabel* titleLbl = new QLabel("Data Structure Visualizations");
    titleLbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #1E293B;");
    toolbar->addWidget(titleLbl);
    toolbar->addStretch();

    auto makeToolBtn = [](const QString& text) -> QPushButton* {
        QPushButton* btn = new QPushButton(text);
        btn->setFixedSize(32, 28);
        btn->setStyleSheet(
            "QPushButton { background: #FFFFFF; color: #1E293B; border: 1px solid #E2E8F0;"
            "  border-radius: 4px; font-weight: 700; font-size: 13px; padding: 0; }"
            "QPushButton:hover { background: #F1F5F9; border-color: #6D28D9; }"
            "QPushButton:pressed { background: #E2E8F0; }"
        );
        return btn;
    };

    QPushButton* zoomInBtn = makeToolBtn("+");
    QPushButton* zoomOutBtn = makeToolBtn("-");
    QPushButton* fitBtn = new QPushButton("Fit");
    fitBtn->setFixedHeight(28);
    fitBtn->setStyleSheet(
        "QPushButton { background: #FFFFFF; color: #64748B; border: 1px solid #E2E8F0;"
        "  border-radius: 4px; font-size: 11px; padding: 0 12px; }"
        "QPushButton:hover { background: #F1F5F9; color: #1E293B; }"
    );

    connect(zoomInBtn, &QPushButton::clicked, this, &DSVisualizerTab::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &DSVisualizerTab::zoomOut);
    connect(fitBtn, &QPushButton::clicked, this, &DSVisualizerTab::zoomFit);

    toolbar->addWidget(zoomInBtn);
    toolbar->addWidget(zoomOutBtn);
    toolbar->addSpacing(4);
    toolbar->addWidget(fitBtn);
    layout->addLayout(toolbar);

    // ── Sub-Tabs for each DS ──
    m_subTabs = new QTabWidget();

    auto makeTab = [&](const QString& name, QGraphicsView*& view, QGraphicsScene*& scene) {
        QWidget* tab = new QWidget();
        QVBoxLayout* tabLayout = new QVBoxLayout(tab);
        tabLayout->setContentsMargins(2, 2, 2, 2);
        scene = new QGraphicsScene(this);
        scene->setBackgroundBrush(QColor("#FFFFFF"));
        view = new QGraphicsView(scene);
        view->setRenderHint(QPainter::Antialiasing);
        view->setRenderHint(QPainter::SmoothPixmapTransform);
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        tabLayout->addWidget(view);
        m_subTabs->addTab(tab, name);
    };

    makeTab("Max Heap",       m_heapView,    m_heapScene);
    makeTab("Red-Black Tree", m_rbView,      m_rbScene);
    makeTab("Skip List",      m_skipView,    m_skipScene);
    makeTab("LRU List",       m_lruView,     m_lruScene);
    makeTab("Segment Tree",   m_segView,     m_segScene);
    makeTab("Fenwick Tree",   m_fenwickView, m_fenwickScene);

    layout->addWidget(m_subTabs, 1);
    
    // ── Impact Panel ──
    m_impactPanel = new QFrame();
    m_impactPanel->setFixedHeight(70);
    m_impactPanel->setStyleSheet(
        "QFrame { background: #FFFFFF; border: 1px solid #E2E8F0; border-top: 2px solid #6D28D9; "
        "  border-radius: 6px; margin: 4px; }"
    );
    m_impactPanel->setVisible(false);
    
    QHBoxLayout* impactLayout = new QHBoxLayout(m_impactPanel);
    impactLayout->setContentsMargins(12, 8, 12, 8);
    
    QVBoxLayout* textLayout = new QVBoxLayout();
    m_impactTitle = new QLabel("LAST ACTION IMPACT");
    m_impactTitle->setStyleSheet("font-size: 10px; font-weight: 700; color: #6D28D9;");
    m_impactStats = new QLabel("Move detected. Optimization applied.");
    m_impactStats->setStyleSheet("font-size: 12px; color: #1E293B;");
    textLayout->addWidget(m_impactTitle);
    textLayout->addWidget(m_impactStats);
    
    impactLayout->addLayout(textLayout, 1);
    
    QPushButton* closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(24, 24);
    closeBtn->setStyleSheet("background: transparent; color: #94A3B8; font-size: 18px;");
    connect(closeBtn, &QPushButton::clicked, [this](){ m_impactPanel->setVisible(false); });
    impactLayout->addWidget(closeBtn, 0, Qt::AlignTop);
    
    layout->addWidget(m_impactPanel);
}

// ── Zoom Controls ──
QGraphicsView* DSVisualizerTab::currentView() {
    int idx = m_subTabs->currentIndex();
    QGraphicsView* views[] = {m_heapView, m_rbView, m_skipView, m_lruView, m_segView, m_fenwickView};
    if (idx >= 0 && idx < 6) return views[idx];
    return m_heapView;
}

void DSVisualizerTab::zoomIn() {
    currentView()->scale(1.25, 1.25);
}

void DSVisualizerTab::zoomOut() {
    currentView()->scale(0.8, 0.8);
}

void DSVisualizerTab::zoomFit() {
    QGraphicsView* v = currentView();
    if (v->scene() && !v->scene()->sceneRect().isEmpty())
        v->fitInView(v->scene()->sceneRect().adjusted(-30, -50, 30, 30), Qt::KeepAspectRatio);
}

// ── Drawing Helpers ──
void DSVisualizerTab::drawNode(QGraphicsScene* scene, double x, double y, double r,
                                const QString& label, const QString& sub, const QColor& fill,
                                const QColor& outline, double outlineWidth) {
    bool isHighlighted = outlineWidth > 3.0; // Convention for highlighting
    
    QRadialGradient grad(x, y - r * 0.3, r * 1.3);
    grad.setColorAt(0, fill.lighter(130));
    grad.setColorAt(0.7, fill);
    grad.setColorAt(1, fill.darker(140));

    QPen pen(outline, outlineWidth);
    if (isHighlighted) {
        // Add a glow effect for highlighted nodes
        QGraphicsEllipseItem* glow = scene->addEllipse(x - r - 4, y - r - 4, 2 * (r + 4), 2 * (r + 4), 
            Qt::NoPen, QBrush(QColor(255, 215, 0, 40)));
        glow->setZValue(-1);
    }
    
    scene->addEllipse(x - r, y - r, 2 * r, 2 * r, pen, QBrush(grad));

#ifdef _WIN32
    QString fontName = "Segoe UI";
#else
    QString fontName = "Sans Serif";
#endif

    QGraphicsTextItem* labelText = scene->addText(label, QFont(fontName, 7, QFont::Bold));
    labelText->setDefaultTextColor(Qt::white);
    labelText->setPos(x - labelText->boundingRect().width() / 2,
                      y - labelText->boundingRect().height() / 2 - 4);

    if (!sub.isEmpty()) {
        QGraphicsTextItem* subText = scene->addText(sub, QFont(fontName, 6));
        subText->setDefaultTextColor(QColor("#64748B"));
        subText->setPos(x - subText->boundingRect().width() / 2, y + 4);
    }
}

void DSVisualizerTab::drawEdge(QGraphicsScene* scene, double x1, double y1, double x2, double y2) {
    QLinearGradient lineGrad(x1, y1, x2, y2);
    lineGrad.setColorAt(0, QColor(109, 40, 217, 100));
    lineGrad.setColorAt(1, QColor(37, 99, 235, 70));
    QPen pen(QBrush(lineGrad), 1.5);
    pen.setCapStyle(Qt::RoundCap);
    scene->addLine(x1, y1, x2, y2, pen);
}

// ── Refresh ──
void DSVisualizerTab::refresh() {
    if (!m_analyzer->hasData()) return;
    drawHeap();
    drawRBTree();
    drawSkipList();
    drawLRU();
    drawSegmentTree();
    drawFenwickTree();
    
    if (m_analyzer->hasLastImpact) {
        m_impactPanel->setVisible(true);
        const auto& imp = m_analyzer->lastImpact;
        
        QString speed = (imp.speedupFactor > 1.0) 
            ? QString("%1x Faster").arg(imp.speedupFactor, 0, 'f', 1)
            : QString("%1x Slower").arg(1.0 / imp.speedupFactor, 0, 'f', 1);
            
        QColor col = (imp.verdict == "Recommended") ? DS::green() : (imp.verdict == "Not Recommended" ? DS::hot() : DS::warm());
        m_impactTitle->setStyleSheet(QString("font-size: 10px; font-weight: 700; color: %1;").arg(col.name()));
        m_impactTitle->setText(QString("LAST ACTION IMPACT: %1").arg(QString::fromStdString(imp.verdict).toUpper()));
        
        m_impactStats->setText(QString(
            "Process: <b>%1</b> (PID %2)  |  <b>%3</b>  |  Latency: %4 → %5  |  Hit Rate: %6% → %7%"
        ).arg(QString::fromStdString(Analyzer::cleanName(imp.processName)))
         .arg(imp.pid).arg(speed).arg(imp.latencyBefore).arg(imp.latencyAfter)
         .arg(imp.hitRateBefore, 0, 'f', 1).arg(imp.hitRateAfter, 0, 'f', 1));
    }
}

// ── Max Heap ──
void DSVisualizerTab::drawHeap() {
    m_heapScene->clear();
    int n = m_analyzer->maxHeap.size();
    if (n == 0) return;

    int showCount = std::min(15, n);
    std::vector<ProcessData> top = m_analyzer->getTopK(showCount);

    double nodeR = 28;
    int levels = 0;
    { int tmp = showCount; while (tmp > 0) { levels++; tmp = (tmp - 1) / 2; } }
    levels = std::min(levels, 4);

    double totalWidth = std::pow(2, levels) * (nodeR * 3);
    double levelHeight = 85;

    // Title
    auto* title = m_heapScene->addText(
        QString("Max Heap — Top %1 Processes by Hotness Score").arg(showCount),
        DS::fontBold(10));
    title->setDefaultTextColor(DS::accent());
    title->setPos(totalWidth / 2 - 170, -45);

    for (int i = 0; i < (int)top.size(); i++) {
        int level = 0;
        { int tmp = i + 1; while (tmp > 1) { tmp /= 2; level++; } }
        int posInLevel = i - ((1 << level) - 1);
        int nodesInLevel = 1 << level;

        double x = (posInLevel + 0.5) * (totalWidth / nodesInLevel);
        double y = level * levelHeight;

        if (i > 0) {
            int parentIdx = (i - 1) / 2;
            int parentLevel = 0;
            { int tmp = parentIdx + 1; while (tmp > 1) { tmp /= 2; parentLevel++; } }
            int parentPos = parentIdx - ((1 << parentLevel) - 1);
            int parentNodesInLevel = 1 << parentLevel;
            double px = (parentPos + 0.5) * (totalWidth / parentNodesInLevel);
            double py = parentLevel * levelHeight;
            drawEdge(m_heapScene, px, py + nodeR, x, y - nodeR);
        }

        QString name = QString::fromStdString(
            Analyzer::cleanName(top[i].name)).left(10);
        QString scoreInfo = QString("%1 (%2)").arg(top[i].hotnessScore, 0, 'f', 1).arg(QString::fromStdString(top[i].storageLayer).left(2));
        QColor fill = DS::classColor(top[i].classification);
        
        bool isLastAction = (top[i].pid == m_analyzer->lastActionPid);
        QColor outline = isLastAction ? QColor("#FFD700") : ((i < 5) ? DS::accent() : DS::border());
        double outlineW = isLastAction ? 4.0 : ((i < 5) ? 2.0 : 1.0);

        drawNode(m_heapScene, x, y, nodeR, name, scoreInfo, fill, outline, outlineW);
    }

    m_heapView->fitInView(m_heapScene->sceneRect().adjusted(-30, -60, 30, 30), Qt::KeepAspectRatio);
}

// ── Red-Black Tree ──
void DSVisualizerTab::drawRBTree() {
    m_rbScene->clear();
    std::vector<ProcessData> sorted = m_analyzer->getSortedRBTree();
    if (sorted.empty()) return;

    int showCount = std::min(15, (int)sorted.size());
    sorted.resize(showCount);
    double nodeR = 26;
    double width = showCount * 95.0;

    auto* title = m_rbScene->addText(
        QString("Red-Black Tree — %1 Processes (Sorted by Score)").arg(showCount),
        DS::fontBold(10));
    title->setDefaultTextColor(DS::accent());
    title->setPos(width / 2 - 170, -45);

    struct NodePos { double x, y; int idx; };
    std::vector<NodePos> positions;

    std::function<void(int, int, int, double, double, double)> buildLayout;
    buildLayout = [&](int lo, int hi, int depth, double xMin, double xMax, double y) {
        if (lo > hi) return;
        int mid = (lo + hi) / 2;
        double x = (xMin + xMax) / 2;
        positions.push_back({x, y, mid});
        buildLayout(lo, mid - 1, depth + 1, xMin, x, y + 75);
        buildLayout(mid + 1, hi, depth + 1, x, xMax, y + 75);
    };
    buildLayout(0, showCount - 1, 0, 0, width, 0);

    // Edges
    std::function<void(int, int, int, double, double, double)> drawEdges;
    drawEdges = [&](int lo, int hi, int, double xMin, double xMax, double y) {
        if (lo > hi) return;
        int mid = (lo + hi) / 2;
        double x = (xMin + xMax) / 2;
        if (lo <= mid - 1) drawEdge(m_rbScene, x, y + nodeR, (xMin + x) / 2, y + 75 - nodeR);
        if (mid + 1 <= hi) drawEdge(m_rbScene, x, y + nodeR, (x + xMax) / 2, y + 75 - nodeR);
        drawEdges(lo, mid - 1, 0, xMin, x, y + 75);
        drawEdges(mid + 1, hi, 0, x, xMax, y + 75);
    };
    drawEdges(0, showCount - 1, 0, 0, width, 0);

    // Nodes
    for (auto& np : positions) {
        const ProcessData& p = sorted[np.idx];
        int depth = 0;
        { double ty = np.y; while (ty > 10) { ty -= 75; depth++; } }
        QColor fill = (depth % 2 == 0) ? QColor(30, 30, 50) : QColor(190, 45, 55);
        QString name = QString::fromStdString(Analyzer::cleanName(p.name)).left(10);
        
        bool isLastAction = (p.pid == m_analyzer->lastActionPid);
        QColor outline = isLastAction ? QColor("#FFD700") : DS::border();
        double outlineW = isLastAction ? 4.0 : 1.0;
        
        QString sub = QString("%1 (%2)").arg(p.hotnessScore, 0, 'f', 1).arg(QString::fromStdString(p.storageLayer).left(2));
        
        drawNode(m_rbScene, np.x, np.y, nodeR, name,
                 sub, fill, outline, outlineW);
    }

    m_rbView->fitInView(m_rbScene->sceneRect().adjusted(-30, -60, 30, 30), Qt::KeepAspectRatio);
}

// ── Skip List ──
void DSVisualizerTab::drawSkipList() {
    m_skipScene->clear();
    std::vector<ProcessData> sorted = m_analyzer->getSortedSkipList();
    if (sorted.empty()) return;

    int showCount = std::min(12, (int)sorted.size());
    int maxLevel = 3;
    double nodeW = 85, nodeH = 28, gapX = 18, gapY = 45;
    double startX = 55, startY = 15;

    auto* title = m_skipScene->addText(
        QString("Skip List — %1 Processes (Multi-Level Sorted)").arg(showCount),
        QFont("Segoe UI", 10, QFont::Bold));
    title->setDefaultTextColor(DS::accent());
    title->setPos(startX, startY - 45);

    for (int level = maxLevel; level >= 0; level--) {
        double y = startY + (maxLevel - level) * gapY;
        auto* lvlLabel = m_skipScene->addText(
            QString("L%1").arg(level), QFont("Segoe UI", 8, QFont::Bold));
        lvlLabel->setDefaultTextColor(QColor("#484F58"));
        lvlLabel->setPos(0, y + 4);

        double prevX = -1;
        for (int i = 0; i < showCount; i++) {
            if (i % (1 << level) != 0) continue;
            double x = startX + i * (nodeW + gapX);

            QColor fill = DS::classColor(sorted[i].classification).darker(130);
            bool isLastAction = (sorted[i].pid == m_analyzer->lastActionPid);
            QColor borderCol = isLastAction ? QColor("#FFD700") : DS::border();
            double borderW = isLastAction ? 2.5 : 1.0;
            
            m_skipScene->addRect(x, y, nodeW, nodeH, QPen(borderCol, borderW), QBrush(fill));

            auto* text = m_skipScene->addText(
                QString::fromStdString(Analyzer::cleanName(sorted[i].name)).left(8),
                QFont("Segoe UI", 7, QFont::Bold));
            text->setDefaultTextColor(Qt::white);
            text->setPos(x + 3, y + 2);

            auto* scoreText = m_skipScene->addText(
                QString("%1 (%2)").arg(sorted[i].hotnessScore, 0, 'f', 1).arg(QString::fromStdString(sorted[i].storageLayer).left(2)), 
                QFont("Segoe UI", 6));
            scoreText->setDefaultTextColor(QColor("#8B949E"));
            scoreText->setPos(x + 50, y + 4);

            if (prevX >= 0) {
                double arrowY = y + nodeH / 2;
                m_skipScene->addLine(prevX + nodeW, arrowY, x, arrowY,
                    QPen(DS::accentLight(), 1.5));
            }
            prevX = x;
        }
    }

    m_skipView->fitInView(m_skipScene->sceneRect().adjusted(-15, -55, 30, 30), Qt::KeepAspectRatio);
}

// ── LRU List ──
void DSVisualizerTab::drawLRU() {
    m_lruScene->clear();
    std::vector<ProcessData> recency = m_analyzer->getRecencyOrder();
    if (recency.empty()) return;

    int showCount = std::min(15, (int)recency.size());
    double nodeW = 95, nodeH = 46, gap = 35;
    double startX = 70, y = 50;

    auto* title = m_lruScene->addText(
        QString("LRU Doubly Linked List — Most Recent to Least Recent (%1 shown)").arg(showCount),
        QFont("Segoe UI", 10, QFont::Bold));
    title->setDefaultTextColor(DS::accent());
    title->setPos(startX, 0);

    auto* headLabel = m_lruScene->addText("HEAD", QFont("Segoe UI", 8, QFont::Bold));
    headLabel->setDefaultTextColor(DS::green());
    headLabel->setPos(startX - 55, y + 10);

    for (int i = 0; i < showCount; i++) {
        double x = startX + i * (nodeW + gap);
        QColor fill = DS::classColor(recency[i].classification).darker(140);
        
        bool isLastAction = (recency[i].pid == m_analyzer->lastActionPid);
        QColor borderCol = isLastAction ? QColor("#FFD700") : (i == 0 ? DS::green() : DS::border());
        double borderW = isLastAction ? 3.5 : (i == 0 ? 2.0 : 1.0);
        
        m_lruScene->addRect(x, y, nodeW, nodeH, QPen(borderCol, borderW), QBrush(fill));

        auto* nameText = m_lruScene->addText(
            QString::fromStdString(Analyzer::cleanName(recency[i].name)).left(10),
            QFont("Segoe UI", 7, QFont::Bold));
        nameText->setDefaultTextColor(Qt::white);
        nameText->setPos(x + 3, y + 3);

        auto* info = m_lruScene->addText(
            QString("%1 | Score: %2").arg(QString::fromStdString(recency[i].storageLayer)).arg(recency[i].hotnessScore, 0, 'f', 1),
            QFont("Segoe UI", 6));
        info->setDefaultTextColor(QColor("#8B949E"));
        info->setPos(x + 3, y + 18);

        auto* cls = m_lruScene->addText(
            QString::fromStdString(recency[i].classification), QFont("Segoe UI", 6, QFont::Bold));
        cls->setDefaultTextColor(DS::classColor(recency[i].classification));
        cls->setPos(x + 3, y + 30);

        if (i > 0) {
            double prevX = startX + (i - 1) * (nodeW + gap) + nodeW;
            double arrowY1 = y + nodeH / 2 - 3;
            double arrowY2 = y + nodeH / 2 + 3;
            m_lruScene->addLine(prevX, arrowY1, x, arrowY1, QPen(DS::green(), 1.2));
            m_lruScene->addLine(x, arrowY2, prevX, arrowY2, QPen(DS::hot(), 1.0));
        }
    }

    double tailX = startX + showCount * (nodeW + gap) - gap + 8;
    auto* tailLabel = m_lruScene->addText("TAIL", QFont("Segoe UI", 8, QFont::Bold));
    tailLabel->setDefaultTextColor(DS::hot());
    tailLabel->setPos(tailX, y + 10);

    m_lruView->fitInView(m_lruScene->sceneRect().adjusted(-15, -15, 30, 30), Qt::KeepAspectRatio);
}

// ── Segment Tree ──
void DSVisualizerTab::drawSegmentTree() {
    m_segScene->clear();
    int n = m_analyzer->segTree.getSize();
    if (n == 0) return;

    int showN = std::min(8, n);
    double totalWidth = showN * 110.0;
    double nodeR = 24;
    double levelH = 75;

    auto* title = m_segScene->addText(
        QString("Segment Tree — Active Time Range Sums (%1 leaves)").arg(showN),
        QFont("Segoe UI", 10, QFont::Bold));
    title->setDefaultTextColor(DS::accent());
    title->setPos(totalWidth / 2 - 170, -45);

    struct TreeNode { double x, y, val; int lo, hi; };
    std::vector<TreeNode> nodes;

    std::function<void(int, int, double, double, double)> build;
    build = [&](int lo, int hi, double xMin, double xMax, double y) {
        double x = (xMin + xMax) / 2;
        double val = m_analyzer->segTree.query(lo, hi);
        nodes.push_back({x, y, val, lo, hi});
        if (lo < hi) {
            int mid = (lo + hi) / 2;
            double leftX = (xMin + x) / 2, rightX = (x + xMax) / 2;
            drawEdge(m_segScene, x, y + nodeR, leftX, y + levelH - nodeR);
            drawEdge(m_segScene, x, y + nodeR, rightX, y + levelH - nodeR);
            build(lo, mid, xMin, x, y + levelH);
            build(mid + 1, hi, x, xMax, y + levelH);
        }
    };
    build(0, showN - 1, 0, totalWidth, 0);

    for (auto& nd : nodes) {
        bool isLeaf = (nd.lo == nd.hi);
        QColor fill = isLeaf ? DS::accent() : QColor(30, 50, 100);
        
        // Show more precision for small values
        QString label;
        if (nd.val == 0) label = "0m";
        else if (nd.val < 1.0) label = QString("%1m").arg(nd.val, 0, 'f', 2);
        else if (nd.val < 10.0) label = QString("%1m").arg(nd.val, 0, 'f', 1);
        else label = QString("%1m").arg(nd.val, 0, 'f', 0);
        
        QString sub;
        if (nd.lo == nd.hi) {
            string layer = "??";
            if (nd.lo < (int)m_analyzer->indexToPid.size()) {
                layer = m_analyzer->findProcessLayer(m_analyzer->indexToPid[nd.lo]).substr(0, 2);
            }
            sub = QString("[%1] %2").arg(nd.lo).arg(QString::fromStdString(layer));
        } else {
            sub = QString("[%1-%2]").arg(nd.lo).arg(nd.hi);
        }
        
        // Highlight if this leaf represents the last action PID
        QColor outline = DS::border();
        double outlineW = 1.0;
        if (isLeaf && nd.lo < (int)m_analyzer->indexToPid.size()) {
            if (m_analyzer->indexToPid[nd.lo] == m_analyzer->lastActionPid) {
                outline = QColor("#FFD700");
                outlineW = 4.0;
            }
        }
        
        drawNode(m_segScene, nd.x, nd.y, nodeR, label, sub, fill, outline, outlineW);
    }

    m_segView->fitInView(m_segScene->sceneRect().adjusted(-30, -60, 30, 30), Qt::KeepAspectRatio);
}

// ── Fenwick Tree ──
void DSVisualizerTab::drawFenwickTree() {
    m_fenwickScene->clear();
    int n = m_analyzer->fenwickTree.getSize();
    if (n == 0) return;

    int showN = std::min(20, n);
    double barW = 36, gap = 5, barMaxH = 220;
    double startX = 55, baseY = 270;

    auto* title = m_fenwickScene->addText(
        QString("Fenwick Tree (BIT) — Cumulative Focus Count (%1 entries)").arg(showN),
        QFont("Segoe UI", 10, QFont::Bold));
    title->setDefaultTextColor(DS::accent());
    title->setPos(startX, 0);

    int maxVal = m_analyzer->getCumulativeFrequency(showN);
    if (maxVal == 0) maxVal = 1;

    for (int i = 1; i <= showN; i++) {
        int prefixSum = m_analyzer->getCumulativeFrequency(i);
        double barH = (double)prefixSum / maxVal * barMaxH;
        double x = startX + (i - 1) * (barW + gap);
        double y = baseY - barH;

        int intensity = (int)(255.0 * prefixSum / maxVal);
        QColor fill(std::min(160, 50 + intensity), 40 + intensity / 4, std::max(70, 237 - intensity));

        bool isLastAction = false;
        if (i-1 < (int)m_analyzer->indexToPid.size()) {
            if (m_analyzer->indexToPid[i-1] == m_analyzer->lastActionPid) isLastAction = true;
        }
        
        QColor borderCol = isLastAction ? QColor("#FFD700") : DS::border();
        double borderW = isLastAction ? 3.0 : 1.0;
        
        m_fenwickScene->addRect(x, y, barW, barH, QPen(borderCol, borderW), QBrush(fill));

        auto* valText = m_fenwickScene->addText(
            QString::number(prefixSum), QFont("Segoe UI", 7, QFont::Bold));
        valText->setDefaultTextColor(Qt::white);
        valText->setPos(x + barW / 2 - valText->boundingRect().width() / 2, y - 16);

        auto* idxText = m_fenwickScene->addText(QString::number(i), QFont("Segoe UI", 7));
        idxText->setDefaultTextColor(QColor("#484F58"));
        idxText->setPos(x + barW / 2 - 4, baseY + 3);
    }

    m_fenwickScene->addLine(startX - 8, baseY, startX + showN * (barW + gap),
        baseY, QPen(DS::border(), 1));

    auto* yLabel = m_fenwickScene->addText("Cumulative\nFocus Count", QFont("Segoe UI", 7));
    yLabel->setDefaultTextColor(QColor("#484F58"));
    yLabel->setPos(0, 90);

    m_fenwickView->fitInView(m_fenwickScene->sceneRect().adjusted(-15, -15, 30, 30), Qt::KeepAspectRatio);
}
