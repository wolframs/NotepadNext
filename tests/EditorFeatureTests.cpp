#include "ApplicationSettings.h"
#include "MarkdownPreviewBrowser.h"
#include <QApplication>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextTable>
#include <QTest>
#include "lua.hpp"

class TestBrowser : public MarkdownPreviewBrowser {
public:
    using MarkdownPreviewBrowser::loadResource;
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

    void resourcesResolveRelativeToEachDocumentAndBlockNetwork() {
        QTemporaryDir first, second;
        QVERIFY(first.isValid() && second.isValid());
        QImage red(3, 3, QImage::Format_RGB32); red.fill(Qt::red);
        QImage blue(3, 3, QImage::Format_RGB32); blue.fill(Qt::blue);
        QVERIFY(red.save(first.filePath("image.png")));
        QVERIFY(blue.save(second.filePath("image.png")));
        TestBrowser browser;
        browser.renderMarkdown("![image](image.png)", QUrl::fromLocalFile(first.filePath("README.md")));
        auto image = browser.loadResource(QTextDocument::ImageResource, QUrl("image.png")).value<QImage>();
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::red));
        browser.renderMarkdown("![image](image.png)", QUrl::fromLocalFile(second.filePath("README.md")));
        image = browser.loadResource(QTextDocument::ImageResource, QUrl("image.png")).value<QImage>();
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::blue));
        QVERIFY(!browser.loadResource(QTextDocument::ImageResource, QUrl("https://example.com/image.png")).isValid());
        QVERIFY(!browser.loadResource(QTextDocument::ImageResource, QUrl("file://server/share/image.png")).isValid());
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
