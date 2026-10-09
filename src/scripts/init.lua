function rgb(x)
    return ((x & 0xFF) << 16) | (x & 0xFF00) | ((x & 0xFF0000) >> 16)
end

local STYLE_DEFAULT = 32

-- Build the active theme table from the dark_mode global injected by C++.
-- Also called by NotepadNextApplication::refreshEditorTheme() on toggle.
function UpdateTheme()
    if dark_mode then
        theme = {
            default_fg = rgb(0xD4D4D4),
            default_bg = rgb(0x1E1E1E),
            light_fg   = rgb(0x000000),
            light_bg   = rgb(0xFFFFFF),
        }
    else
        theme = {
            default_fg = rgb(0x000000),
            default_bg = rgb(0xFFFFFF),
            light_fg   = rgb(0x000000),
            light_bg   = rgb(0xFFFFFF),
        }
    end
end

UpdateTheme()

-- Light-language syntax colors need enough contrast against the dark canvas.
-- Scintilla stores colors as BGR. Lift only foregrounds that fail 4.5:1.
function ThemeBackground(color)
    if not dark_mode or color == nil then return color end
    if color == theme.light_bg then return theme.default_bg end
    local r, g, b = color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF
    if (0.2126*r + 0.7152*g + 0.0722*b) > 160 then
        r, g, b = math.floor(20+r*0.08), math.floor(20+g*0.08), math.floor(20+b*0.08)
        return r | (g << 8) | (b << 16)
    end
    return color
end

function ThemeForeground(color, backgroundColor)
    if not dark_mode or color == nil then return color end
    if color == theme.light_fg then return theme.default_fg end
    local r, g, b = color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF
    local function linear(c)
        c = c / 255
        if c <= 0.04045 then return c / 12.92 end
        return ((c + 0.055) / 1.055) ^ 2.4
    end
    local bg = backgroundColor or theme.default_bg
    local background = 0.2126*linear(bg & 0xFF) + 0.7152*linear((bg >> 8) & 0xFF) + 0.0722*linear((bg >> 16) & 0xFF)
    for step = 0, 10 do
        local mix = step / 10
        local rr, gg, bb = math.floor(r + (255-r)*mix), math.floor(g + (255-g)*mix), math.floor(b + (255-b)*mix)
        local light = 0.2126*linear(rr) + 0.7152*linear(gg) + 0.0722*linear(bb)
        local contrast = (math.max(light, background)+0.05) / (math.min(light, background)+0.05)
        if contrast >= 4.5 then return rr | (gg << 8) | (bb << 16) end
    end
    return theme.default_fg
end

function DetectLanguageFromContents(contents)
    for name, L in pairs(languages) do
        if L.first_line then
            for _, pattern in ipairs(L.first_line) do
                if string.match(contents, pattern) then
                    return name
                end
            end
        end
    end
    return "Text"
end

function FilterForLanguage(name)
    local extensions = {}
    local language_definition = languages[name]

    if not language_definition.extensions then
        return nil
    end

    for _, ext in ipairs(language_definition.extensions) do
        if #ext > 0 then
            extensions[#extensions + 1] = "*." .. ext
        end
    end

    return  name .. " Files (" .. table.concat(extensions, " ") .. ")"
end

function DialogFilters()
    local filters = {}

    for name, L in pairs(languages) do
        local filter = FilterForLanguage(name)
        if filter then
            filters[#filters + 1] = filter
        end
    end

    table.sort(filters, function (a, b) return a:lower() < b:lower() end)
    table.insert(filters, 1, "All Files (*)")

    return table.concat(filters, ";;")
end

function SetStyle(L)
    -- Apply theme base: STYLE_DEFAULT sets the canvas for styleClearAll().
    -- This ensures every style slot starts with the correct background color
    -- even for styles not explicitly listed in the language definition.
    editor.StyleFore[STYLE_DEFAULT] = theme.default_fg
    editor.StyleBack[STYLE_DEFAULT] = theme.default_bg
    editor:StyleClearAll()

    if L.styles then
        for _, style in pairs(L.styles) do
            local bg = ThemeBackground(style.bgColor)
            local fg = ThemeForeground(style.fgColor, bg)
            editor.StyleFore[style.id] = fg
            editor.StyleBack[style.id] = bg

            if style.fontStyle then
                editor.StyleBold[style.id] = (style.fontStyle & 1 == 1)
                editor.StyleItalic[style.id] = (style.fontStyle & 2 == 2)
                editor.StyleUnderline[style.id] = (style.fontStyle & 4 == 4)
                editor.StyleEOLFilled[style.id] = (style.fontStyle & 8 == 8)
            end
        end
    end

    if L.keywords then
        for id, kw in pairs(L.keywords) do
            editor.KeyWords[id] = kw
        end
    end

    if L.properties then
        for p, v in pairs(L.properties) do
            editor.Property[p] = v
        end
    end
end

function SetLanguage(languageName)
    local L = languages[languageName]

    if not skip_tabs then
        editor.UseTabs = (L.tabSettings or "tabs") == "tabs"
    end

    if not skip_tabwidth then
        editor.TabWidth = L.tabSize or 4
    end

    editor.MarginWidthN[2] = L.disableFoldMargin and 0 or 16

    SetStyle(L)

    if L.additionalLanguages then
        for _, language in pairs(L.additionalLanguages) do
            SetStyle(languages[language])
        end
    end


    editor.Property["fold"] = "1"
    editor.Property["fold.compact"] = "0"
end

function GetLanguageKeywords(languageName)
    local L = languages[languageName]

    if not L or not L.keywords then
        return {}
    end

    local seen = {}

    local function collectKeywords(lang)
        if not lang then return end
        if lang.keywords then
            for _, kwString in pairs(lang.keywords) do
                for token in kwString:gmatch("%S+") do
                    seen[token] = true
                end
            end
        end
    end

    collectKeywords(L)

    if L.additionalLanguages then
        for _, language in pairs(L.additionalLanguages) do
            collectKeywords(languages[language])
        end
    end

    local result = {}
    for token, _ in pairs(seen) do
        result[#result + 1] = token
    end

    table.sort(result)

    return result
end

languages = {}
languages["ActionScript"] = require("actionscript")
languages["ADA"] = require("ada")
languages["Assembly"] = require("asm")
languages["ASN.1"] = require("asn1")
languages["asp"] = require("asp")
languages["autoIt"] = require("autoit")
languages["AviSynth"] = require("avs")
languages["BaanC"] = require("baanc")
languages["bash"] = require("bash")
languages["Batch"] = require("batch")
languages["BlitzBasic"] = require("blitzbasic")
languages["C"] = require("c")
languages["Caml"] = require("caml")
languages["CMakeFile"] = require("cmake")
languages["COBOL"] = require("cobol")
languages["Csound"] = require("csound")
languages["CoffeeScript"] = require("coffeescript")
languages["C++"] = require("cpp")
languages["C#"] = require("cs")
languages["CSS"] = require("css")
languages["SCSS"] = require("scss")
languages["D"] = require("d")
languages["DIFF"] = require("diff")
languages["Erlang"] = require("erlang")
languages["ESCRIPT"] = require("escript")
languages["Forth"] = require("forth")
languages["Fortran (free form)"] = require("fortran")
languages["Fortran (fixed form)"] = require("fortran77")
languages["FreeBasic"] = require("freebasic")
languages["GUI4CLI"] = require("gui4cli")
languages["Go"] = require("go")
languages["Haskell"] = require("haskell")
languages["HTML"] = require("html")
languages["ini file"] = require("ini")
languages["InnoSetup"] = require("inno")
languages["Intel HEX"] = require("ihex")
languages["Java"] = require("java")
languages["JavaScript"] = require("javascript")
languages["JSON"] = require("json")
languages["KiXtart"] = require("kix")
languages["LISP"] = require("lisp")
languages["LaTeX"] = require("latex")
languages["Lua"] = require("lua")
languages["Less"] = require("less")
languages["Makefile"] = require("makefile")
languages["Markdown"] = require("markdown")
languages["Matlab"] = require("matlab")
languages["MMIXAL"] = require("mmixal")
languages["Nimrod"] = require("nimrod")
languages["Nix"] = require("nix")
languages["extended crontab"] = require("nncrontab")
languages["Dos Style"] = require("nfo")
languages["NSIS"] = require("nsis")
languages["OScript"] = require("oscript")
languages["Objective-C"] = require("objc")
languages["Pascal"] = require("pascal")
languages["Perl"] = require("perl")
languages["PHP"] = require("php")
languages["Postscript"] = require("postscript")
languages["PowerShell"] = require("powershell")
languages["Properties file"] = require("props")
languages["PureBasic"] = require("purebasic")
languages["Python"] = require("python")
languages["R"] = require("r")
languages["REBOL"] = require("rebol")
languages["registry"] = require("registry")
languages["RC"] = require("rc")
languages["Ruby"] = require("ruby")
languages["Rust"] = require("rust")
languages["Scheme"] = require("scheme")
languages["Smalltalk"] = require("smalltalk")
languages["spice"] = require("spice")
languages["SQL"] = require("sql")
languages["S-Record"] = require("srec")
languages["Swift"] = require("swift")
languages["TCL"] = require("tcl")
languages["Tektronix extended HEX"] = require("tehex")
languages["TeX"] = require("tex")
languages["Text"] = require("text")
languages["VB / VBS"] = require("vb")
languages["txt2tags"] = require("txt2tags")
languages["Verilog"] = require("verilog")
languages["VHDL"] = require("vhdl")
languages["Visual Prolog"] = require("visualprolog")
languages["XML"] = require("xml")
languages["YAML"] = require("yaml")
languages["Abaqus"] = require("abaqus")
