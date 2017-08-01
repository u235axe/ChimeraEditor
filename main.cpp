#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <future>
#include <atomic>
#include <mutex>
#include <functional>
#include <iostream>

using namespace std;
#define NOMINMAX
#include <windows.h>
#include <Windowsx.h>

template<typename T> struct V2 { T x, y; };
auto v2(int x, int y){ return V2<int>{x, y}; }

struct Style{ COLORREF color; };

struct Clipboard
{
    HWND hwnd;
    bool copy( std::wstring const& data_in )
    {
        auto data = data_in + L"x";
        data[data.size()-1] = L'\0';
        if( !OpenClipboard(hwnd) ){ return false; } 
        EmptyClipboard();

        auto sz = sizeof(wchar_t)*data.size();
        auto h = GlobalAlloc(GMEM_MOVEABLE, sz);
        if(!h){ CloseClipboard(); return false; }
        auto hh = GlobalLock(h);
        memcpy_s(hh, sz, data.data(), sz);
        GlobalUnlock(hh);
        SetClipboardData(CF_UNICODETEXT, h);
        CloseClipboard(); 
        return true;
    }

    auto paste()
    {
        std::wstring data;
        if( !OpenClipboard(hwnd) ){ return data; } 
        if( !IsClipboardFormatAvailable(CF_UNICODETEXT)){ return data; }
        auto h = GetClipboardData(CF_UNICODETEXT); 
        auto hh = GlobalLock(h);
        auto sz = GlobalSize(h);
        data.resize(sz / sizeof(wchar_t));
        memcpy_s(&data[0], sz, hh, sz);
        GlobalUnlock(hh);
        CloseClipboard();

        if(data.size() > 0 && data[data.size()-1] == L'\0'){ data.erase(--data.end()); }

        return data;
    }
};

struct Window
{
    HWND    hwnd;
    HDC     wdc;
    HBITMAP bb;
    HDC     bdc;
    Clipboard   clipboard;
    int w, h;

    int         font_size;
    HFONT       hFont;

    Window(){}
    
    bool create(HINSTANCE hinst, WNDPROC wp, int sh)
    {
        WNDCLASSEX wcex;

        wcex.cbClsExtra = 0;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.cbWndExtra = 0;
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hInstance = hinst;
        wcex.lpfnWndProc = wp;
        wcex.lpszClassName = L"RapidCodeWndClass";
        wcex.lpszMenuName = NULL;
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        if (!RegisterClassEx(&wcex))
        {
            MessageBox(NULL, TEXT("RegisterClassEx Failed!"), TEXT("Error"), MB_ICONERROR);
            return false;
        }

        if (!(hwnd = CreateWindow(wcex.lpszClassName, L"Rapid Code", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinst, NULL)))
        {
            MessageBox(NULL, TEXT("CreateWindow Failed!"), TEXT("Error"), MB_ICONERROR);
            return false;
        }

        wdc = GetDC(hwnd);
        bb = nullptr;
        bdc = nullptr;
        w = 0; h = 0;
        font_size = 22;
        hFont = CreateFont(font_size,0,0,0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Consolas"));
        SelectObject(wdc, hFont);

        SetBkMode(wdc, TRANSPARENT);
        clipboard.hwnd = hwnd;

        ShowWindow(hwnd, sh);
        UpdateWindow(hwnd);
        return true;
    }

    ~Window()
    {
        DestroyWindow(hwnd);
    }

    void present()
    {
        PAINTSTRUCT ps;
        auto dc = BeginPaint(hwnd, &ps);
        auto res = BitBlt(wdc, 0, 0, w, h, bdc, 0, 0, SRCCOPY) == TRUE;
        EndPaint(hwnd, &ps);
    }

    void Resize(int w_in, int h_in)
    {
        if(bdc){ DeleteDC(bdc); }
        if(bb){ DeleteObject(bb); }
        w = w_in; h = h_in;
        bb = CreateCompatibleBitmap(wdc, w, h);
        bdc = CreateCompatibleDC(wdc);
        SelectObject(bdc, bb);
        SelectObject(bdc, hFont);
        SetBkMode(bdc, TRANSPARENT);
    }

    void drawText(int x, int y, std::wstring const& text, Style const& style)
    {
        SetTextColor(bdc, style.color);
        TextOut(bdc, x, y, text.c_str(), (int)text.size());
    }

    void drawRect(int x, int y, int w, int h, Style const& style )
    {
        SetDCBrushColor(bdc, style.color);
        RECT rct; rct.left = x; rct.right = x+w; rct.top = y; rct.bottom = y+h;
        FrameRect(bdc, &rct, (HBRUSH)GetStockObject(DC_BRUSH));
    }

    void fillRect(int x, int y, int w, int h, Style const& style)
    {
        RECT rect;
        rect.left = x;
        rect.top = y;
        rect.right = x + w;
        rect.bottom = y + h;
        SetDCBrushColor(bdc, style.color);
        FillRect(bdc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    }

    void drawLine(int x0, int y0, int x1, int y1, Style const& style )
    {
        SelectObject(bdc, GetStockObject(DC_PEN));
        SetDCPenColor(bdc, style.color);
        MoveToEx(bdc, x0, y0, 0);
        LineTo(bdc, x1, y1);
    }

    V2<int> measureString( std::wstring const& text )
    {
        SIZE sz;
        GetTextExtentPoint32(bdc, text.c_str(), (int)text.length(), &sz);
        return {(int)sz.cx, (int)sz.cy};
    }
};

static const std::vector<std::wstring> cppkeyws =
{
    L"alignas", L"alignof",
    L"asm",
    L"bool", L"true", L"false",
    L"short", L"int", L"long", L"float", L"double", L"char", L"char16_t", L"char32_t", L"signed", L"unsigned", L"wchar_t",
    L"auto", L"const", L"constexpr",  L"sizeof",
    L"void", L"decltype", L"using", L"template", L"typename", L"static_assert", L"typedef", L"typeid",
    L"break", L"default", L"case", L"continue", L"do", L"else", L"if", L"for", L"goto", L"return", L"while", L"switch",
    L"try", L"catch", L"noexcept", L"throw",
    L"class", L"struct", L"union", L"enum", L"friend", L"namespace", L"private", L"public", L"protected", L"this", L"virtual"
    L"const_cast", L"static_cast", L"dynamic_cast", L"reinterpret_cast", L"explicit", L"operator", L"override", L"final",
    L"new", L"delete", L"nullptr",
    L"extern", L"static", L"inline", L"mutable", L"thread_local", L"volatile"
};

static const std::wstring nums = L"0123456789";
static const std::wstring cppoperator = L"+/*-<>{}()[]?!#$%^&|:.,;=";
static const std::wstring wssep = L" \t";
static const std::wstring strsep = L"\"'";
static const std::wstring cppoperatorws = L" \t+/*-<>{}()[]?!#$%^&|:.,;=";
static const std::wstring allnonid = L" \t'\"+/*-<>{}()[]?!#$%^&|:.,;=";



enum SyntaxGroup : short { WhiteSpace, Operator, Identifier, Number, Text, Include, Preprocessor, Comment, Keyword };

Style syntax_style(SyntaxGroup const& sg);

struct Entry
{
    size_t p0, p1;
    SyntaxGroup g;
    auto style() const { return syntax_style(g); }
};

struct Line
{
    std::wstring text;
    std::vector<Entry> entries;
    bool is_multiline_comment;

    int length() const { return (int)text.size(); }
    auto& str() const { return text; }
    auto set( std::wstring const& s ){ text = s; entries.clear(); }
    auto sub(size_t const& p0, size_t const& p1)const{ return text.substr(p0, p1-p0); }
    auto sub_from(size_t const& p0)const{ return text.substr(p0, text.npos); }
    auto sub_to  (size_t const& p1)const{ return text.substr(0, p1); }
    auto reduce_initial_ws()
    {
        if(text.size() > 0 && entries.size() > 0 && entries[0].g == SyntaxGroup::WhiteSpace)
        {
            text.erase(0, 1);
        }
    }
};

enum LogType : size_t { Note, Warning, Error };
struct Log
{
    LogType ty;
    size_t line, col0, col1;
};

std::wstring wsl_current_working_dir()
{
    wchar_t tmp[1];
    auto sz = GetCurrentDirectoryW(1, tmp);
    wchar_t* t = new wchar_t[sz];
    GetCurrentDirectoryW(sz, t);
    std::wstring res(t);
    delete[] t;

    res[1] = towlower(res[0]);
    res[0] = L'/';
    res.insert(0, L"/mnt");
    std::replace(res.begin(), res.end(), L'\\', L'/');
    return res;
}

struct Compiler
{
    std::wstring name;
    std::wstring env, env_arch;
    std::wstring cmd_chain;
    std::wstring compiler, flags;
    std::wstring fn_suffix;
    std::wstring out_prefix;
    std::wstring fwd_prefix;
    std::wstring diag_sep;
    std::wstring cmd_terminator;
    bool in_wsl;

    std::wstring get_cmdline( std::wstring const& fn, std::wstring const& outfn ) const
    {
        auto res = env;
        if(in_wsl && env == L"bash.exe")
        {
#ifdef _WIN64
            res = std::wstring( L"\"bash.exe\""); 
#else
            res = std::wstring( L"\"C:\\Windows\\Sysnative\\bash.exe\"" );
#endif
            res += L" -c \"cd " + wsl_current_working_dir() + L" ";
        }
        else
        {
            res = L"\"" + env + L"\" " + env_arch + L" ";
        }

        res += cmd_chain + L" \"" + compiler + L"\" " + flags + L" " + fn + L" " + fn_suffix + L" " + out_prefix + L" " + outfn;
        //gpp.prefix = bash_prefix + L" -c \"cd " + wsl_current_working_dir()
        return res;
        //return env2 + L" " + cmd_chain + L" " + compiler + L" " + flags + L" " + fn + L" " + fn_suffix + L" " + out_prefix + L" " + outfn;
    }
};

struct CompilerLog
{
    Compiler compiler;
    std::vector<Log> lines;

    void clear(){ lines.clear(); }
    std::wstring get_cmdline( std::wstring const& fn, std::wstring const& outfn ){ return compiler.get_cmdline(fn, outfn); }
};

std::wstring get_build_log();

struct RegularSelection
{
    V2<int> pos0, pos1;
    bool active;

    auto lo() const
    {
        if(pos0.y < pos1.y){ return pos0; }
        else if(pos0.y == pos1.y)
        {
            if(pos0.x <= pos1.x){ return pos0; }
            else{ return pos1; }
        }
        else{ return pos1; }
    }

    auto hi() const
    {
        if(pos0.y < pos1.y){ return pos1; }
        else if(pos0.y == pos1.y)
        {
            if(pos0.x <= pos1.x){ return pos1; }
            else{ return pos0; }
        }
        else{ return pos0; }
    }

    bool is_single_line() const { return active && (pos0.y == pos1.y); }

    void drop(){ pos0 = pos1 = V2<int>{-1, -1}; active = false; }

    void create_at(V2<int> const& pos){ pos0 = pos1 = pos; active = true; }
    void create_at(int const& x, int const& y){ pos0 = pos1 = v2(x, y); active = true; }

    void extend(V2<int> const& pos){ pos1 = pos; }
    void extend(int const& x, int const& y){ pos1 = v2(x, y); }
};

struct File
{
    std::wstring fn;
    std::wstring fn_without_ext() const
    {
        auto p = fn.find_last_of(L'.');
        if( p == fn.npos){ return fn; }
        else
        {
            auto tmp = fn.substr(0, p);
            return tmp;
        }
    }
    std::mutex mcontent, mlog;
    std::vector<Line>         lines;  //annotations
    std::vector<CompilerLog>  logs;   //compiler logs

    void add_compiler( Compiler&& c ){ logs.push_back( CompilerLog{c, {}} ); }

    void clear_logs(){ for(auto& l : logs){ l.clear(); } }

    auto const& operator[]( int const& line ) const { return lines[line]; }
    auto &      operator[]( int const& line )       { return lines[line]; }

    auto last_line_idx() const { return (int)lines.size()-1; }

    bool load( std::wstring const& fn_in, bool create = false )
    {
        {
            std::lock_guard<std::mutex> lock(mlog);
            clear_logs();
        }

        std::wifstream file(fn_in);
        if(!file.is_open())
        {
            if( create ){ std::wofstream file2(fn_in); file2 << "\n"; file2.close(); file.open(fn_in); }
            else{ return false; }
        }

        std::lock_guard<std::mutex> lock(mcontent);
        fn = fn_in;
        lines.clear();

        Line l;
        while(std::getline(file, l.text))
        {
            l.entries.clear();
            l.is_multiline_comment = false;
            lines.push_back(l);
        }
        
        format();
        return true;
    }

    auto recreate() 
    {
        int sz = 0;
        for(auto const& l : lines){ sz += l.length(); }
        std::wstring res; res.reserve(sz);
        size_t n= 0;
        for(auto const& l : lines)
        {
            ++n;
            std::wclog << l.str();
            res = res + l.str() + L"\n";
        }
        return res;
    }
  
    auto syntax_group_of(std::wstring const& s)
    {
        
        if( nums.find(s[0]) != s.npos ){ return SyntaxGroup::Number; }

        if( s == L"struct"  ){ return SyntaxGroup::Keyword; }
        if( s == L"class"   ){ return SyntaxGroup::Keyword; }
       
        for( auto const& key : cppkeyws)
        {
            if( s == key ){ return SyntaxGroup::Keyword; }
        }
   
        if(s.find_first_of(cppoperator, 0) != s.npos){ return SyntaxGroup::Operator; }
        if(s.find_first_of(wssep, 0) != s.npos){ return SyntaxGroup::WhiteSpace; }
   
        return SyntaxGroup::Identifier;
    }

    void format_line( Line& line, bool& multilinecomment )
    {
        Line res;
        res.text = line.text;
        res.is_multiline_comment = false;

        bool is_preproc = false;
        size_t p = 0;

        auto& t = line.text;
       
        while(p != t.npos)
        {
            if(multilinecomment)
            {
                auto q = p;
                bool found = false;
                while(!found && q < t.size())
                {
                    q = t.find_first_of(L"*", q);
                    if( q == t.npos ){ break; }
                    if( t[q+1] == L'/'){ found = true; }
                    else{ q += 1; }
                }
                if( !found ){ res.entries.push_back({0, q, SyntaxGroup::Comment}); res.is_multiline_comment = true; break; }
                q += 2;
                res.entries.push_back({p, q, SyntaxGroup::Comment});
                multilinecomment = false;
                res.is_multiline_comment = false;
                p = q;
                continue;
            }

            if(wssep.find(t[p]) != wssep.npos)//if white space seek first non ws
            {
                auto q = t.find_first_not_of(wssep, p+1);
                auto s = t.substr(p, q-p);
                res.entries.push_back({p, q, SyntaxGroup::WhiteSpace});
                p = q;
                continue;
            }

            if( t[p] == L'/' && t[p+1] == L'/' )//comments
            {
                auto s = t.substr(p, t.size());
                res.entries.push_back({p, t.size(), SyntaxGroup::Comment});
                break;
            }
            else if( t[p] == L'/' && t[p+1] == L'*' )//comments
            {
                auto q = p+2;
                bool found = false;
                while(!found && q < t.size())
                {
                    q = t.find_first_of(L"*", q);
                    if( q == t.npos ){ break; }
                    if( t[q+1] == L'/'){ found = true; }
                    else{ q += 1; }
                }
                if( found )
                {
                    auto s = t.substr(p, q+2-p);
                    res.entries.push_back({p, q+2, SyntaxGroup::Comment});
                    res.is_multiline_comment = multilinecomment = false; p = q+2;
                }
                else
                {
                    auto s = t.substr(p, t.size());
                    res.entries.push_back({p, t.size(), SyntaxGroup::Comment});
                    res.is_multiline_comment = true;
                    multilinecomment = true;
                    break;
                }   
            }
            else if(cppoperator.find(t[p]) != wssep.npos)//if cppoperator step 1
            {
                auto s = t.substr(p, 1);
                if(s == L"#" || s == L"##"){ res.entries.push_back({p, p+1, SyntaxGroup::Include}); is_preproc = true; }
                else{ res.entries.push_back({p, p+1, SyntaxGroup::Operator}); }
                p += 1;
            }
            else if(strsep.find(t[p]) != wssep.npos)
            {
                bool escaped = true;
                size_t q = p+1;
                while(escaped)
                {
                    auto qq = t.find_first_of(t[p], q)+1;
                    if( qq < 2 ){ break; }
                    if( t[qq-2] != '\\' ){ escaped = false; }
                    q = qq;
                }
                auto s = t.substr(p, q-p);
                res.entries.push_back({p, q, SyntaxGroup::Text});
                p = q;
            }
            else //plain text
            {
                auto q = t.find_first_of(allnonid, p+0);
                auto s = t.substr(p, q-p);
                res.entries.push_back({p, q, syntax_group_of(s)});
                p = q;
            }
        }
    
        //update colors of preproc fields:
        if(is_preproc)
        {
            auto sz = (int)res.entries.size();
            for(int i=0; i<sz; ++i)
            {
                auto const& sub1 = res.sub(res.entries[i].p0, res.entries[i].p1);
                if(sub1 == L"include" || sub1 == L"define" || sub1 == L"undef" || sub1 == L"if" || sub1 == L"ifdef" || sub1 == L"ifndef" || sub1 == L"else" || sub1 == L"elif" || sub1 == L"endif" || sub1 == L"line" || sub1 == L"error" || sub1 == L"pragma" || sub1 == L"comment"){ res.entries[i].g = SyntaxGroup::Include; }

                if(sub1 == L"<")
                {
                    int j=i+1;
                    while(j<sz)
                    {
                        auto const& sub2 = res.sub(res.entries[j].p0, res.entries[j].p1);
                        if( sub2 == L">" ){ break; } ++j;
                    }
                    if(j<sz){ for(int k=i; k<=j; ++k){ res.entries[k].g = SyntaxGroup::Text; } }
                }
            }
        }
        line = res;
    }

    void format()
    {
        bool multilinecomment = false;
        for(auto& line : lines){ format_line(line, multilinecomment); }
    }

    void delete_selection( RegularSelection& sel )
    {
        if(sel.is_single_line())
        {
            lines[sel.pos0.y].text.erase(sel.lo().x, sel.hi().x-sel.lo().x);
        }
        else
        {
            auto left = lines[sel.lo().y].sub_to(sel.lo().x);
            auto right = lines[sel.hi().y].sub_from(sel.hi().x);
            lines[sel.lo().y].set(left+right);
            for(int k=sel.lo().y; k<sel.hi().y; ++k)
            {
                lines.erase(lines.begin()+sel.lo().y+1);
            }
        }
        format();
    }

    auto copy(Clipboard& c, RegularSelection& sel)
    {
        std::wstring data;
        if(!sel.active){ return; }
        auto lx = sel.lo().x;
        auto hx = sel.hi().x;
        auto ly = sel.lo().y;
        auto hy = sel.hi().y;
        if(sel.is_single_line())
        {
            data = lines[ly].sub(lx, hx);
        }
        else
        {
            for(int l=ly; l<=hy; ++l)
            {
                if     (l == ly){                  data += lines[l].sub_from(lx); }
                else if(l == hy){ data += L"\r\n"; data += lines[l].sub_to(hx); }
                else            { data += L"\r\n"; data += lines[l].text; }
            }
        }
        c.copy(data);
    }

    void paste(Clipboard& c, RegularSelection& sel, V2<int> const& cur)
    {
        std::vector<Line> ls;

        {
            auto data = c.paste();
            data.erase(std::remove(data.begin(), data.end(), '\r'), data.end());
            std::wstringstream wss(data);
            Line l;
            if(data[0] == L'\n' || data[0] == L'\r'){ ls.push_back(l); }
            while(std::getline(wss, l.text))
            {
                l.entries.clear();
                l.is_multiline_comment = false;
                ls.push_back(l);
            }
            if(data.size() > 0 && (data[data.size()-1] == L'\n' || data[data.size()-1] == L'\r')){ ls.push_back(l); }
        }

        if(sel.active)
        {
            auto tmp = lines[sel.hi().y].sub_from(sel.hi().x);
            delete_selection(sel);
            lines[sel.lo().y].text.insert(sel.lo().x, ls[0].text);
            
            for(int k=1; k<ls.size()-1; ++k)
            {
                lines.insert(lines.begin()+sel.lo().y+k, ls[k]);
            }

            if(!sel.is_single_line())
            {
                auto ip = ls.size()-1;
                if(ip > lines.size()-1){ ip = lines.size()-1; }
                lines.insert(lines.begin()+ip, Line{ ls[ls.size()-1].text + tmp, {}, false });
            }
        }
        else
        {
            auto left  = lines[cur.y].sub_to(cur.x);
            auto right = lines[cur.y].sub_from(cur.x);
            ls[ls.size()-1].text += right;
            left.append(ls[0].text);
            lines[cur.y].set(left);
            
            for(int k=1; k<ls.size(); ++k)
            {
                lines.insert(lines.begin()+cur.y+k, ls[k]);
            }
        }
        format();
    }
};

struct cmd_handle
{
    PROCESS_INFORMATION p_info;
    STARTUPINFO s_info;
    bool valid;
    void wait() const
    {
        if(valid)
        {
            WaitForSingleObject(p_info.hProcess, INFINITE);
            CloseHandle(p_info.hProcess);
            CloseHandle(p_info.hThread);
        }
    }
};

cmd_handle windows_system(std::wstring const& cmd)
{
    cmd_handle h;
    wchar_t* cmdline = new wchar_t[cmd.size()*2];
    wcscpy_s(cmdline, cmd.size()*2, cmd.c_str());

    memset(&h.s_info, 0, sizeof(h.s_info));
    memset(&h.p_info, 0, sizeof(h.p_info));
    h.s_info.cb = sizeof(h.s_info);

    if (CreateProcessW(NULL, cmdline, NULL, NULL, 0, CREATE_NO_WINDOW, NULL, NULL, &h.s_info, &h.p_info))
    {
        h.valid = true;
    }
    else
    {
        auto err = GetLastError();
        h.valid = false;
    }
    delete[] cmdline;
    return h;
}

struct PaintStyle
{
    Style bk   = { RGB(255, 255, 255) };//background color
    Style fn   = { RGB(64,  64,  64 ) };//filename color
    Style ln   = { RGB(0,   128, 255) };//line number color
    Style sel  = { RGB(128, 192, 255) };//selection color
    Style err  = { RGB(255, 0,   0  ) };//error color;
    Style warn = { RGB(255, 128, 0  ) };//warning color;
    Style note = { RGB(0,   0,   255) };//note color;
    Style grey = { RGB(128, 128, 128) };//grey color;
    Style cur  = { RGB(0,   0,   0  ) };//cursor color;

    Style commentcolor  = { RGB(0, 128, 0) };
    Style keycolor      = { RGB(0, 0, 255) };
    Style numcolor      = { RGB(64, 64, 64) };
    Style tycolor       = { RGB(0, 192, 64) };
    Style includecolor  = { RGB(128, 128, 255) };
    Style strcolor      = { RGB(128, 0, 0) };
    Style defaultcolor  = { RGB(0, 0, 0) };
    Style operatorcolor = { RGB(0, 0, 0) };
    Style identifier    = { RGB(0, 0, 0) };
    Style definecolor   = { RGB(64, 32, 128) };

    PaintStyle(){}

    Style& LogStyle( LogType const& lt )
    {
        if     ( lt == LogType::Error   ){ return err; }
        else if( lt == LogType::Warning ){ return warn; }
        else if( lt == LogType::Note    ){ return note; }
        return cur;
    }

    bool load_from(std::wstring const& fn)
    {
        std::wifstream file(fn);
        if(!file.is_open()){ return false; }

        PaintStyle st;

        std::wstring line;
        int r, g, b;
        while( std::getline(file, line) )
        {
            std::wistringstream ss(line);
            std::wstring field;
            std::wstring op;
            if( !(ss >> field >> op >> r >> g >> b) ){ continue; }
            /*std::getline(ss, value, L'\n');
            auto p = value.find_first_not_of(L" ");
            if(p != value.npos){ value = value.substr(p); }
            auto q = value.find_last_not_of(L" ");
            if(q != value.npos){ value = value.substr(0, q+1); }*/
            if(op != L"="){ break; }
        
            Style col = { RGB(r, g, b) };

            if     (field == L"background" ){ st.bk = col; }
            else if(field == L"filename"   ){ st.fn = col; }
            else if(field == L"line_number"){ st.ln = col; }
            else if(field == L"selection"  ){ st.sel = col; }
            else if(field == L"error"      ){ st.err = col; }
            else if(field == L"warning"    ){ st.warn = col; }
            else if(field == L"note"       ){ st.note = col; }
            else if(field == L"grey"       ){ st.grey = col; }
            else if(field == L"cursor"     ){ st.cur = col; }
            else if(field == L"comment"    ){ st.commentcolor = col; }
            else if(field == L"keyword"    ){ st.keycolor = col; }
            else if(field == L"number"     ){ st.numcolor = col; }
            else if(field == L"type"       ){ st.tycolor = col; }
            else if(field == L"include"    ){ st.includecolor = col; }
            else if(field == L"string"     ){ st.strcolor = col; }
            else if(field == L"default"    ){ st.defaultcolor = col; }
            else if(field == L"operator"   ){ st.operatorcolor = col; }
            else if(field == L"identifier" ){ st.identifier = col; }
            else if(field == L"define"     ){ st.definecolor = col; }
            
        }
        *this = st;
        return true;
    }
};

enum class TextOperation {none, caret, character, backspace, del, enter, tab, shift_tab};

struct SingleLineEdit
{
    std::wstring data;
    int x0, y0, spacing_ch, spacing_y;
    int curx, dcurx;
    RegularSelection sel;
    Window* wnd;
    TextOperation   op = TextOperation::none;
    std::function<void(void)> update_target;
    wchar_t     ch;
    bool        shifted = false;
    bool        has_focus = false;

    void set( std::wstring const& s )
    {
        data = s;
        curx = 0;
        sel.drop();
    }

    void update()
    {
        if(op == TextOperation::caret)
        {
            if(shifted && !sel.active){ sel.create_at(curx, 0); }
            if(dcurx != 0)
            {
                auto lc = (int)data.length();
                if(dcurx == -1)
                {
                    if(curx == 0){}
                    else{ curx -= 1; }
                }
                else if(dcurx == +1)
                {
                    if(curx == lc){}
                    else{ curx += 1; }
                }
                else if(dcurx == -10){ curx = 0; }
                else if(dcurx == +10){ curx = lc; }
            }
            if(!shifted){ sel.drop(); }
        }

        if(shifted && sel.active){ sel.extend(curx, 0); }

        if(sel.active && (op == TextOperation::backspace || op == TextOperation::character || op == TextOperation::del || op == TextOperation::enter) )
        {
            data.erase(sel.lo().x, sel.hi().x-sel.lo().x);
            curx = sel.lo().x;
            sel.drop();
            if(op == TextOperation::backspace){ op = TextOperation::none; }
        }

        if(op != TextOperation::none)
        {
            if(op == TextOperation::character)
            {
                data.insert(data.begin()+curx, ch);
                curx += 1;
            }
            else if(op == TextOperation::backspace && curx > 0)
            {
                data.erase(curx-1, 1);
                curx -= 1;
            }
            else if(op == TextOperation::del && curx < data.size()-1)
            {
                data.erase(curx, 1);
            }
            else if(op == TextOperation::tab){}
            else if(op == TextOperation::enter)
            {
                has_focus = false;
                sel.drop();
                update_target();
            }
        }
        op = TextOperation::none;
    }

    V2<int> decode_mouse_pos( V2<int> const& pos )
    {
        auto xc = (pos.x - x0) / spacing_ch;
        if( xc < 0 ){ xc = 0; }
        return V2<int>{xc, 0};
    }

    void caret_from_click( V2<int> const& pos )
    {
        auto v = decode_mouse_pos(pos); curx = v.x; ;
        sel.drop();
    }

    void sel_from_move(V2<int> const& pos)
    {
        auto v = decode_mouse_pos(pos);
        curx = v.x;
        if(!sel.active){ sel.create_at(curx, 0); }
        else{ sel.extend(v.x, 0); }
    }

    void clear_selection()
    {
        if(sel.active)
        {
            data.erase(sel.lo().x, sel.hi().x-sel.lo().x);
            curx = sel.lo().x;
            sel.drop();
        }
    }

    void key_down( wchar_t ch, bool shift, bool ctrl )
    {
        dcurx = 0;
        shifted = shift;
        switch (ch) 
        { 
            case VK_LEFT:   dcurx = -1;  op = TextOperation::caret; break;
            case VK_RIGHT:  dcurx = +1;  op = TextOperation::caret; break;
            case VK_HOME:   dcurx = -10; op = TextOperation::caret; break;
            case VK_END:    dcurx = +10; op = TextOperation::caret; break;

            case 'C':      if(ctrl){ wnd->clipboard.copy( data.substr(sel.lo().x, sel.hi().x-sel.lo().x) ); } break;
            case 'V':      if(ctrl){ clear_selection(); data.insert(curx, wnd->clipboard.paste() ); } break;
            case 'A':      if(ctrl){ sel.create_at(0, 0); sel.extend((int)data.length(), 0); } break;

            case VK_DELETE: op = TextOperation::del;       break;
            case VK_BACK:   op = TextOperation::backspace; break;
            case VK_RETURN: op = TextOperation::enter;     break;
            case VK_ESCAPE: sel.drop();                    break;
            //case VK_TAB: if(shift){ op = App::shift_tab; }else{ p->op = App::tab; } break;
            default: break;
        }
        update();
    }

    void charachter( wchar_t ch_in )
    {
        if( ch_in >= ' ' ){ op = TextOperation::character; ch = ch_in; update(); }
    }

    void paint( PaintStyle const& style )
    {
        auto lx = sel.lo().x;
        auto hx = sel.hi().x;
        if(has_focus)
        {
            auto ws1 = data.substr(0, lx);
            auto x00 = x0 + wnd->measureString(ws1).x;
            auto ws2 = data.substr(0, lx+(hx-lx));
            auto x01 = x0 + wnd->measureString(ws2).x;
            wnd->fillRect(x00, y0, x01-x00, spacing_y, style.sel);
        }

        wnd->drawText(x0, y0, data, style.fn);
        if(has_focus)
        {
            auto xc = x0 + wnd->measureString(data.substr(0, curx)).x;
            wnd->drawLine(xc, y0, xc, spacing_y, style.cur);
        }
    }
};

struct App
{
    HWND hwnd;
    HDC hdc;
    Window          wnd;

    File            file;
    SingleLineEdit  filename;
   
    PaintStyle  style;
    TextOperation   op;
    wchar_t     ch;
    bool        shifted;
    bool        has_focus = true;

    RegularSelection sel;
    int fst_vis_line;

    V2<int> cur;
    int dcurx, dcury;
    int spacing_x, spacing_y, spacing_ch;
    int recompile_timeout;

    std::atomic<int>    recompile_count;
    std::atomic<int>    recompile_close;
    std::future<void>   recompiler;
    
    App()
    {
        style.load_from(L"style.txt");
        cur = V2<int>{0, 0};
        fst_vis_line = 0;
      
        dcurx = dcury = 0;
        op = TextOperation::none;

        recompile_timeout = 100;
        recompile_close.store(0);
        recompile_count.store(recompile_timeout);
        recompiler = std::async(std::launch::async, [&]()
        {
            while(recompile_close.load() == 0)
            {
                auto c = recompile_count.load();
                if( c > 1 ){ recompile_count.fetch_sub(1); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
                else if( c == 1 )
                {
                    recompile_count.store(0);
                    get_build_log();
                    paint();
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        });

        sel.drop();
        make_source_focus();
    }

    ~App()
    {
        recompile_close.store(1);
        recompiler.wait();
    }

    bool load( std::wstring const& fn )
    {
        filename.set(fn);
        filename.update_target = [&](){ file.load(filename.data, true); sel.drop(); cur = V2<int>{0, 0}; make_source_focus(); op = TextOperation::none; paint(); };
        return file.load(fn);
    }

    void make_source_focus(){ has_focus = true; filename.has_focus = false; }
    void make_filename_focus(){ has_focus = false; filename.has_focus = true; }

    void mouse_left_down(V2<int> const& pos)
    {
        if(pos.y < spacing_y + 2)
        {
            filename.caret_from_click(pos);
            make_filename_focus();
        }
        else
        {
            caret_from_click(pos);
            make_source_focus();
        }
        paint();
    }

    void mouse_left_up(V2<int> const& pos){ paint(); }

    void mouse_move(V2<int> const& pos, bool left, bool mid, bool right)
    {
        if(has_focus && left){ sel_from_move(pos); paint(); }
        if(filename.has_focus && left){ filename.sel_from_move(pos); paint(); }
    }

    void key_down( wchar_t ch, bool shift, bool ctrl )
    {
        if(has_focus)
        {
            switch (ch) 
            { 
                case VK_LEFT:   dcurx = -1;  op = TextOperation::caret; break;
                case VK_RIGHT:  dcurx = +1;  op = TextOperation::caret; break;
                case VK_HOME:   dcurx = -10; op = TextOperation::caret; break;
                case VK_END:    dcurx = +10; op = TextOperation::caret; break;

                case VK_UP:     dcury = -1; op = TextOperation::caret; break;
                case VK_DOWN:   dcury = +1; op = TextOperation::caret; break;
                case VK_PRIOR:  dcury = -2; op = TextOperation::caret; break;
                case VK_NEXT:   dcury = +2; op = TextOperation::caret; break;

                //
                case 'C':      if(ctrl){ file.copy(wnd.clipboard, sel); } break;
                case 'V':      if(ctrl){ file.paste(wnd.clipboard, sel, cur); sel.drop(); } break;
                case 'A':      if(ctrl){ select_all(); } break;

                case VK_DELETE: op = TextOperation::del;       break;
                case VK_BACK:   op = TextOperation::backspace; break;
                case VK_RETURN: op = TextOperation::enter;     break;
                case VK_ESCAPE: sel.drop();                    break;
                case VK_TAB: if(shift){ op = TextOperation::shift_tab; }else{ op = TextOperation::tab; } break;
                default: break;
            }
            if(op == TextOperation::caret){ shifted = shift; }
        }
        if(filename.has_focus){ filename.key_down(ch, shift, ctrl); }
        paint();
    }

    bool createWindow(HINSTANCE hi, WNDPROC wp, int sh)
    {
        auto res = wnd.create(hi, wp, sh);
        auto spacing = wnd.measureString(L"00000|");
        spacing_x = spacing.x;
        filename.spacing_y = spacing_y = spacing.y;
        filename.spacing_ch = spacing_ch = wnd.measureString(L"_").x;
        filename.x0 = 2;
        filename.y0 = 2;
        filename.data = file.fn;
        filename.wnd = &wnd;
        filename.sel.drop();
        return res;
    }

    void start_sel(){ sel.create_at(cur); }

    void select_all()
    {
        sel.create_at(0, 0);
        sel.extend( file[last_line_idx()].length(), last_line_idx() );
    }

    int last_line_idx() const { return file.last_line_idx(); }

    V2<int> decode_mouse_pos( V2<int> const& pos )
    {
        auto xc = (pos.x - spacing_x-10) / spacing_ch;
        auto yc = (pos.y - 25) / spacing_y + fst_vis_line;
        if( xc < 0 ){ xc = 0; }
        if( yc < 0 ){ yc = 0; }
        return V2<int>{xc, yc};
    }

    void caret_from_click( V2<int> const& pos )
    {
        auto v = decode_mouse_pos(pos);
        normalize_cursor(v);
        sel.drop();
    }

    void sel_from_move(V2<int> const& pos)
    {
        auto v = decode_mouse_pos(pos);
        normalize_cursor(v);
        if(!sel.active){ start_sel(); }
        else{ sel.extend(cur); }
    }

    void mouse_wheel(int z)
    {
        auto tcy = fst_vis_line - z * 10;
        if(tcy < 0){ tcy = 0; }
        if(tcy > file.last_line_idx()){ tcy = file.last_line_idx(); }
        fst_vis_line = tcy;
        paint();
    }

    void charachter( wchar_t ch_in )
    {
        if(has_focus)
        {
            if( ch_in >= ' ' ){ op = TextOperation::character; ch = ch_in; }
            recompile_count.store(recompile_timeout);
        }
        if(filename.has_focus){ filename.charachter(ch_in); }
        paint();
    }

    void normalize_cursor(V2<int> const& pos)
    {
        cur = pos;
        if(cur.y > last_line_idx()){ cur.y = last_line_idx(); }
        auto lc = file.lines[cur.y].length();
        if(cur.x > lc){ cur.x = lc; }
    }
   
    void paint()
    {
        if( file.lines.size() == 0 ){ return; }
        //move caret:
        if(op == TextOperation::caret)
        {
            if(shifted && !sel.active){ sel.create_at(cur); }
            if(dcurx != 0)
            {
                auto lc = file.lines[cur.y].length();
                if(dcurx == -1)
                {
                    if(cur.x == 0)
                    {
                        if(cur.y != 0){ cur.y -=1; cur.x = file.lines[cur.y].length(); }
                    }
                    else{ cur.x -= 1; }
                }
                else if(dcurx == +1)
                {
                    if(cur.x == lc)
                    {
                        cur.y += 1;
                        if(cur.y > last_line_idx()){ cur.y = last_line_idx(); }
                        else{ cur.x = 0; }
                    }
                    else{ cur.x += 1; }
                }
                else if(dcurx == -10){ cur.x = 0; }
                else if(dcurx == +10){ cur.x = lc; }
            }
            else if(dcury != 0)
            {
                if(dcury == -1)
                {
                    if(cur.y == 0){}
                    else{ cur.y -= 1; normalize_cursor(cur); }
                }
                else if(dcury == +1)
                {
                    if(cur.y >= last_line_idx()){}
                    else{ cur.y += 1; normalize_cursor(cur); }
                }
                else if(dcury == -2)
                {
                    auto H = wnd.h / spacing_y;
                    if(cur.y < H){ cur.y = 0; }else{ cur.y -= H; }
                    normalize_cursor(cur);
                }
                else if(dcury == +2)
                {
                    auto H = wnd.h / spacing_y;
                    if(last_line_idx() - cur.y < H){ cur.y = last_line_idx(); cur.x = file.lines[cur.y].length(); }
                    else{ cur.y += H; normalize_cursor(cur); }
                }
            }
            if(cur.y < fst_vis_line){ fst_vis_line = cur.y; }
            if(cur.y > fst_vis_line + wnd.h / spacing_y - 1){ fst_vis_line = cur.y - wnd.h / spacing_y + 1; }
            dcurx = dcury = 0;
            op = TextOperation::none;
            if(!shifted){ sel.drop(); }
            if(shifted && sel.active){ sel.extend(cur); }
            
        }
        
        if(sel.active && (op == TextOperation::backspace || op == TextOperation::character || op == TextOperation::del || op == TextOperation::enter) )
        {
            file.delete_selection(sel);
            cur = sel.lo();
            sel.drop();
            if(op == TextOperation::backspace || op == TextOperation::del){ op = TextOperation::none; }
        }
        
        if(!paint_impl()){ paint_impl(); }
    }

    bool paint_impl()
    {
        auto h = spacing_y;
        auto x0 = spacing_x+10;
        wnd.fillRect(0, 0, wnd.w, wnd.h, style.bk);
        
        int x = 0;
        int y = 25;

        //filename edit control
        filename.paint(style);
        
        //source edit field
        for(int l = fst_vis_line; l<(int)file.lines.size(); ++l)
        {
            if( y > wnd.h - spacing_y){ y += h; break; }
            auto const& line = file.lines[l];
            x = x0;
            wnd.drawText(0, y, std::to_wstring(l+1), style.ln);

            int wc = 0;
            if(l == cur.y)
            {
                std::wstring wsum = line.str();
                bool has_content_access = file.mcontent.try_lock();
                if(has_content_access)
                {
                    bool repaint = false;
                    bool full_reformat = false;
                    if(op != TextOperation::none)
                    {
                        if(op == TextOperation::character)
                        {
                            wsum.insert(wsum.begin()+cur.x, ch);
                            file.lines[l].set(wsum);
                            cur.x += 1;
                        }
                        else if(op == TextOperation::backspace)
                        {
                            if(cur.y > 0 && cur.x == 0)
                            {
                                std::wstring wsum2 = file.lines[l-1].str();
                                file.lines[l-1].set(wsum2 + wsum);
                                file.lines.erase(file.lines.begin()+l);
                                l -= 1;
                                cur.y -= 1;
                                cur.x = (int)wsum2.size();
                                repaint = true;
                            }
                            else if(cur.x > 0)
                            {
                                if(wsum[cur.x-1] == L'/' || wsum[cur.x-1] == L'*'){ full_reformat = true; }
                                wsum.erase(cur.x-1, 1);
                                file.lines[l].set(wsum);
                                cur.x -= 1;
                            }
                        }
                        else if(op == TextOperation::del)
                        {
                            if(cur.y == last_line_idx() && cur.x == file.lines[l].length())
                            {
                            }
                            else if(cur.x == file.lines[l].length())
                            {
                                std::wstring wsum2 = file.lines[l+1].str();
                                file.lines[l].set(wsum + wsum2);
                                file.lines.erase(file.lines.begin()+l+1);
                                if( l > 0 ){ l -= 1; }
                                repaint = true;
                            }
                            else if(cur.x < file.lines[l].length())
                            {
                                if(wsum[cur.x] == L'/' || wsum[cur.x] == L'*'){ full_reformat = true; }
                                wsum.erase(cur.x, 1);
                                file.lines[l].set(wsum);
                            }
                        }
                        else if(op == TextOperation::tab)
                        {
                            if(!sel.active)
                            {
                                wsum.insert(cur.x, std::wstring(L"  "));
                                file.lines[l].set(wsum);
                                cur.x += 2;
                            }
                            else
                            {
                                for(int sl=sel.pos0.y; sl<=sel.pos1.y; ++sl)
                                {
                                    file.lines[sl].text.insert(0, L"  ");
                                    file.format_line(file.lines[sl], file.lines[sl].is_multiline_comment);
                                }
                            }
                        }
                        else if(op == TextOperation::shift_tab)
                        {
                            if(!sel.active)
                            {
                                file.lines[l].reduce_initial_ws();
                                file.format_line(file.lines[l], file.lines[l].is_multiline_comment);
                            }
                            else
                            {
                                for(int sl=sel.pos0.y; sl<=sel.pos1.y; ++sl)
                                {
                                    file.lines[sl].reduce_initial_ws();
                                    file.format_line(file.lines[sl], file.lines[sl].is_multiline_comment);
                                }
                            }
                        }
                        else if(op == TextOperation::enter)
                        {
                            auto s1  = wsum.substr(0, cur.x);
                            auto s20 = wsum.substr(cur.x);
                            std::wstring spacer;
                            bool zerocurx = true;
                            if( cur.x > 0 && file.lines[l].text[cur.x-1] == L'{' ){ for(int s=0; s<cur.x+4; ++s){ spacer += L" "; } cur.x += 3; zerocurx = false; }
                        
                            auto s2 = spacer + s20;

                            file.lines[l].set(s2);
                            file.lines.insert(file.lines.begin()+l, Line{ s1, std::vector<Entry>{}, file.lines[l].is_multiline_comment} );
                            cur.y += 1;
                            repaint = true;
                            if(zerocurx){ cur.x = 0; }
                        
                        }
                    
                        auto multilinecomment = full_reformat || file.lines[l].text.find(L"/*") != std::wstring::npos || file.lines[l].text.find(L"*/") != std::wstring::npos;
                        if(multilinecomment){ file.format(); }
                        else
                        {
                            bool dummy = file.lines[l].is_multiline_comment;
                            file.format_line(file.lines[l], dummy);
                            if(l+1<(int)file.lines.size())
                            {
                                dummy = file.lines[l+1].is_multiline_comment;
                                file.format_line(file.lines[l+1], dummy);
                            }
                            if(l!=0)
                            {
                                dummy = file.lines[l-1].is_multiline_comment;
                                file.format_line(file.lines[l-1], dummy);
                            }
                        }
                        recompile_count.store(recompile_timeout);
                        op = TextOperation::none;
                        if(repaint){ file.mcontent.unlock(); return false; }
                    }
                    file.mcontent.unlock();
                    wsum += L"|";
                    auto sub = wsum.substr(0, cur.x);
                    wc = x0+wnd.measureString(sub).x;
                }//lock
                else
                {
                    wsum += L"|";
                    auto sub = wsum.substr(0, cur.x);
                    wc = x0+wnd.measureString(sub).x;
                }
            }

            {
                auto lo = sel.lo();
                auto hi = sel.hi();
                if(lo.y <= l && l <= hi.y)
                {
                    std::wstring wsum = line.str();
                    
                    auto hx = hi.x;
                    auto lx = lo.x;
                    if( sel.is_single_line() )
                    {
                        if( wsum.size() == 0 ){ wnd.fillRect(x0, y, 5, h, style.sel); }
                        else
                        {
                            if((int)wsum.size() > lx)
                            {
                                auto ws1 = wsum.substr(0, lx);
                                auto x00 = x0 + wnd.measureString(ws1).x;
                                auto ws2 = wsum.substr(0, lx+(hx-lx));
                                auto x01 = x0 + wnd.measureString(ws2).x;
                                wnd.fillRect(x00, y, x01-x00, h, style.sel);
                            }
                        }
                    }
                    else
                    {
                        std::wstring ws1, ws2;
                        if( l == lo.y )
                        {
                            ws1 = wsum.substr(0, lo.x);
                            ws2 = wsum.substr(0, wsum.npos);
                        }
                        else if(l == hi.y)
                        {
                            ws1 = wsum.substr(0, 0);
                            ws2 = wsum.substr(0, hi.x);
                        }
                        else
                        {
                            ws1 = wsum.substr(0, 0);
                            ws2 = wsum.substr(0, wsum.npos);
                        }
                        auto x00 = x0 + wnd.measureString(ws1).x;
                        auto x01 = x0 + wnd.measureString(ws2).x;
                        if( wsum.size() == 0 ){ wnd.fillRect(x00, y, 5,       h, style.sel); }
                        else{                   wnd.fillRect(x00, y, x01-x00, h, style.sel); }
                    }
                }
            }

            for(auto const& elem : line.entries)
            {
                auto text = line.sub(elem.p0, elem.p1);
                wnd.drawText(x, y, text.c_str(), elem.style());
                x += wnd.measureString(text).x;
            }
            
            if(l == cur.y){ wnd.drawLine(wc, y, wc, y+h, style.cur); }

            if(file.mlog.try_lock())
            {
                std::vector<std::wstring> cs;
                std::vector<LogType> lt;
                bool frame = false;
                for(int c=0; c<(int)file.logs.size(); ++c)
                {
                    const Log* log = nullptr;
                    auto const& loglines = file.logs[c].lines;
                    for(int li=0; li<(int)loglines.size(); ++li){ if(loglines[li].line == l+1){ log = &loglines[li]; break; } }

                    if( log )
                    {
                        cs.push_back( file.logs[c].compiler.name + L" ");
                        lt.push_back(log->ty);
                        if(!frame){ wnd.drawRect(0, y, wnd.w, h, style.grey);  frame = true; }
                        if     ( log->ty == LogType::Error   ){ wnd.drawRect(0, y, wnd.w, h, style.err); frame = true; wnd.fillRect(x0-14, y, 8, h, style.err);  }
                        else if( log->ty == LogType::Warning ){ wnd.fillRect(x0-6 , y, 8, h, style.warn); }
                    }
                }
                file.mlog.unlock();
                int csw = 0;
                for(int i=0; i<(int)cs.size(); ++i)
                {
                    csw += wnd.measureString(cs[i]).x;
                    wnd.drawText(wnd.w-20-csw, y, cs[i], style.LogStyle(lt[i]));
                }
            }//try lock log
            y += h;
        }//src lines

        wnd.drawText(wnd.w-100, 10, std::to_wstring(cur.x), style.cur);
        wnd.drawText(wnd.w-100, 30, std::to_wstring(cur.y), style.cur);
        wnd.drawText(wnd.w-100, 50, std::to_wstring(recompile_count.load()), style.cur);
        
        wnd.present();
        return true;
    }
};

std::unique_ptr<App> app;

Style syntax_style(SyntaxGroup const& sg)
{
    auto const& style = app->style;
    switch(sg)
    {
        case WhiteSpace   : return style.defaultcolor;
        case Operator     : return style.operatorcolor;
        case Identifier   : return style.identifier;
        case Number       : return style.numcolor;    
        case Text         : return style.strcolor;    
        case Include      : return style.includecolor;
        case Preprocessor : return style.definecolor; 
        case Comment      : return style.commentcolor;
        case Keyword      : return style.keycolor;
        default           : return style.defaultcolor;
    }
}

std::wstring get_build_log()
{
    {
        std::lock_guard<std::mutex> lock(app->file.mcontent);
        std::wstring alls = app->file.recreate();
        std::wofstream file(app->file.fn);
        std::copy(alls.begin(), alls.end(), std::ostreambuf_iterator<wchar_t>(file));
    }
   
    {
        std::lock_guard<std::mutex> lock(app->file.mlog);
        app->file.clear_logs();
        std::vector<cmd_handle> hs(app->file.logs.size());
        for(int i=0; i<(int)app->file.logs.size(); ++i)
        {
            auto const& c = app->file.logs[i].compiler;
            auto logfn = c.name + L"_log.txt";
            auto cmdline = c.get_cmdline(app->file.fn, app->file.fn_without_ext() + L"_" + c.name + L".obj") + L" " + c.fwd_prefix + logfn + (c.in_wsl ? L"\"" : L"");//c.cmd_terminator;
            hs[i] = windows_system( cmdline );
        }
    
        for(int i=0; i<(int)app->file.logs.size(); ++i)
        {
            auto const& c = app->file.logs[i].compiler;
            auto logfn = c.name + L"_log.txt";
            hs[i].wait();

            {
                std::wifstream file(c.name + L"_log.txt");
                std::wstring str; 
                while(std::getline(file, str))
                {
                    auto search = app->file.fn + c.diag_sep[0];
                    auto q = str.find(search);

                    if(c.diag_sep.size() == 3)
                    {
                        auto r = str.find(c.diag_sep[1], q);
                        auto s = str.find(c.diag_sep[2], q);
                        if(q == str.npos || r == str.npos || s == str.npos){ continue; }
                        auto s0 = str.substr(q+search.size(), s-q-search.size());
                        auto l = std::stoi(s0);

                        s0 = str.substr(r+1, s-r-1);
                        auto c = std::stoi(s0);

                        Log olog;
                        olog.line = l;
                        olog.col0 = c; olog.col1 = c;

                        auto type = str.substr(r+2, r+14);
                        if     (type.find(L"error")   != str.npos){ olog.ty = LogType::Error; }
                        else if(type.find(L"warning") != str.npos){ olog.ty = LogType::Warning; }
                        else if(type.find(L"note")    != str.npos){ olog.ty = LogType::Note; }
          
                        app->file.logs[i].lines.push_back(olog);
                    }
                    else if(c.diag_sep.size() == 2 && c.diag_sep[0] != L':' && c.diag_sep[1] != L':' )
                    {
                        auto r = str.find(c.diag_sep[1], q);
                        if(q == str.npos || r == str.npos){ continue; }
                        auto s0 = str.substr(q+search.size(), r-q-search.size());
                        auto l = std::stoi(s0);

                        Log olog;
                        olog.line = l;
                        olog.col0 = -1; olog.col1 = -1;

                        auto type = str.substr(r+2, r+14);
                        if     (type.find(L"error")   != str.npos){ olog.ty = LogType::Error; }
                        else if(type.find(L"warning") != str.npos){ olog.ty = LogType::Warning; }
                        else if(type.find(L"note")    != str.npos){ olog.ty = LogType::Note; }
          
                        app->file.logs[i].lines.push_back(olog);
                    }
                    else if(c.diag_sep.size() == 2 && c.diag_sep[0] == L':' && c.diag_sep[1] == L':' )
                    {
                        auto r = str.find(c.diag_sep[1], q);
                        if(q == str.npos || r == str.npos){ continue; }
                        auto s0 = str.substr(q+search.size(), r-q-search.size());
                        if(s0[0] == ' '){ continue; }
                        auto l = std::stoi(s0);

                        Log olog;
                        olog.line = l;
                        olog.col0 = -1; olog.col1 = -1;

                        auto type = str.substr(r+2, r+14);
                        if     (type.find(L"error")   != str.npos){ olog.ty = LogType::Error; }
                        else if(type.find(L"warning") != str.npos){ olog.ty = LogType::Warning; }
                        else if(type.find(L"note")    != str.npos){ olog.ty = LogType::Note; }
          
                        app->file.logs[i].lines.push_back(olog);
                    }
                }//getline
            }//scope
        }//for compilers
    }//log lock scope

    return L"";
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int tmp = 0;
    bool ctrl = false;
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(EXIT_SUCCESS);
        break;
    case WM_LBUTTONDOWN:
        app->mouse_left_down( V2<int>{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)} );
        break;
    case WM_MOUSEMOVE:
        app->mouse_move( V2<int>{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}, (bool)(wParam & MK_LBUTTON), (bool)(wParam & MK_MBUTTON), (bool)(wParam & MK_RBUTTON) );
        break;
    case WM_LBUTTONUP:
        app->mouse_left_up( V2<int>{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)} );
        break;
    case WM_MOUSEWHEEL:
        app->mouse_wheel(GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA);
        break;
    case WM_KEYDOWN:
        app->key_down((wchar_t)wParam, (bool)(GetKeyState(VK_SHIFT) & 0x8000), (bool)(GetKeyState(VK_LCONTROL) & 0x8000));
        break;
    case WM_CHAR:
        app->charachter( (wchar_t)wParam );
        break;
    case WM_SIZE:
        app->wnd.Resize(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_PAINT:
        app->paint();
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

bool load_compiler( std::wstring const& fn )
{
    std::wifstream file(fn);
    if(!file.is_open()){ return false; }

    Compiler c;

    std::wstring line;
    while( std::getline(file, line) )
    {
        std::wistringstream ss(line);
        std::wstring field;
        std::wstring op;
        std::wstring value;
        if( !(ss >> field >> op) ){ break; }
        std::getline(ss, value, L'\n');
        auto p = value.find_first_not_of(L" ");
        if(p != value.npos){ value = value.substr(p); }
        auto q = value.find_last_not_of(L" ");
        if(q != value.npos){ value = value.substr(0, q+1); }
        if(op != L"="){ break; }
        
        if     (field == L"name"      ){ c.name = value; }
        else if(field == L"in_wsl"    ){ c.in_wsl = (value == L"true" ? true : false); }
        else if(field == L"env"       ){ c.env = value; }
        else if(field == L"env_arch"  ){ c.env_arch = value; }
        else if(field == L"cmd_chain" ){ c.cmd_chain = value; }
        else if(field == L"command"   ){ c.compiler = value; }
        else if(field == L"flags"     ){ c.flags = value; }
        else if(field == L"out_prefix"){ c.out_prefix = value; }
        else if(field == L"fn_suffix" ){ c.fn_suffix = value; }
        else if(field == L"fwd_prefix"){ c.fwd_prefix = value; }
        else if(field == L"diag_sep"  ){ c.diag_sep = value; }
    }
    app->file.add_compiler( std::move(c) );
    return true;
}

void load_compilers( std::wstring const& fn )
{
    std::wifstream file(fn);
    if(!file.is_open())
    {
        //Error opening compilers file
        //MessageBox(app->wnd.hwnd, L"Error opening compilers.txt", L"Error", 0);
        return;
    }

    std::wstring line;
    while( std::getline(file, line) )
    {
        bool res = load_compiler(line);
        if(!res)
        {
            //auto text = L"Error loading compiler from: " + line;
            //MessageBox(app->wnd.hwnd, text.c_str(), L"Error", 0);
        }
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* nCmdLine, int nCmdShow)
{
    
    app = std::make_unique<App>();
    app->createWindow(hInstance, WndProc, nCmdShow);

    load_compilers(L"compilers.txt");
    //load_compiler(L"g++.compiler");
    //load_compiler(L"msvc15.compiler");

    /*{
        Compiler msvc;
        msvc.name = L"MSVC15";
        msvc.prefix = L"\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x86 && cl.exe /EHsc /nologo";
        msvc.out_prefix = L"/Fo:";
        msvc.fn_suffix = L"";
        msvc.fwd_prefix = L">";
        msvc.diag_sep = L"()";
        app->file.add_compiler( std::move(msvc) );
    }

    {
        Compiler msvc2;
        msvc2.name = L"MSVC12";
        msvc2.prefix = L"\"C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\vcvarsall.bat\" x86 && cl.exe /EHsc /nologo";
        msvc2.out_prefix = L"/Fo:";
        msvc2.fn_suffix = L"";
        msvc2.fwd_prefix = L">";
        msvc2.diag_sep = L"()";
        app->file.add_compiler( std::move(msvc2) );
    }

    {
        Compiler clang;
        clang.name = L"clang";
        clang.prefix = L"\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x86 && \"C:\\Program Files (x86)\\LLVM\\bin\\clang-cl.exe\" /EHsc /nologo -Xclang -pedantic";
        clang.out_prefix = L"-o";
        clang.fn_suffix = L"-c";
        clang.fwd_prefix = L"2>";
        clang.diag_sep = L"(,)";
        app->file.add_compiler( std::move(clang) );
    }*/

    /*{
        Compiler gpp;
        gpp.name = L"g++";
        //bash.exe location is different for 32 bits and 64 bits:
#ifdef _WIN64
        std::wstring bash_prefix(L"\"bash.exe\""); 
#else
        std::wstring bash_prefix(L"\"C:\\Windows\\Sysnative\\bash.exe\"");
#endif
        gpp.prefix = bash_prefix + L" -c \"cd " + wsl_current_working_dir() + L" && g++-6 -pedantic -std=c++14";
        gpp.out_prefix = L"-o";
        gpp.fn_suffix = L"-c";
        gpp.fwd_prefix = L"2>";
        gpp.diag_sep = L"::";
        gpp.cmd_terminator = L"\"";
        app->file.add_compiler( std::move(gpp) );
    }*/

    if(!app->load( nCmdLine ))
    {
        app->load( L"test.cpp");
    }

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}

