#include "ApplicationSettings.h"
#include "MarkdownPreviewBrowser.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QSignalSpy>
#include <QStyleHints>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextTable>
#include <QTest>
#include "lua.hpp"

class TestBrowser : public MarkdownPreviewBrowser {
};

class EditorFeatureTests : public QObject {
    Q_OBJECT
private slots:
    void darkSyntaxIsReadableAndLightColorsStayUnchanged() {
        auto *state = luaL_newstate();
        luaL_openlibs(state);
        QCOMPARE(luaL_dostring(state, "require = function() return {} end"), LUA_OK);
        QCOMPARE(luaL_dofile(state, NOTEPADNEXT_SOURCE_DIR "/src/scripts/init.lua"), LUA_OK);
        const char *checks = R"(
            dark_mode = false; UpdateTheme()
            assert(ThemeForeground(rgb(0x000080)) == rgb(0x000080))
            dark_mode = true; UpdateTheme()
            assert(ThemeForeground(rgb(0x000000)) == theme.default_fg)
            assert(ThemeForeground(rgb(0x000080)) ~= rgb(0x000080))
            assert(ThemeForeground(rgb(0xFFFFFF)) == rgb(0xFFFFFF))
            assert(ThemeBackground(rgb(0xEEEEEE)) ~= rgb(0xEEEEEE))
            dark_mode = false; UpdateTheme()
            assert(ThemeBackground(rgb(0xEEEEEE)) == rgb(0xEEEEEE))
            dark_mode = true; UpdateTheme()
            editor = {StyleFore={}, StyleBack={}, StyleBold={}, StyleItalic={}, StyleUnderline={}, StyleEOLFilled={}}
            function editor:StyleClearAll() end
            SetStyle({styles={{id=1, fgColor=rgb(0x000080), bgColor=rgb(0xFFFFFF)}}})
            assert(editor.StyleBack[1] == theme.default_bg)
            assert(editor.StyleFore[1] ~= rgb(0x000080))
        )";
        const int result = luaL_dostring(state, checks);
        const QString error = result == LUA_OK ? QString() : QString::fromUtf8(lua_tostring(state, -1));
        lua_close(state);
        QVERIFY2(result == LUA_OK, qPrintable(error));
    }
    void markdownRendersFormattingAndUnsavedChanges() {
        TestBrowser browser;
        browser.renderMarkdown(QString::fromUtf8("# Überschrift\n\n**bold** and `code`\n\n| A | B |\n|---|---|\n| one | two |\n\n```cpp\nint x = 1;\n```"), QUrl("file:///example/README.md"));
        QCOMPARE(browser.document()->firstBlock().blockFormat().headingLevel(), 1);
        QVERIFY(browser.toPlainText().contains(QString::fromUtf8("Überschrift")));
        QVERIFY(browser.toPlainText().contains("int x = 1;"));
        bool foundTable = false;
        for (auto it = browser.document()->rootFrame()->begin(); !it.atEnd(); ++it)
            if (qobject_cast<QTextTable *>(it.currentFrame())) foundTable = true;
        QVERIFY(foundTable);
        browser.renderMarkdown("# Unsaved replacement", QUrl("file:///example/README.md"));
        QCOMPARE(browser.toPlainText().trimmed(), QString("Unsaved replacement"));
    }

    void repeatedRendersReleaseDocumentsAndPreserveScroll() {
        TestBrowser browser;
        browser.resize(400, 250);
        browser.show();
        QString text;
        for (int i = 0; i < 300; ++i) text += QString("Paragraph %1\n\n").arg(i);
        const QUrl base("file:///example/README.md");
        browser.renderMarkdown(text, base);
        QCoreApplication::processEvents();
        browser.verticalScrollBar()->setValue(browser.verticalScrollBar()->maximum() / 2);
        const int position = browser.verticalScrollBar()->value();
        QVERIFY(position > 0);
        for (int i = 0; i < 20; ++i) browser.renderMarkdown(text + "Update", base);
        QCOMPARE(browser.findChildren<QTextDocument *>().size(), 1);
        QCOMPARE(browser.verticalScrollBar()->value(), position);
    }

    void headingLinksHaveStableTargets() {
        TestBrowser browser;
        browser.renderMarkdown("# A **Heading**!\n\n# A Heading!\n\n# Überschrift\n", QUrl());
        auto block = browser.document()->begin();
        QCOMPARE(block.begin().fragment().charFormat().anchorNames(), QStringList{"a-heading"});
        block = block.next();
        QCOMPARE(block.begin().fragment().charFormat().anchorNames(), QStringList{"a-heading-1"});
        block = block.next();
        QCOMPARE(block.begin().fragment().charFormat().anchorNames(), QStringList{QString::fromUtf8("überschrift")});
    }

    void combinedLanguageStylesAreResetOnlyOnce() {
        auto *state = luaL_newstate();
        luaL_openlibs(state);
        QCOMPARE(luaL_dostring(state, "require = function() return {} end"), LUA_OK);
        QCOMPARE(luaL_dofile(state, NOTEPADNEXT_SOURCE_DIR "/src/scripts/init.lua"), LUA_OK);
        const char *checks = "languages.PHP = dofile('" NOTEPADNEXT_SOURCE_DIR "/src/languages/php.lua')\n"
            "languages.HTML = dofile('" NOTEPADNEXT_SOURCE_DIR "/src/languages/html.lua')\n" R"(
            editor = {StyleFore={}, StyleBack={}, StyleBold={}, StyleItalic={}, StyleUnderline={},
                      StyleEOLFilled={}, KeyWords={}, Property={}, MarginWidthN={}}
            local resets = 0
            function editor:StyleClearAll()
                resets = resets + 1
                local fg, bg = self.StyleFore[32], self.StyleBack[32]
                self.StyleFore = {[32]=fg}; self.StyleBack = {[32]=bg}
            end
            for _, dark in ipairs({false, true}) do
                dark_mode = dark; UpdateTheme(); resets = 0
                SetLanguage('PHP')
                assert(resets == 1)
                assert(editor.StyleFore[118] ~= nil)
                assert(editor.StyleFore[121] ~= theme.default_fg)
                assert(editor.StyleFore[1] ~= nil)
            end
        )";
        const int result = luaL_dostring(state, checks);
        const QString error = result == LUA_OK ? QString() : QString::fromUtf8(lua_tostring(state, -1));
        lua_close(state);
        QVERIFY2(result == LUA_OK, qPrintable(error));
    }

    void resourcesResolveRelativeToEachDocumentAndBlockNetwork() {
        QTemporaryDir first, second;
        QVERIFY(first.isValid() && second.isValid());
        QImage red(3, 3, QImage::Format_RGB32); red.fill(Qt::red);
        QImage blue(3, 3, QImage::Format_RGB32); blue.fill(Qt::blue);
        QVERIFY(red.save(first.filePath("image.png")));
        QVERIFY(blue.save(second.filePath("image.png")));
        TestBrowser browser;
        browser.renderMarkdown("![image](image.png)", QUrl::fromLocalFile(first.filePath("README.md")));
        auto image = browser.document()->resource(QTextDocument::ImageResource, QUrl("image.png")).value<QImage>();
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::red));
        browser.renderMarkdown("![image](image.png)", QUrl::fromLocalFile(second.filePath("README.md")));
        image = browser.document()->resource(QTextDocument::ImageResource, QUrl("image.png")).value<QImage>();
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::blue));
        // Deliberately do not contact a host; both spellings are denied before I/O.
        for (const auto &url : {"https://example.invalid/image.png", "file://example.invalid/share/image.png",
                               "file:////example.invalid/share/image.png", "file:///\\\\example.invalid/share/image.png"}) {
            QVERIFY(browser.document()->resource(QTextDocument::ImageResource, QUrl(url)).value<QImage>().isNull());
        }
        // An untitled document must not read relative images from the process cwd.
        const QString previousCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(first.path()));
        browser.renderMarkdown("![image](image.png)", QUrl());
        const QImage untitled = browser.document()->resource(QTextDocument::ImageResource, QUrl("image.png")).value<QImage>();
        QVERIFY(QDir::setCurrent(previousCwd));
        QVERIFY(untitled.isNull());
    }

    void localLinksOpenFilesWithoutReplacingPreview() {
        QTemporaryDir dir;
        QFile file(dir.filePath("other.md"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("# Other"); file.close();
        TestBrowser browser;
        browser.renderMarkdown("[next](other.md)", QUrl::fromLocalFile(dir.filePath("README.md")));
        QSignalSpy requested(&browser, &MarkdownPreviewBrowser::localFileRequested);
        browser.anchorClicked(QUrl("other.md"));
        QCOMPARE(requested.size(), 1);
        QCOMPARE(requested.first().first().toString(), file.fileName());
        QCOMPARE(browser.toPlainText().trimmed(), QString("next"));
        browser.anchorClicked(QUrl("#heading"));
        browser.anchorClicked(QUrl("javascript:alert(1)"));
        browser.anchorClicked(QUrl("file:////example.invalid/share/file.md"));
        QCOMPARE(requested.size(), 1);
    }

    void themeOverridesPersistAndSignal() {
        QTemporaryDir dir;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
        ApplicationSettings settings;
        settings.clear();
        QSignalSpy changed(&settings, &ApplicationSettings::effectiveDarkModeChanged);
        settings.setTheme(ApplicationSettings::DarkTheme);
        QVERIFY(settings.effectiveDarkMode());
        QCOMPARE(changed.size(), 1);
        settings.sync();
        ApplicationSettings reopened;
        QCOMPARE(reopened.theme(), ApplicationSettings::DarkTheme);
        settings.setTheme(ApplicationSettings::LightTheme);
        QVERIFY(!settings.effectiveDarkMode());
        settings.setTheme(ApplicationSettings::SystemTheme);
        QCOMPARE(changed.size(), 3);
        QCOMPARE(settings.effectiveDarkMode(), QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    }
};

QTEST_MAIN(EditorFeatureTests)
#include "EditorFeatureTests.moc"
