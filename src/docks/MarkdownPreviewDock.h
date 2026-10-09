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
    QMetaObject::Connection renameConnection;
    QMetaObject::Connection lexerConnection;
};
