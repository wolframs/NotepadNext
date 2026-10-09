# Upstream readiness review — 2026-10-09

Reviewed commit: `e2fefbd07469966f73682a426fd7184cd93165cc`

Reviewer: Claude CLI, `claude -p --model claude-opus-5-5 --effort high`.
Scope: `github/master...HEAD` and surrounding repository source.
Duration: 12.6 minutes; 60 turns.
Claude reported successful completion using `claude-opus-5-5`, with no permission denials.

This was a static, read-only review using Read/Glob/Grep. The reviewer did not build or run the code. The labels “confirmed” below are Claude's source-analysis classifications, not a claim that each issue was reproduced. Qt-internal behavior, performance, and OS-dependent cases need targeted verification before implementation.

No implementation changes, commits, pushes, or system configuration changes were made for this review.

For planning: the suggestion to keep tests fork-only is an upstream scope recommendation, not a repository requirement. Prefer proposing focused regression tests with appropriate CI coverage when the maintainers accept that scope. Retain existing author attribution when preparing the dark-mode contribution; co-author trailers should reflect actual joint authorship, not be added mechanically.

## Claude review (verbatim)
**Status: not ready for upstream yet.** I found 6 confirmed dark-mode defects (one also breaks PHP highlighting in Light mode) and 3 in the Markdown preview (one is a Windows network-access hole), plus several items to verify by running. I didn't edit, build or run anything. There's no Qt source on this machine, so anything that depends on Qt internals is labelled as such.

## Confirmed defects: dark mode (follow-ups to #1040)

**1. PHP blocks lose their highlighting in both themes.** `src/scripts/init.lua:113-115`, `:161-164`
- `SetStyle` now starts with `StyleClearAll()`, and `SetLanguage` calls it again for each entry in `additionalLanguages`.
- PHP adds HTML (`src/languages/php.lua:91`). The HTML pass wipes PHP's styles 18, 104 and 118+, which `html.lua` doesn't define.
- **Trigger:** open any `.php` file. PHP code shows as plain default text.
- The Lua test stubs out `StyleClearAll` (`tests/EditorFeatureTests.cpp:39`), so it can't catch this.
- **Fix:** set `STYLE_DEFAULT` and call `StyleClearAll` once in `SetLanguage`, before the first `SetStyle`.

**2. Switching from Dark to Follow system pins a light palette.** `src/dialogs/MainWindow.cpp:1888`
- `setPalette(style()->standardPalette())` marks every colour as explicitly set, so Qt uses it instead of the OS palette. On Windows that standard palette is light.
- **Trigger:** OS in dark mode, Theme = Dark (the fork default), then pick Follow system. You get light widgets, a dark editor and dark CSS. Later OS theme switches no longer update the widgets.
- **Fix:** `QApplication::setPalette(QPalette())`. An empty palette hands control back to the OS palette. Verify on Qt 6.5 and 6.8.

**3. Lua console dark colours are written as RGB, but Scintilla expects BGR.** `src/docks/LuaConsoleDock.cpp:188`, `:198`, `:405-428`
- The error colour `0xFF6B6B` shows as blue (#6B6BFF), not red.
- Strings `0xCE9178` show blue, keywords `0xD7BA7D` light blue, labels `0x4FC1FF` orange.
- The light branch and `EditorManager.cpp:391` already use BGR correctly.

**4. Search results are unreadable in dark mode.** `src/docks/SearchResultsDock.cpp:136`
- Line-number cells get a fixed #DCDCDC background but the dark palette's #D4D4D4 text. That's about 1.1:1 contrast.
- The headers at `:102-103` and `:121-122` are bright pastel bars.

**5. Printing in dark mode prints a dark page.** `src/EditorPrintPreviewRenderer.cpp:64`
- `formatRange` uses Scintilla's default print mode, which prints screen colours, and nothing sets a print colour mode.
- Copy as HTML/RTF has the same problem: `HtmlConverter.cpp:41` exports the dark background.
- **Fix:** print and export with the light styles, or at least use `SC_PRINT_BLACKONWHITE` for printing.

**6. In explicit Light mode, disabled labels look enabled.** `MainWindow.cpp:1917-1930`
- `setColor(WindowText, black)` sets that colour for every state, including disabled.
- The dark branch resets the disabled colour at `:1915`; the light branch doesn't.

## Confirmed defects: Markdown preview

**7. The network-share (UNC) block can be bypassed, so an untrusted README can trigger an SMB connection.** `src/widgets/MarkdownPreviewBrowser.cpp:36`
- `file:////host/share/x.png` (four slashes) parses with an empty host, so it passes the check.
- `toLocalFile()` then returns `//host/share/x.png` and `QImage` opens it. On Windows that's an outbound SMB/NTLM handshake just from showing the preview.
- **Likely (from Qt source, not checkable here):** when the override returns `{}`, `QTextDocument::loadResource` falls back to reading the file itself. That means even `file://host/share/x.png` probably gets loaded.
- `followLink` (`:50-52`) has the same four-slash gap: `isFile()` touches the share on click.
- The test (`EditorFeatureTests.cpp:77`) calls the override directly, so it can't see either path.
- **Fix:**
  - Check the final local path and reject anything starting with `//` or `\\`.
  - For blocked resources, return a non-null placeholder such as `QImage()` instead of `{}`, so Qt doesn't fall back.

**8. The Ctrl+Alt+M shortcut can't be remapped.** `src/docks/MarkdownPreviewDock.cpp:14-15`
- `applyCustomShortcuts()` runs at `MainWindow.cpp:115`, before the dock exists, and only searches direct children (`:1049`). It never finds `actionMarkdownPreview`.
- This matters because on a German Windows layout, AltGr+M (µ) arrives as Ctrl+Alt+M. The shortcut likely swallows µ; check on a DE layout.

**9. A language change on any tab re-renders the preview.** `MainWindow.cpp:1970`
- Every `setLanguage` call, including background tabs during session restore, triggers a full synchronous render.
- **Fix:** `ScintillaNext::lexerChanged` already exists. Connect to it in `watchEditor` and drop the MainWindow hook.

## Likely, needs verification
- **Memory leak:** each refresh creates a new `QTextDocument` parented to the browser (`MarkdownPreviewBrowser.cpp:23-27`). I believe `setDocument` only deletes documents owned by its internal control, so one leaks per typing pause. Check with `findChildren<QTextDocument*>().size()` after two renders.
- **Unsaved documents:** with an empty base URL, Qt's fallback loader resolves relative images against the process's working directory.
- **Qt 6.5 on Windows** (release builds use 6.5, `build.yml:13`): in Follow system mode on a dark OS, the native style may stay light while the OS reports dark. That would give a dark editor inside light chrome.
- **Scroll position:** the restore at `MarkdownPreviewBrowser.cpp:29` may be cut short on long documents.
- **Size limit:** the 2 MiB cap (`MarkdownPreviewDock.cpp:53`) runs `setMarkdown` on the UI thread after each 250 ms pause. Measure it; the cap is probably too high.
- **Dark preview rendering:** check contrast of table borders and horizontal rules. `#section` links probably do nothing, because Qt doesn't create heading anchors.
- **Explicit Dark on a light OS:** colours the palette doesn't set (e.g. placeholder text) come from the light OS palette. The Quick Find placeholder may be hard to read.

## Checked and fine
- `Update::Text` exists in the bundled Scintilla and fires only on text insert/delete (`Editor.cxx:2805`).
- In dark mode the brace-highlight style matches normal text, but BraceMatch draws with indicators instead (`BraceMatch.cpp:36-40`).
- When the theme changes, the editor's clear-styles handler (connected at `NotepadNextApplication.cpp:128`) runs before the language styles are re-applied (`:156`). The result is correct but depends on connection order.
- Going Light → Dark → Light leaves no dark leftovers: colours the code doesn't set fall back to the OS palette.
- The theme combo box and the setting don't trigger each other in a loop.
- Remote images are never fetched.
- The preview dock's position is restored: it's created with an object name before `restoreWindowState()`.
- Everything used is available in Qt 6.5. All new lambdas have a context object, so they disconnect when it's destroyed.
- `effectiveDarkModeChanged` fires on every theme change, even when light/dark doesn't flip. That's load-bearing: MainWindow needs it to swap styles between Dark and Follow system. Don't de-duplicate it without separating that case.

## Upstream acceptance

**Dark mode, on top of #1040:**
- The upstream default should be Follow system (`ApplicationSettings.cpp:99`). Keep Dark as the default only in a fork-only commit.
- On Qt 6.8+, use `QStyleHints::setColorScheme()`. It keeps native styles and darkens every window's title bar, which also replaces the DWM call. Keep Fusion plus a hand-built palette as the fallback for 6.5–6.7.
- If the DWM call stays, use the named constant `DWMWA_USE_IMMERSIVE_DARK_MODE` instead of `20`. Dialogs and floating docks still get light title bars.
- On a theme change, `NotepadNextApplication.cpp:532` re-runs the full `setEditorLanguage` for every editor. Re-applying styles is enough; merge the two theme-change handlers into one.
- Upstream writes `[=, this]`, and implicit `this` capture is deprecated in C++20. Fix at `EditorManager.cpp:143`, `LuaConsoleDock.cpp:195`, `PreferencesDialog.cpp:51`.
- The fold-marker setup is duplicated: `EditorManager.cpp:265-269` and `:388-392`.
- In the theme combo box, store the enum value as item data instead of casting the index.

**Markdown PR:**
- Add GPL headers to the four new files.
- List them alphabetically in `src/CMakeLists.txt`, alongside the other docks (`:153`) and widgets (`:177`).
- Follow upstream brace style: no one-line `if`/`else`.
- Show the link target on hover before opening it in the browser.
- Upstream has no tests and CI never turns them on. Either leave `tests/` fork-only or propose it later as its own small PR with a CI step. If you keep the tests, exercise `document()->resource()` and the dock itself, not the protected override.
- `FORK.md` stays in the fork.

## Suggested split
1. **Dark mode → onto #1040.** Leave Andrei Shevchenko's commits untouched and put ours on top as separate commits, either as a PR to that branch or as review suggestions. It contains fixes 1–6, the Follow-system default and the `setColorScheme` path. Add `Co-authored-by` wherever we amend their hunks.
2. **Markdown preview → standalone PR from `master`.** It contains the dock, the browser widget, the CMake and View-menu lines, and fixes 7–9. It doesn't depend on (1): the preview never reads theme settings.
3. **Fork-only:** `FORK.md`, the Dark default, and `tests/`.

`e2fefbd` mixes dark-mode and Markdown changes, including in `MainWindow.cpp`, so it has to be split hunk by hunk first.

## Acceptance checklist
- [ ] `.php` files highlight PHP blocks in Light and Dark.
- [ ] Dark OS: Dark → Follow system gives dark widgets, and they keep following later OS theme changes.
- [ ] Light OS: Follow system → Dark → Light → Follow system leaves no leftovers and restores the native style.
- [ ] Tested on Qt 6.5 and 6.8+ on Windows, plus Linux and macOS, in all three modes.
- [ ] Search results, Lua console, Quick Find and Preferences are readable in Dark.
- [ ] Print and Copy as HTML/RTF give a white background in Dark.
- [ ] Existing users with no saved theme setting see no change.
- [ ] Previewing both `file:////host/…` and `file://host/…` causes no SMB traffic.
- [ ] Memory stays flat during long editing with the preview open.
- [ ] The preview shortcut is remappable and doesn't swallow AltGr characters.
- [ ] Restoring a session with 50 files and the preview open stays responsive; typing in a 1–2 MiB `.md` is acceptable.
- [ ] CI passes on the full matrix, and the translation update job picks up the new strings with no hand-edited `.ts` files.

