/*
 * ============================================================
 *   ADAPTIVE PROCESS USAGE ANALYZER — Qt GUI Entry Point
 * ============================================================
 */

#define NOMINMAX

#include <QApplication>
#include <QFont>
#include "analyzer.h"
#include "gui/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QFont appFont("Segoe UI", 10);
    app.setFont(appFont);

    Analyzer analyzer;
    MainWindow window(&analyzer);
    window.show();

    return app.exec();
}
