#pragma once

#include <QTextBrowser>

// A lightweight Markdown renderer. Remote resources are never fetched implicitly.
class MarkdownPreviewBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit MarkdownPreviewBrowser(QWidget *parent = nullptr);
    void renderMarkdown(const QString &markdown, const QUrl &baseUrl);

signals:
    void localFileRequested(const QString &path);

protected:
    QVariant loadResource(int type, const QUrl &name) override;

private:
    void followLink(const QUrl &url);
};
