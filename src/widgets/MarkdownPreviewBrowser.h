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


#pragma once

#include <QTextBrowser>
#include <QTextObjectInterface>

// A lightweight Markdown renderer. Remote resources are never fetched implicitly.
class MarkdownPreviewBrowser : public QTextBrowser, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    explicit MarkdownPreviewBrowser(QWidget *parent = nullptr);
    void renderMarkdown(const QString &markdown, const QUrl &baseUrl);

signals:
    void localFileRequested(const QString &path);

private:
    QSizeF intrinsicSize(QTextDocument *document, int position, const QTextFormat &format) override;
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document,
                    int position, const QTextFormat &format) override;
    void followLink(const QUrl &url);
};
