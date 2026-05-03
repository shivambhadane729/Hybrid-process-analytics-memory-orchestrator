/*
 * ============================================================
 *   ADAPTIVE PROCESS USAGE ANALYZER — Qt GUI Entry Point
 * ============================================================
 */

#ifndef _WIN32
#include <vector>
#endif

#include <QApplication>
#include <QFont>
#include "analyzer.h"
#include "gui/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Use a standard sans-serif font for Linux portability
#ifdef _WIN32
    QFont appFont("Segoe UI", 10);
#else
    QFont appFont("Sans Serif", 10);
#endif
    app.setFont(appFont);

    Analyzer analyzer;
    MainWindow window(&analyzer);
    window.show();

    return app.exec();
}
