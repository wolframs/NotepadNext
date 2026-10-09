#include "MarkdownPreviewDock.h"
#include "MainWindow.h"
#include "MarkdownPreviewBrowser.h"
#include "ScintillaNext.h"
#include <QDir>
#include <QKeySequence>

MarkdownPreviewDock::MarkdownPreviewDock(MainWindow *window)
    : QDockWidget(tr("Markdown Preview"), window), window(window), browser(new MarkdownPreviewBrowser(this))
{
    setObjectName(QStringLiteral("MarkdownPreviewDock"));
    setWidget(browser);
    setMinimumWidth(260);
    toggleViewAction()->setObjectName(QStringLiteral("actionMarkdownPreview"));
    toggleViewAction()->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+M")));
    refreshTimer.setSingleShot(true);
    refreshTimer.setInterval(250);
    connect(&refreshTimer, &QTimer::timeout, this, &MarkdownPreviewDock::refresh);
    connect(window, &MainWindow::editorActivated, this, &MarkdownPreviewDock::watchEditor);
    connect(browser, &MarkdownPreviewBrowser::localFileRequested, window, &MainWindow::openFile);
    connect(this, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible) watchEditor(this->window->currentEditor());
        else { refreshTimer.stop(); disconnect(editorConnection); }
    });
}

void MarkdownPreviewDock::watchEditor(ScintillaNext *next)
{
    disconnect(editorConnection);
    refreshTimer.stop();
    editor = next;
    if (!isVisible()) return;
    if (editor) {
        editorConnection = connect(editor, &ScintillaNext::updateUi, this, [this](Scintilla::Update flags) {
            if (Scintilla::FlagSet(flags, Scintilla::Update::Text)) refreshTimer.start();
        });
        connect(editor, &ScintillaNext::renamed, this, &MarkdownPreviewDock::refresh, Qt::UniqueConnection);
    }
    refresh();
}

void MarkdownPreviewDock::refresh()
{
    if (!isVisible()) return;
    if (!editor) { browser->setPlainText(tr("Open a Markdown document to preview it.")); return; }
    const QString suffix = editor->getFileInfo().suffix().toLower();
    if (editor->languageName != QStringLiteral("Markdown") && suffix != QStringLiteral("md")
        && suffix != QStringLiteral("markdown")) {
        browser->setPlainText(tr("Select a Markdown document to preview it."));
        return;
    }
    // Avoid blocking the editor on very large documents.
    if (editor->length() > 2 * 1024 * 1024) {
        browser->setPlainText(tr("Preview is limited to documents up to 2 MiB."));
        return;
    }
    const QUrl base = editor->isFile() ? QUrl::fromLocalFile(editor->getFilePath()) : QUrl();
    browser->renderMarkdown(QString::fromUtf8(editor->getText(editor->length() + 1)), base);
}
