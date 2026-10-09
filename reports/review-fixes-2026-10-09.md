# Review follow-up — 2026-10-09

Implemented and deployed on `personal/enhancements`. The [original Claude review](https://github.com/wolframs/NotepadNext/blob/d3ba33f/reports/upstream-review-2026-10-09.md)
is retained in Git history; this report supersedes its unfixed findings and checklist.

The feature fixes are `27852a7` (theme), `9937d78` (preview), and `d3ba33f` (tests/CI/docs).
They are synced to the personal GitHub fork and private Forgejo mirror. The existing local-build
Start menu shortcut launches the updated package. No OS theme or global configuration was changed.

## Findings and disposition

| Finding | Result |
| --- | --- |
| PHP styles erased by the additional HTML language | Fixed: reset style slots once per language, then apply PHP and HTML. Regression test loads both real language definitions and simulates destructive clearing. |
| Follow system pins the light palette | Fixed: restore the native style and release the palette override with an empty palette. Qt 6.8+ also sets/unsets the application colour-scheme hint. |
| Lua console RGB/BGR mismatch | Fixed, including error text and syntax colours. |
| Search-results contrast | Fixed: query headers and line numbers use palette brushes; existing results update when the tree palette changes. |
| Dark printing | Fixed: use black-on-white for a dark canvas and restore the prior Scintilla print mode afterward. Light-theme printing retains its prior mode. PDF generation exercised without contacting a printer. |
| Disabled light labels / dark placeholder text | Fixed with explicit palette entries. |
| UNC image/link bypass | Fixed: validate the final local path, not just the URL authority. Override document resource loading and image painting, so Qt cannot use either filename fallback. |
| Shortcut registration / AltGr collision | Fixed: apply custom shortcuts after dock creation and discover descendant actions. Default is Ctrl+Shift+M; an empty shortcut list disables it. |
| Background-tab language changes re-render preview | Fixed: watch only the active editor's lexer/rename/text signals, disconnecting all three on hide/switch. |
| Preview document leak | Confirmed in Qt ownership rules and fixed by deleting the replaced document safely. Twenty refreshes leave one document. |
| Relative images in untitled buffers read the working directory | Confirmed by a renderer-level regression test and fixed. |
| Scroll restoration / heading anchors | Layout completes before restoring scroll. Headings receive unique anchors, preserving inline formatting. |
| Full language reload on theme change | Fixed: one application handler updates colours and reapplies styles without recreating lexers or resetting tab settings. Lua's active-editor reference is restored. |
| Code conventions | Added GPL headers; alphabetized new source entries; expanded terse dock conditionals; explicit `this` captures and enum combo item data. |
| Tests absent from CI | Existing platform/Qt matrix now enables and runs the optional tests. Windows test executable has console output for useful failure logs. |

## Recommendations not adopted blindly

HTML export already intentionally preserves the editor's style colours. RTF writes foreground colours, but does not write a background; the review's claim that it exports a dark page is incorrect. Forcing both formats to white would change existing styled-export semantics. A separate light-export option is a product choice, not part of this fix.

Fusion remains the explicit-theme style, including on Qt 6.8+, because a colour-scheme hint is not honoured by every platform/style. The native style is restored for Follow system. Native-only explicit themes need platform-specific validation before replacing this fallback. The Windows DWM fallback for older Qt now uses the named SDK constant.

Dark remains the **personal fork's** default. An upstream dark-mode proposal should use Follow system and preserve the original PR #1040 authorship. No upstream PR or review comment was submitted, and no published history was rewritten. The fixes are committed separately by feature; the original mixed feature commit still needs extracting onto upstream-based PR branches when submitting.

## Validation and limits

Windows 11, MSVC 2022, Qt 6.8.3 Release build: feature tests passed, including combined PHP/HTML styling, resource blocking through `QTextDocument::resource`, missing/untitled images, relative-image folder changes, document lifetime, scroll restoration, heading anchors, local links, and settings.

The isolated full-application smoke check passed: shortcut override, unsaved edits, hidden preview reopening, background-tab lexer changes, Dark/Light/System transitions, disabled/placeholder colours, existing search-result recolouring, preserved custom tab width, and PDF printing with print-mode restoration. It uses temporary settings, an independent application identity, and an offscreen platform. It does not touch the user's editor session or OS theme.

Measured paragraph-heavy parser/layout times varied across runs: about 38–94 ms at 64 KiB, 130–256 ms at 256 KiB, 574–780 ms at 1 MiB, and 1.25–1.71 s at 2 MiB. The live-preview cap is now 256 KiB. These measurements are not a worst-case bound for adversarial Markdown or large local images.

Native OS dark/light transitions, Qt 6.5 builds, Linux/macOS execution, floating-window title bars, and the full CI matrix were not exercised locally. The new CI configuration provides coverage when run; this report does not claim those jobs have passed.

Qt source checks: [document fallback](https://github.com/qt/qtbase/blob/v6.8.3/src/gui/text/qtextdocument.cpp), [image fallback](https://github.com/qt/qtbase/blob/v6.8.3/src/gui/text/qtextimagehandler.cpp), [document ownership](https://github.com/qt/qtbase/blob/v6.8.3/src/widgets/widgets/qwidgettextcontrol.cpp), and [application colour-scheme API](https://doc.qt.io/qt-6/qstylehints.html#colorScheme-prop).
