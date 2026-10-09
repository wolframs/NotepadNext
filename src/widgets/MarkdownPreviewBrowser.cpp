#include "MarkdownPreviewBrowser.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QImage>
#include <QScrollBar>
#include <QTextDocument>

MarkdownPreviewBrowser::MarkdownPreviewBrowser(QWidget *parent) : QTextBrowser(parent)
{
    setObjectName(QStringLiteral("MarkdownPreviewBrowser"));
    setOpenLinks(false);
    setOpenExternalLinks(false);
    document()->setDocumentMargin(18);
    connect(this, &QTextBrowser::anchorClicked, this, &MarkdownPreviewBrowser::followLink);
}

void MarkdownPreviewBrowser::renderMarkdown(const QString &markdown, const QUrl &baseUrl)
{
    const int scroll = verticalScrollBar()->value();
    const bool sameDocument = document()->baseUrl() == baseUrl;
    // A new document also clears cached relative images when switching folders.
    auto *rendered = new QTextDocument(this);
    rendered->setDefaultFont(font());
    rendered->setDocumentMargin(18);
    rendered->setBaseUrl(baseUrl);
    setDocument(rendered);
    rendered->setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    verticalScrollBar()->setValue(sameDocument ? scroll : 0);
}

QVariant MarkdownPreviewBrowser::loadResource(int type, const QUrl &name)
{
    const QUrl url = document()->baseUrl().resolved(name);
    // File images are useful; UNC paths must not silently access network shares.
    if (url.isLocalFile() && url.host().isEmpty()) {
        if (type == QTextDocument::ImageResource) return QImage(url.toLocalFile());
        return QTextBrowser::loadResource(type, url);
    }
    return {};
}

void MarkdownPreviewBrowser::followLink(const QUrl &url)
{
    if (url.isRelative() && url.path().isEmpty() && url.hasFragment()) {
        scrollToAnchor(url.fragment());
        return;
    }
    const QUrl resolved = document()->baseUrl().resolved(url);
    if (resolved.isLocalFile() && resolved.host().isEmpty()) {
        const QFileInfo file(resolved.toLocalFile());
        if (file.isFile())
            emit localFileRequested(file.absoluteFilePath());
    }
    else if (resolved.scheme() == QStringLiteral("https") || resolved.scheme() == QStringLiteral("http")
             || resolved.scheme() == QStringLiteral("mailto")) {
        QDesktopServices::openUrl(resolved);
    }
}
