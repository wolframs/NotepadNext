/*
 * This file is part of Notepad Next.
 * Copyright 2026 Wolfram Siener
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "MarkdownPreviewBrowser.h"

#include <QDesktopServices>
#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QImage>
#include <QDir>
#include <QPointer>
#include <QPainter>
#include <QScrollBar>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QSet>

namespace {
QString localPath(const QUrl &url)
{
    if (!url.isLocalFile() || !url.host().isEmpty()) {
        return {};
    }
    const QString path = QDir::fromNativeSeparators(url.toLocalFile());
    // A file URL can encode a UNC path without an authority/host.
    if (path.startsWith(QStringLiteral("//")) || !QDir::isAbsolutePath(path)) {
        return {};
    }
    return path;
}

class MarkdownDocument : public QTextDocument
{
public:
    using QTextDocument::QTextDocument;

protected:
    QVariant loadResource(int type, const QUrl &name) override
    {
        const QString path = localPath(baseUrl().resolved(name));
        if (type == ImageResource && !path.isEmpty()) {
            const QImage image(path);
            addResource(type, name, image);
            return image;
        }
        // Override the document loader, not just the browser: Qt's default
        // document loader falls back to QFile after an empty browser result.
        return QImage();
    }
};
}

MarkdownPreviewBrowser::MarkdownPreviewBrowser(QWidget *parent) : QTextBrowser(parent)
{
    setObjectName(QStringLiteral("MarkdownPreviewBrowser"));
    setOpenLinks(false);
    setOpenExternalLinks(false);
    document()->setDocumentMargin(18);
    connect(this, &QTextBrowser::anchorClicked, this, &MarkdownPreviewBrowser::followLink);
    connect(this, &QTextBrowser::highlighted, this, [this](const QUrl &url) {
        setToolTip(url.isEmpty() ? QString() : document()->baseUrl().resolved(url).toDisplayString());
    });
}

void MarkdownPreviewBrowser::renderMarkdown(const QString &markdown, const QUrl &baseUrl)
{
    const int scroll = verticalScrollBar()->value();
    const bool sameDocument = document()->baseUrl() == baseUrl;
    // A new document also clears cached relative images when switching folders.
    QPointer<QTextDocument> previous = document();
    auto *rendered = new MarkdownDocument(this);
    // Qt's default image handler probes files before calling resource(), and
    // retries filenames after a null image. Use only our document loader.
    rendered->documentLayout()->registerHandler(QTextFormat::ImageObject, this);
    rendered->setDefaultFont(font());
    rendered->setDocumentMargin(18);
    rendered->setBaseUrl(baseUrl);
    setDocument(rendered);
    // QTextEdit only deletes documents owned by its private text control.
    delete previous.data();
    rendered->setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    // QTextDocument headings do not automatically expose GitHub-style anchors.
    QSet<QString> headings;
    for (QTextBlock block = rendered->begin(); block.isValid(); block = block.next()) {
        if (block.blockFormat().headingLevel() == 0) {
            continue;
        }
        QString slug;
        for (const QChar character : block.text().trimmed().toLower()) {
            if (character.isLetterOrNumber() || character == u'_' || character == u'-') {
                slug += character;
            }
            else if (character.isSpace()) {
                slug += u'-';
            }
        }
        const QString baseSlug = slug;
        int duplicate = 0;
        while (headings.contains(slug)) {
            slug = baseSlug + QStringLiteral("-%1").arg(++duplicate);
        }
        headings.insert(slug);
        QTextCursor cursor(block);
        cursor.select(QTextCursor::BlockUnderCursor);
        QTextCharFormat anchor;
        anchor.setAnchorNames({slug});
        cursor.mergeCharFormat(anchor);
    }
    // Complete lazy layout before restoring a scroll position near the bottom.
    rendered->documentLayout()->documentSize();
    verticalScrollBar()->setValue(sameDocument ? scroll : 0);
}

QSizeF MarkdownPreviewBrowser::intrinsicSize(QTextDocument *document, int, const QTextFormat &format)
{
    const QTextImageFormat imageFormat = format.toImageFormat();
    const QImage image = document->resource(QTextDocument::ImageResource, QUrl(imageFormat.name())).value<QImage>();
    const QSizeF natural = image.isNull() ? QSizeF(16, 16) : image.deviceIndependentSize();
    QSizeF size = natural;
    if (imageFormat.hasProperty(QTextFormat::ImageWidth)) {
        size.setWidth(imageFormat.width());
        size.setHeight(natural.height() * size.width() / natural.width());
    }
    if (imageFormat.hasProperty(QTextFormat::ImageHeight)) {
        size.setHeight(imageFormat.height());
        if (!imageFormat.hasProperty(QTextFormat::ImageWidth)) {
            size.setWidth(natural.width() * size.height() / natural.height());
        }
    }
    return size;
}

void MarkdownPreviewBrowser::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document,
                                       int, const QTextFormat &format)
{
    const QImage image = document->resource(QTextDocument::ImageResource,
        QUrl(format.toImageFormat().name())).value<QImage>();
    if (!image.isNull()) {
        painter->drawImage(rect, image);
    }
}

void MarkdownPreviewBrowser::followLink(const QUrl &url)
{
    if (url.isRelative() && url.path().isEmpty() && url.hasFragment()) {
        scrollToAnchor(url.fragment());
        return;
    }
    const QUrl resolved = document()->baseUrl().resolved(url);
    const QString path = localPath(resolved);
    if (!path.isEmpty()) {
        const QFileInfo file(path);
        if (file.isFile()) {
            emit localFileRequested(file.absoluteFilePath());
        }
    }
    else if (resolved.scheme() == QStringLiteral("https") || resolved.scheme() == QStringLiteral("http")
             || resolved.scheme() == QStringLiteral("mailto")) {
        QDesktopServices::openUrl(resolved);
    }
}
