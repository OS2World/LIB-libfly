#ifndef FLY_MESSAGES_H_INCLUDED
#define FLY_MESSAGES_H_INCLUDED

/* ---------------------------------------------------------------------
 Messages from interface system
 ----------------------------------------------------------------------- */
#define FMSG_BASE_KEYS           000000000
#define FMSG_BASE_SYSTEM         100000000
#define FMSG_BASE_MENU           200000000
#define FMSG_BASE_MOUSE          300000000

#define IS_KEY(k)    (k < FMSG_BASE_SYSTEM && k >= FMSG_BASE_KEYS  )
#define IS_SYSTEM(k) (k < FMSG_BASE_MENU   && k >= FMSG_BASE_SYSTEM)
#define IS_MENU(k)   (k < FMSG_BASE_MOUSE  && k >= FMSG_BASE_MENU  )
#define IS_MOUSE(k)  (                        k >= FMSG_BASE_MOUSE )

/* Mousing.
 message format:    3          00    0     000  000 (decimal)
 .                  mouse      event kbd   row  column
 .                  signature  type  flags
 example:           3          12    7     080  050 (3 127 080 050)
 */
#define FMSG_BASE_MOUSE_EVTYPE     1000000
#define FMSG_BASE_MOUSE_Y             1000
#define FMSG_BASE_MOUSE_X                1

#define MOUEV_MOVE       40    // mouse is moved, no buttons are down

#define MOUEV_B1SC        1    // button1 single click
#define MOUEV_B1DC        2    // button1 double click
#define MOUEV_B1MS        3    // start dragging with button1
#define MOUEV_B1DR        4    // dragging with button1
#define MOUEV_B1ME        5    // end dragging with button1
#define MOUEV_B1AC        6    // autoclick, i.e. button is held down
#define MOUEV_B1DN        7    // button down
#define MOUEV_B1UP        8    // button up

#define MOUEV_B2SC       11    // button2 single click
#define MOUEV_B2DC       12    // button2 double click
#define MOUEV_B2MS       13    // start dragging with button2
#define MOUEV_B2DR       14    // dragging with button2
#define MOUEV_B2ME       15    // end dragging with button2
#define MOUEV_B2AC       16    // autoclick, i.e. button is held down
#define MOUEV_B2DN       17    // button down
#define MOUEV_B2UP       18    // button up

#define MOUEV_B3SC       21    // button3 single click
#define MOUEV_B3DC       22    // button3 double click
#define MOUEV_B3MS       23    // start dragging with button3
#define MOUEV_B3DR       24    // dragging with button3
#define MOUEV_B3ME       25    // end dragging with button3
#define MOUEV_B3AC       26    // autoclick, i.e. button is held down
#define MOUEV_B3DN       27    // button down
#define MOUEV_B3UP       28    // button up

#define MOU_EVTYPE(k)     ((k/FMSG_BASE_MOUSE_EVTYPE)%100)
#define MOU_X(k)          ((k/FMSG_BASE_MOUSE_X)%1000)        // column
#define MOU_Y(k)          ((k/FMSG_BASE_MOUSE_Y)%1000)        // row

#define FMSG_BASE_SYSTEM_TYPE     1000000
#define FMSG_BASE_SYSTEM_INT2        1000
#define FMSG_BASE_SYSTEM_INT1           1
#define SYS_TYPE(k)       ((k/FMSG_BASE_SYSTEM_TYPE)%100)
#define SYS_INT1(k)       ((k/FMSG_BASE_SYSTEM_INT1)%1000)
#define SYS_INT2(k)       ((k/FMSG_BASE_SYSTEM_INT2)%1000)

#define SYSTEM_RESIZE        1       //int1 - cols, int2 - rows
#define SYSTEM_QUIT          2       // ask user confirmation, then close
#define SYSTEM_DIE           3       // die unconditionally

/* ---------------------------------------------------------------------
 *   (C)  A.S.V.  1990, 1991, 1996, 1998.
 *   KEYDEFS.H  -  Macros for keyboard codes
 ----------------------------------------------------------------------- */

#define __EXT_KEY_BASE__       256

#define KEY_NOTHING          -1
#define _CtrlA                1
#define _CtrlB                2
#define _CtrlC                3
#define _CtrlD                4
#define _CtrlE                5
#define _CtrlF                6
#define _CtrlG                7
#define _CtrlH                8
#define _BackSpace            8
#define _CtrlI                9
#define _Tab                  9
#define _CtrlJ               10
#define _CtrlEnter           10
#define _CtrlK               11
#define _CtrlL               12
#define _CtrlM               13
#define _Enter               13
#define _CtrlN               14
#define _CtrlO               15
#define _CtrlP               16
#define _CtrlQ               17
#define _CtrlR               18
#define _CtrlS               19
#define _CtrlT               20
#define _CtrlU               21
#define _CtrlV               22
#define _CtrlW               23
#define _CtrlX               24
#define _CtrlY               25
#define _CtrlZ               26
#define _Esc                 27
#define _CtrlLeftRectBrac    27
#define _CtrlBackSlash       28
#define _CtrlRightRectBrac   29
#define _Ctrl6               30
#define _CtrlMinus           31
#define _Space               32

/* Letters a-z, A-Z here .... */

#define _Asterisk            42
#define _Plus                43
#define _Minus               45

#define _CtrlBackSpace      127

#define _CtrlBreak          __EXT_KEY_BASE__+0
#define _AltEsc             __EXT_KEY_BASE__+1
#define _Ctrl2              __EXT_KEY_BASE__+3
#define _AltBackSpace       __EXT_KEY_BASE__+14
#define _ShiftTab           __EXT_KEY_BASE__+15
#define _AltQ               __EXT_KEY_BASE__+16
#define _AltW               __EXT_KEY_BASE__+17
#define _AltE               __EXT_KEY_BASE__+18
#define _AltR               __EXT_KEY_BASE__+19
#define _AltT               __EXT_KEY_BASE__+20
#define _AltY               __EXT_KEY_BASE__+21
#define _AltU               __EXT_KEY_BASE__+22
#define _AltI               __EXT_KEY_BASE__+23
#define _AltO               __EXT_KEY_BASE__+24
#define _AltP               __EXT_KEY_BASE__+25
#define _AltLeftRectBrac    __EXT_KEY_BASE__+26   /* Alt-[ */
#define _AltRightRectBrac   __EXT_KEY_BASE__+27   /* Alt-] */
#define _AltEnter           __EXT_KEY_BASE__+28
#define _AltA               __EXT_KEY_BASE__+30
#define _AltS               __EXT_KEY_BASE__+31
#define _AltD               __EXT_KEY_BASE__+32
#define _AltF               __EXT_KEY_BASE__+33
#define _AltG               __EXT_KEY_BASE__+34
#define _AltH               __EXT_KEY_BASE__+35
#define _AltJ               __EXT_KEY_BASE__+36
#define _AltK               __EXT_KEY_BASE__+37
#define _AltL               __EXT_KEY_BASE__+38
#define _AltSemicolon       __EXT_KEY_BASE__+39
#define _AltApostrophe      __EXT_KEY_BASE__+40
#define _AltBackApostrophe  __EXT_KEY_BASE__+41
#define _AltBackSlash       __EXT_KEY_BASE__+43
#define _AltZ               __EXT_KEY_BASE__+44
#define _AltX               __EXT_KEY_BASE__+45
#define _AltC               __EXT_KEY_BASE__+46
#define _AltV               __EXT_KEY_BASE__+47
#define _AltB               __EXT_KEY_BASE__+48
#define _AltN               __EXT_KEY_BASE__+49
#define _AltM               __EXT_KEY_BASE__+50
#define _AltComma           __EXT_KEY_BASE__+51
#define _AltPeriod          __EXT_KEY_BASE__+52
#define _AltSlash           __EXT_KEY_BASE__+53
#define _AltNumAsterisk     __EXT_KEY_BASE__+55
#define _F1                 __EXT_KEY_BASE__+59
#define _F2                 __EXT_KEY_BASE__+60
#define _F3                 __EXT_KEY_BASE__+61
#define _F4                 __EXT_KEY_BASE__+62
#define _F5                 __EXT_KEY_BASE__+63
#define _F6                 __EXT_KEY_BASE__+64
#define _F7                 __EXT_KEY_BASE__+65
#define _F8                 __EXT_KEY_BASE__+66
#define _F9                 __EXT_KEY_BASE__+67
#define _F10                __EXT_KEY_BASE__+68
#define _Home               __EXT_KEY_BASE__+71
#define _Up                 __EXT_KEY_BASE__+72
#define _PgUp               __EXT_KEY_BASE__+73
#define _AltNumMinus        __EXT_KEY_BASE__+74
#define _Left               __EXT_KEY_BASE__+75
#define _Gold               __EXT_KEY_BASE__+76	  /* "5" on numeric keypad */
#define _Right              __EXT_KEY_BASE__+77
#define _AltNumPlus         __EXT_KEY_BASE__+78
#define _End                __EXT_KEY_BASE__+79
#define _Down               __EXT_KEY_BASE__+80
#define _PgDn               __EXT_KEY_BASE__+81
#define _Insert             __EXT_KEY_BASE__+82
#define _Delete             __EXT_KEY_BASE__+83
#define _ShiftF1            __EXT_KEY_BASE__+84
#define _ShiftF2            __EXT_KEY_BASE__+85
#define _ShiftF3            __EXT_KEY_BASE__+86
#define _ShiftF4            __EXT_KEY_BASE__+87
#define _ShiftF5            __EXT_KEY_BASE__+88
#define _ShiftF6            __EXT_KEY_BASE__+89
#define _ShiftF7            __EXT_KEY_BASE__+90
#define _ShiftF8            __EXT_KEY_BASE__+91
#define _ShiftF9            __EXT_KEY_BASE__+92
#define _ShiftF10           __EXT_KEY_BASE__+93
#define _CtrlF1             __EXT_KEY_BASE__+94
#define _CtrlF2             __EXT_KEY_BASE__+95
#define _CtrlF3             __EXT_KEY_BASE__+96
#define _CtrlF4             __EXT_KEY_BASE__+97
#define _CtrlF5             __EXT_KEY_BASE__+98
#define _CtrlF6             __EXT_KEY_BASE__+99
#define _CtrlF7             __EXT_KEY_BASE__+100
#define _CtrlF8             __EXT_KEY_BASE__+101
#define _CtrlF9             __EXT_KEY_BASE__+102
#define _CtrlF10            __EXT_KEY_BASE__+103
#define _AltF1              __EXT_KEY_BASE__+104
#define _AltF2              __EXT_KEY_BASE__+105
#define _AltF3              __EXT_KEY_BASE__+106
#define _AltF4              __EXT_KEY_BASE__+107
#define _AltF5              __EXT_KEY_BASE__+108
#define _AltF6              __EXT_KEY_BASE__+109
#define _AltF7              __EXT_KEY_BASE__+110
#define _AltF8              __EXT_KEY_BASE__+111
#define _AltF9              __EXT_KEY_BASE__+112
#define _AltF10             __EXT_KEY_BASE__+113
#define _CtrlSysReq         __EXT_KEY_BASE__+114
#define _CtrlLeft           __EXT_KEY_BASE__+115
#define _CtrlRight          __EXT_KEY_BASE__+116
#define _CtrlEnd            __EXT_KEY_BASE__+117
#define _CtrlPgDn           __EXT_KEY_BASE__+118
#define _CtrlHome           __EXT_KEY_BASE__+119
#define _Alt1               __EXT_KEY_BASE__+120
#define _Alt2               __EXT_KEY_BASE__+121
#define _Alt3               __EXT_KEY_BASE__+122
#define _Alt4               __EXT_KEY_BASE__+123
#define _Alt5               __EXT_KEY_BASE__+124
#define _Alt6               __EXT_KEY_BASE__+125
#define _Alt7               __EXT_KEY_BASE__+126
#define _Alt8               __EXT_KEY_BASE__+127
#define _Alt9               __EXT_KEY_BASE__+128
#define _Alt0               __EXT_KEY_BASE__+129
#define _AltPlus            __EXT_KEY_BASE__+130
#define _AltMinus           __EXT_KEY_BASE__+131
#define _CtrlPgUp           __EXT_KEY_BASE__+132
#define _F11                __EXT_KEY_BASE__+133
#define _F12                __EXT_KEY_BASE__+134
#define _ShiftF11           __EXT_KEY_BASE__+135
#define _ShiftF12           __EXT_KEY_BASE__+136
#define _CtrlF11            __EXT_KEY_BASE__+137
#define _CtrlF12            __EXT_KEY_BASE__+138
#define _AltF11             __EXT_KEY_BASE__+139
#define _AltF12             __EXT_KEY_BASE__+140
#define _CtrlUp             __EXT_KEY_BASE__+141
#define _CtrlNumMinus       __EXT_KEY_BASE__+142
#define _CtrlGold           __EXT_KEY_BASE__+143      /* Ctrl - "5" on numeric keypad */
#define _CtrlNumPlus        __EXT_KEY_BASE__+144
#define _CtrlDown           __EXT_KEY_BASE__+145
#define _CtrlInsert         __EXT_KEY_BASE__+146
#define _CtrlNumIns         __EXT_KEY_BASE__+146
#define _CtrlDelete         __EXT_KEY_BASE__+147
#define _CtrlNumDel         __EXT_KEY_BASE__+147
#define _CtrlTab            __EXT_KEY_BASE__+148
#define _CtrlNumSlash       __EXT_KEY_BASE__+149
#define _CtrlNumAsterisk    __EXT_KEY_BASE__+150
#define _AltHome            __EXT_KEY_BASE__+151
#define _AltUp              __EXT_KEY_BASE__+152
#define _AltPageUp          __EXT_KEY_BASE__+153
#define _AltLeft            __EXT_KEY_BASE__+155
#define _AltRight           __EXT_KEY_BASE__+157
#define _AltEnd             __EXT_KEY_BASE__+159
#define _AltDown            __EXT_KEY_BASE__+160
#define _AltPageDn          __EXT_KEY_BASE__+161
#define _AltInsert          __EXT_KEY_BASE__+162
#define _AltDelete          __EXT_KEY_BASE__+163
#define _AltNumSlash        __EXT_KEY_BASE__+164
#define _AltTab             __EXT_KEY_BASE__+165
#define _AltNumEnter        __EXT_KEY_BASE__+166
#define _ShiftInsert        __EXT_KEY_BASE__+167
#define _AltSpace           __EXT_KEY_BASE__+168
#define _Scroll1Up          __EXT_KEY_BASE__+169
#define _Scroll1Down        __EXT_KEY_BASE__+170
#define _ScrollPgUp         __EXT_KEY_BASE__+171
#define _ScrollPgDown       __EXT_KEY_BASE__+172
#define _ShiftHome          __EXT_KEY_BASE__+173
#define _ShiftEnd           __EXT_KEY_BASE__+174
#define _ShiftLeft          __EXT_KEY_BASE__+175
#define _ShiftRight         __EXT_KEY_BASE__+176
#define _ShiftPgUp          __EXT_KEY_BASE__+177
#define _ShiftPgDn          __EXT_KEY_BASE__+178
#define _ShiftUp            __EXT_KEY_BASE__+179
#define _ShiftDown          __EXT_KEY_BASE__+180
#define _ShiftEnter         __EXT_KEY_BASE__+181
#define _ShiftDelete        __EXT_KEY_BASE__+182

#endif
