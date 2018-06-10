#define KB_SHIFT   0x00010000
#define KB_ALT     0x00020000
#define KB_CTRL    0x00040000

struct _transkey
{
    int vk_key;
    int plain, shift, alt, ctrl;
};

struct _transkey winkeytable[] =
{
    //VK_CANCEL ?
    {VK_BACK, _BackSpace, -1, _AltBackSpace, _CtrlBackSpace},
    {VK_TAB, _Tab, _ShiftTab, _AltTab, _CtrlTab},
    //VK_CLEAR ?
    {VK_RETURN, _Enter, _Enter, _AltEnter, _CtrlEnter},
    //VK_SHIFT
    //VK_CONTROL
    //VK_MENU
    //VK_PAUSE
    //VK_CAPITAL
    {VK_ESCAPE, _Esc, -1, _AltEsc, -1},
    {VK_SPACE, _Space, ' ', ' ', ' '},
    {VK_PRIOR, _PgUp,  -1,  -1,  _CtrlPgUp},
    {VK_NEXT,  _PgDn,  -1,  -1,  _CtrlPgDn},
    {VK_END,   _End,   -1,   _AltEnd,   _CtrlEnd},
    {VK_HOME,  _Home,  -1,  _AltHome,  _CtrlHome},
    {VK_LEFT,  _Left,  -1,  _AltLeft,  _CtrlLeft},
    {VK_UP,    _Up,    -1,    _AltUp,    _CtrlUp},
    {VK_RIGHT, _Right, -1, _AltRight, _CtrlRight},
    {VK_DOWN,  _Down,  -1,  _AltDown,  _CtrlDown},
    //VK_SELECT
    //VK_PRINT
    //VK_EXECUTE
    //VK_SNAPSHOT
    {VK_INSERT, _Insert, _ShiftInsert, _AltInsert, _CtrlInsert},
    {VK_DELETE, _Delete, -1, _AltDelete, _CtrlDelete},
    //VK_HELP
    {VK_0, '0', ')', -1, -1},
    {VK_1, '1', '!', -1, -1},
    {VK_2, '2', '@', -1, -1},
    {VK_3, '3', '#', -1, -1},
    {VK_4, '4', '$', -1, -1},
    {VK_5, '5', '%', -1, -1},
    {VK_6, '6', '^', -1, -1},
    {VK_7, '7', '&', -1, -1},
    {VK_8, '8', '*', -1, -1},
    {VK_9, '9', '(', -1, -1},
    {VK_A, 'a', 'A', _AltA, _CtrlA},
    {VK_B, 'b', 'B', _AltB, _CtrlB},
    {VK_C, 'c', 'C', _AltC, _CtrlC},
    {VK_D, 'd', 'D', _AltD, _CtrlD},
    {VK_E, 'e', 'E', _AltE, _CtrlE},
    {VK_F, 'f', 'F', _AltF, _CtrlF},
    {VK_G, 'g', 'G', _AltG, _CtrlG},
    {VK_H, 'h', 'H', _AltH, _CtrlH},
    {VK_I, 'i', 'I', _AltI, _CtrlI},
    {VK_J, 'j', 'J', _AltJ, _CtrlJ},
    {VK_K, 'k', 'K', _AltK, _CtrlK},
    {VK_L, 'l', 'L', _AltL, _CtrlL},
    {VK_M, 'm', 'M', _AltM, _CtrlM},
    {VK_N, 'n', 'N', _AltN, _CtrlN},
    {VK_O, 'o', 'O', _AltO, _CtrlO},
    {VK_P, 'p', 'P', _AltP, _CtrlP},
    {VK_Q, 'q', 'Q', _AltQ, _CtrlQ},
    {VK_R, 'r', 'R', _AltR, _CtrlR},
    {VK_S, 's', 'S', _AltS, _CtrlS},
    {VK_T, 't', 'T', _AltT, _CtrlT},
    {VK_U, 'u', 'U', _AltU, _CtrlU},
    {VK_V, 'v', 'V', _AltV, _CtrlV},
    {VK_W, 'w', 'W', _AltW, _CtrlW},
    {VK_X, 'x', 'X', _AltX, _CtrlX},
    {VK_Y, 'y', 'Y', _AltY, _CtrlY},
    {VK_Z, 'z', 'Z', _AltZ, _CtrlZ},
    {188, ',', '<', -1, -1},
    {190, '.', '>', -1, -1},
    {191, '/', '?', -1, -1},
    {186, ';', ':', -1, -1},
    {222, '\'', '\"', -1, -1},
    {219, '[', '{', -1, -1},
    {221, ']', '}', -1, -1},
    {189, '-', '_', -1, -1},
    {187, '=', '+', -1, -1},
    {220, '\\', '|', -1, -1},
    {192, '`', '~', -1, -1},
    //VK_LWIN
    //VK_RWIN
    //VK_APPS
    /*
    VK_NUMPAD0
    VK_NUMPAD1
    VK_NUMPAD2
    VK_NUMPAD3
    VK_NUMPAD4
    VK_NUMPAD5
    VK_NUMPAD6
    VK_NUMPAD7
    VK_NUMPAD8
    VK_NUMPAD9
    */
    {VK_MULTIPLY, _Asterisk, _Asterisk, _AltNumAsterisk, _CtrlNumAsterisk},
    {VK_ADD, '+', '+', -1, _CtrlNumPlus},
    //{VK_SEPARATOR
    {VK_SUBTRACT, '-', '-', -1, _CtrlNumMinus},
    //VK_DECIMAL
    {VK_DIVIDE, '/', '/', -1, _CtrlNumSlash},
    {VK_F1,  _F1,  _ShiftF1,  _AltF1,  _CtrlF1},
    {VK_F2,  _F2,  _ShiftF2,  _AltF2,  _CtrlF2},
    {VK_F3,  _F3,  _ShiftF3,  _AltF3,  _CtrlF3},
    {VK_F4,  _F4,  _ShiftF4,  _AltF4,  _CtrlF4},
    {VK_F5,  _F5,  _ShiftF5,  _AltF5,  _CtrlF5},
    {VK_F6,  _F6,  _ShiftF6,  _AltF6,  _CtrlF6},
    {VK_F7,  _F7,  _ShiftF7,  _AltF7,  _CtrlF7},
    {VK_F8,  _F8,  _ShiftF8,  _AltF8,  _CtrlF8},
    {VK_F9,  _F9,  _ShiftF9,  _AltF9,  _CtrlF9},
    {VK_F10, _F10, _ShiftF10, _AltF10, _CtrlF10},
    {VK_F11, _F11, _ShiftF11, _AltF11, _CtrlF11},
    {VK_F12, _F12, _ShiftF12, _AltF12, _CtrlF12},
};

