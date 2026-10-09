#pragma once

#include <QDockWidget>
#include <QPointer>
#include <QTimer>

class MainWindow;
class MarkdownPreviewBrowser;
class ScintillaNext;

class MarkdownPreviewDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit MarkdownPreviewDock(MainWindow *window);

public slots:
    void refresh();

private:
    void watchEditor(ScintillaNext *editor);
    MainWindow *window;
    MarkdownPreviewBrowser *browser;
    QPointer<ScintillaNext> editor;
    QTimer refreshTimer;
    QMetaObject::Connection editorConnection;
};
