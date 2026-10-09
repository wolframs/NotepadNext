# Personal Notepad Next build

This fork tracks [dail8859/NotepadNext](https://github.com/dail8859/NotepadNext).
The first feature pass adds dark mode and a live Markdown preview. Plugin support is future work.

## Dark mode

Settings → Preferences → GUI → Theme offers Dark, Light, and Follow system.
Dark is this fork's default when no theme preference has been saved. Theme changes apply immediately.

The implementation starts from [upstream PR #1040](https://github.com/dail8859/NotepadNext/pull/1040),
with the original author attribution preserved. This fork resolves the conflicts with
newer upstream code, uses Fusion for explicit theme overrides, restores the native style on returning
to Follow system, darkens the Windows title bar, improves disabled-label visibility, and lifts dark
syntax foregrounds for readability against the dark editor canvas.

## Markdown preview

View → Markdown Preview, or **Ctrl+Shift+M**, opens a dockable preview beside the editor.
It follows the active Markdown tab and updates after a short pause in typing, including unsaved edits.
The dock's visibility and position are saved with the other window layout settings.

Qt's Markdown renderer supports headings, emphasis, links, lists, tables, and fenced code blocks.
Local images resolve relative to the document; clicking a local file link opens it in the editor.
Web links open in the default browser only when clicked. Remote images are not fetched.
Preview is limited to documents up to 256 KiB to keep the editor responsive.
Relative images require a saved document path. UNC URLs are blocked, including hostless spellings.
Heading links use stable anchors; link targets appear on hover. The shortcut can be overridden
through the existing `Shortcuts/MarkdownPreview` setting (an empty list disables it).

This is a text-document renderer, not a web browser. It does not execute scripts, render Mermaid,
provide browser-equivalent HTML/CSS, or syntax-highlight fenced code blocks.
There is no WebEngine/Electron dependency, no Notepad++ DLL plugin compatibility, and no new plugin API.

## Building and tests

Follow [the upstream build instructions](doc/Building.md), using Qt 6.5 or newer with `qt5compat`.
For a Windows Release build from an MSVC developer shell, with Qt on PATH:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/path/to/Qt
cmake --build build --parallel 2
cmake --build build --target package
```

To build the optional feature tests, add `-DNOTEPADNEXT_BUILD_TESTS=ON` when configuring.
They require Qt Test, supplied with the Qt desktop libraries. Run:

```powershell
ctest --test-dir build --output-on-failure
```

Tests cover Markdown structure and content refresh, relative image resolution across document folders,
network-resource blocking through the document renderer, local links and heading anchors,
document lifetime and scroll position, theme persistence and signals, and combined PHP/HTML styles.
The build workflow enables these tests across its Qt/platform matrix.

## Current status and next session

The deployed feature branch is `personal/enhancements`. Windows 11 / Qt 6.8.3 validation
is recorded in [the review follow-up](reports/review-fixes-2026-10-09.md), including decisions
that differed from the original static review. Cross-platform CI results are not yet confirmed.

Before submitting upstream, extract Markdown preview onto a branch from upstream `master`
and keep the dark-mode contribution separate, preserving PR #1040's authorship. Follow system
should be the upstream default; Dark is a personal-fork preference. The original feature commit
mixes both features, while the subsequent fixes are separated by feature.

Plugins remain future work. No upstream PR has been submitted.
