#include "fly/fly.h"

#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------------------------- */

#define E __EXT_KEY_BASE__
struct _kv {char *n; int v;};
static struct _kv kv[] =
{
    // alphanumeric keys

    {"A", 'A'}, {"B", 'B'}, {"C", 'C'}, {"D", 'D'}, {"E", 'E'}, {"F", 'F'},
    {"G", 'G'}, {"H", 'H'}, {"I", 'I'}, {"J", 'J'}, {"K", 'K'}, {"L", 'L'},
    {"M", 'M'}, {"N", 'N'}, {"O", 'O'}, {"P", 'P'}, {"Q", 'Q'}, {"R", 'R'},
    {"S", 'S'}, {"T", 'T'}, {"U", 'U'}, {"V", 'V'}, {"W", 'W'}, {"X", 'X'},
    {"Y", 'Y'}, {"Z", 'Z'},
    
    {"a", 'a'}, {"b", 'b'}, {"c", 'c'}, {"d", 'd'}, {"e", 'e'}, {"f", 'f'},
    {"g", 'g'}, {"h", 'h'}, {"i", 'i'}, {"j", 'j'}, {"k", 'k'}, {"l", 'l'},
    {"m", 'm'}, {"n", 'n'}, {"o", 'o'}, {"p", 'p'}, {"q", 'q'}, {"r", 'r'},
    {"s", 's'}, {"t", 't'}, {"u", 'u'}, {"v", 'v'}, {"w", 'w'}, {"x", 'x'},
    {"y", 'y'}, {"z", 'z'},

    {"0", '0'}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'}, {"5", '5'},
    {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'},
    
    // Ctrl+letters
    
    {"ctrl-a",  1}, {"ctrl-b",  2}, {"ctrl-c",  3}, {"ctrl-d",  4},
    {"ctrl-e",  5}, {"ctrl-f",  6}, {"ctrl-g",  7}, {"ctrl-h",  8},
    {"ctrl-i",  9}, {"ctrl-j", 10}, {"ctrl-k", 11}, {"ctrl-l", 12},
    {"ctrl-m", 13}, {"ctrl-n", 14}, {"ctrl-o", 15}, {"ctrl-p", 16},
    {"ctrl-q", 17}, {"ctrl-r", 18}, {"ctrl-s", 19}, {"ctrl-t", 20},
    {"ctrl-u", 21}, {"ctrl-v", 22}, {"ctrl-w", 23}, {"ctrl-x", 24},
    {"ctrl-y", 25}, {"ctrl-z", 26},
        
    // regular keys under names
        
    {"esc", 27},

    {"approx", 126}, {"backapostrophe", 96}, {"exclamation", 33},
    {"at", 64}, {"grate", 35}, {"dollar", 36}, {"percentsign", 37},
    {"caret", 94}, {"ampersand", 38}, {"asterisk", 42},
    {"lparenth", 40}, {"rparenth", 41}, {"minus", 45}, {"underline", 95},
    {"plus", 43}, {"equalsign", 61}, {"vertline", 124}, {"backslash", 92}, 

    {"backspace", 8}, {"tab", 9}, {"enter", 13}, {"space", 32},
        
    {"lrbracket", 91}, {"rrbracket", 93},
    {"lcbracket", 123}, {"rcbracket", 125},
    {"colon", 58}, {"semicolon", 59}, 
    {"doublequote", 34}, {"apostrophe", 39},
    {"lessthan", 60}, {"comma", 44}, 
    {"greaterthan", 62}, {"period", 46},
    {"questionmark", 63}, {"slash", 47},
    
    // ctrl+some keys

    {"ctrl-tab", E+148}, {"ctrl-enter", 10},
    {"ctrl-lrbracket", 27}, {"ctrl-lcbracket", 27},
    {"ctrl-backslash", 28}, {"ctrl-vertline", 28},
    {"ctrl-rrbracket", 29}, {"ctrl-rcbracket", 29},
    {"ctrl-6", 30}, {"ctrl-caret", 30}, 
    {"ctrl-minus", 31}, {"ctrl-underline", 31},
    {"ctrl-backspace", 127},
    {"ctrl-2", E+3}, {"ctrl-at", E+3},

    // alt+letters
        
    {"alt-a", E+30}, {"alt-b", E+48}, {"alt-c", E+46}, {"alt-d", E+32},
    {"alt-e", E+18}, {"alt-f", E+33}, {"alt-g", E+34}, {"alt-h", E+35},
    {"alt-i", E+23}, {"alt-j", E+36}, {"alt-k", E+37}, {"alt-l", E+38},
    {"alt-m", E+50}, {"alt-n", E+49}, {"alt-o", E+24}, {"alt-p", E+25},
    {"alt-q", E+16}, {"alt-r", E+19}, {"alt-s", E+31}, {"alt-t", E+20},
    {"alt-u", E+22}, {"alt-v", E+47}, {"alt-w", E+17}, {"alt-x", E+45},
    {"alt-y", E+21}, {"alt-z", E+44},
        
    {"alt-1", E+120}, {"alt-exclamation", E+120},
    {"alt-2", E+121}, {"alt-at", E+121},
    {"alt-3", E+122}, {"alt-grate", E+122},
    {"alt-4", E+123}, {"alt-dollar", E+123},
    {"alt-5", E+124}, {"alt-percentsign", E+124},
    {"alt-6", E+125}, {"alt-caret", E+125},
    {"alt-7", E+126}, {"alt-ampersand", E+126},
    {"alt-8", E+127}, {"alt-asterisk", E+127},
    {"alt-9", E+128}, {"alt-lparenth", E+128},
    {"alt-0", E+129}, {"alt-rparenth", E+129},
        
    // alt+some keys
        
    {"alt-esc", E+1},  /* :-} */
        
    {"alt-backapostrophe", E+41}, {"alt-approx", E+41},
    {"alt-minus", E+131}, {"alt-underline", E+131},
    {"alt-plus", E+130}, {"alt-equalsign", E+130},
    {"alt-backslash", E+43}, {"alt-vertline", E+43},
    {"alt-lrbracket", E+26}, {"alt-lcbracket", E+26},
    {"alt-rrbracket", E+27}, {"alt-rcbracket", E+27},
    {"alt-semicolon", E+39}, {"alt-colon", E+39},
    {"alt-apostrophe", E+40}, {"alt-doublequote", E+40},
    {"alt-comma", E+51}, {"alt-lessthan", E+51},
    {"alt-period", E+52}, {"alt-greaterthan", E+52},
    {"alt-slash", E+53}, {"alt-questionmark", E+53},
        
    {"alt-tab", E+165}, {"alt-backspace", E+14}, {"alt-enter", E+28},
        
    // shift+some keys
        
    {"shift-tab", E+15}, {"shift-insert", E+167},
        
    // function keys
        
    {"f1", E+59}, {"f2", E+60}, {"f3", E+61}, {"f4", E+62}, {"f5",  E+63},
    {"f6", E+64}, {"f7", E+65}, {"f8", E+66}, {"f9", E+67}, {"f10", E+68},
    {"f11", E+133}, {"f12", E+134},
        
    {"ctrl-f1",  E+94},  {"ctrl-f2",  E+95},  {"ctrl-f3",  E+96},
    {"ctrl-f4",  E+97},  {"ctrl-f5",  E+98},  {"ctrl-f6",  E+99},
    {"ctrl-f7",  E+100}, {"ctrl-f8",  E+101}, {"ctrl-f9",  E+102},
    {"ctrl-f10", E+103}, {"ctrl-f11", E+137}, {"ctrl-f12", E+138},

    {"shift-f1",  E+84}, {"shift-f2",  E+85},  {"shift-f3",  E+86},
    {"shift-f4",  E+87}, {"shift-f5",  E+88},  {"shift-f6",  E+89},
    {"shift-f7",  E+90}, {"shift-f8",  E+91},  {"shift-f9",  E+92},
    {"shift-f10", E+93}, {"shift-f11", E+135}, {"shift-f12", E+136},
        
    {"alt-f1",  E+104}, {"alt-f2", E+105},  {"alt-f3", E+106},
    {"alt-f4",  E+107}, {"alt-f5", E+108},  {"alt-f6", E+109},
    {"alt-f7",  E+110}, {"alt-f8",  E+111}, {"alt-f9",  E+112},
    {"alt-f10", E+113}, {"alt-f11", E+139}, {"alt-f12", E+140},
        
    // arrow keys
        
    {"insert", E+82}, {"delete", E+83}, 
    {"home",   E+71}, {"end",    E+79},
    {"pgup",   E+73}, {"pgdn",   E+81},
    {"left",   E+75}, {"right",  E+77},
    {"up",     E+72}, {"down",   E+80},
        
    {"alt-insert", E+162}, {"alt-delete", E+163}, 
    {"alt-home",   E+151}, {"alt-end",    E+159},
    {"alt-pgup",   E+153}, {"alt-pgdn",   E+161},
    {"alt-left",   E+155}, {"alt-right",  E+157},
    {"alt-up",     E+152}, {"alt-down",   E+160},
        
    {"ctrl-insert", E+146}, {"ctrl-delete", E+147},
    {"ctrl-home",   E+119}, {"ctrl-end",    E+117},
    {"ctrl-pgup",   E+132}, {"ctrl-pgdn",   E+118},
    {"ctrl-left",   E+115}, {"ctrl-right",  E+116},
    {"ctrl-up",     E+141}, {"ctrl-down",   E+145},
        
    // numeric keypad
        
    {"gold", E+76}, {"numinsert", E+82}, {"numdelete", E+83},
    {"numslash", 47}, {"numasterisk", 42}, {"numminus", 45},
    {"numplus", 43}, {"numenter", 13},
    {"alt-numslash", E+164}, {"alt-numasterisk", E+55}, 
    {"alt-numminus", E+74}, {"alt-numplus", E+78}, 
    {"alt-numenter", E+166},
    {"ctrl-numslash", E+149}, {"ctrl-numasterisk", E+150},
    {"ctrl-numminus", E+142}, {"ctrl-numplus", E+144},
    {"ctrl-numenter", 10}, {"ctrl-gold", E+143}
};

/* --------------------------------------------------------------------- */

int fly_keyvalue (char *s)
{
    int i, nkv = sizeof(kv)/sizeof(struct _kv);
    
    for (i=0; i<nkv; i++)
        if (stricmp1 (s, kv[i].n) == 0) return kv[i].v;
    
    return -1;
}

/* --------------------------------------------------------------------- */

char **fly_keyname (int key)
{
    int i, n=0, nkv = sizeof(kv)/sizeof(struct _kv);
    static char *names[16];
    
    for (i=0; i<16; i++)
        names[i] = NULL;
    
    for (i=0; i<nkv; i++)
        if (kv[i].v == key)
            names[n++] = kv[i].n;
    
    return names;
}

/* --------------------------------------------------------------------- */

static struct _kv kv2[] =
{
    // Ctrl+letters
    
    {"Ctrl-A",  1},
    {"Ctrl-B",  2},
    {"Ctrl-C",  3},
    {"Ctrl-D",  4},
    {"Ctrl-E",  5},
    {"Ctrl-F",  6},
    {"Ctrl-G",  7},
    //{"Ctrl-H",  8},
    {"Backspace", 8},
    //{"Ctrl-I",  9},
    {"Tab", 9},
    {"Ctrl-J", 10},
    //{"Ctrl-Enter", 10},
    {"Ctrl-K", 11},
    {"Ctrl-L", 12},
    //{"Ctrl-M", 13},
    {"Enter", 13},
    {"Ctrl-N", 14},
    {"Ctrl-O", 15},
    {"Ctrl-P", 16},
    {"Ctrl-Q", 17},
    {"Ctrl-R", 18},
    {"Ctrl-S", 19},
    {"Ctrl-T", 20},
    {"Ctrl-U", 21},
    {"Ctrl-V", 22},
    {"Ctrl-W", 23},
    {"Ctrl-X", 24},
    {"Ctrl-Y", 25},
    {"Ctrl-Z", 26},
    
    //{"Ctrl-[", 27},
    {"Esc",   27},
    
    {"Ctrl-\\", 28}, 
    {"Ctrl-]",  29},
    {"Ctrl-6",  30},
    {"Ctrl-minus", 31},
    
    {"Space", 32},
    {"!",     33},
    {"\"",    34},
    {"#",     35},
    {"$",     36},
    {"%",     37},
    {"&",     38},
    {"'",     39},
    {"(",     40},
    {")",     41},
    {"*",     42},
    {"+",     43},
    {",",     44},
    {"-",     45},
    {".",     46},
    {"/",     47},
    
    // alphanumeric keys
    
    {"0", '0'}, /* 48 */
    {"1", '1'}, /* 49 */
    {"2", '2'}, /* 50 */
    {"3", '3'}, /* 51 */
    {"4", '4'}, /* 52 */
    {"5", '5'}, /* 53 */
    {"6", '6'}, /* 54 */
    {"7", '7'}, /* 55 */
    {"8", '8'}, /* 56 */
    {"9", '9'}, /* 57 */

    {":", 58},
    {";", 59},
    {"<", 60},
    {"=", 61},
    {">", 62},
    {"?", 63},
    {"@", 64},

    {"Shift-A", 'A'}, /* 65 */
    {"Shift-B", 'B'}, /* 66 */
    {"Shift-C", 'C'}, /* 67 */
    {"Shift-D", 'D'}, /* 68 */
    {"Shift-E", 'E'}, /* 69 */
    {"Shift-F", 'F'}, /* 70 */
    {"Shift-G", 'G'}, /* 71 */
    {"Shift-H", 'H'}, /* 72 */
    {"Shift-I", 'I'}, /* 73 */
    {"Shift-J", 'J'}, /* 74 */
    {"Shift-K", 'K'}, /* 75 */
    {"Shift-L", 'L'}, /* 76 */
    {"Shift-M", 'M'}, /* 77 */
    {"Shift-N", 'N'}, /* 78 */
    {"Shift-O", 'O'}, /* 79 */
    {"Shift-P", 'P'}, /* 80 */
    {"Shift-Q", 'Q'}, /* 81 */
    {"Shift-R", 'R'}, /* 82 */
    {"Shift-S", 'S'}, /* 83 */
    {"Shift-T", 'T'}, /* 84 */
    {"Shift-U", 'U'}, /* 85 */
    {"Shift-V", 'V'}, /* 86 */
    {"Shift-W", 'W'}, /* 87 */
    {"Shift-X", 'X'}, /* 88 */
    {"Shift-Y", 'Y'}, /* 89 */
    {"Shift-Z", 'Z'}, /* 90 */

    {"[",  91},
    {"\\", 92},
    {"]",  93},
    {"^",  94},
    {"_",  95},
    {"`",  96},

    {"a", 'a'},  /* 97 */
    {"b", 'b'},  /* 98 */
    {"c", 'c'},  /* 99 */
    {"d", 'd'},  /* 100 */
    {"e", 'e'},  /* 101 */
    {"f", 'f'},  /* 102 */
    {"g", 'g'},  /* 103 */
    {"h", 'h'},  /* 104 */
    {"i", 'i'},  /* 105 */
    {"j", 'j'},  /* 106 */
    {"k", 'k'},  /* 107 */
    {"l", 'l'},  /* 108 */
    {"m", 'm'},  /* 109 */
    {"n", 'n'},  /* 110 */
    {"o", 'o'},  /* 111 */
    {"p", 'p'},  /* 112 */
    {"q", 'q'},  /* 113 */
    {"r", 'r'},  /* 114 */
    {"s", 's'},  /* 115 */
    {"t", 't'},  /* 116 */
    {"u", 'u'},  /* 117 */
    {"v", 'v'},  /* 118 */
    {"w", 'w'},  /* 119 */
    {"x", 'x'},  /* 120 */
    {"y", 'y'},  /* 121 */
    {"z", 'z'},  /* 122 */

    {"{", 123},
    {"|", 124},
    {"}", 125},
    {"~", 126},
    
    {"Ctrl-Backspace", 127},

    // -----------------------------------------------------------
    // Extended keys
    // -----------------------------------------------------------
    
    {"Alt-Esc", E+1},  /* :-} */
    {"Ctrl-2", E+3}, 
    {"Alt-Backspace", E+14},
    {"Shift+Tab", E+15},
    
    // alt+letters
        
    {"Alt-Q", E+16},
    {"Alt-W", E+17},
    {"Alt-E", E+18},
    {"Alt-R", E+19},
    {"Alt-T", E+20},
    {"Alt-Y", E+21},
    {"Alt-U", E+22},
    {"Alt-I", E+23},
    {"Alt-O", E+24},
    {"Alt-P", E+25},
    {"Alt-[", E+26},
    {"Alt-]", E+27},
    {"Alt-Enter", E+28},
    {"Alt-A", E+30},
    {"Alt-S", E+31},
    {"Alt-D", E+32},
    {"Alt-F", E+33},
    {"Alt-G", E+34},
    {"Alt-H", E+35},
    {"Alt-J", E+36},
    {"Alt-K", E+37},
    {"Alt-L", E+38},
    {"Alt-;", E+39},
    {"Alt-'", E+40},
    {"Alt-`", E+41},
    {"Alt-\\", E+43},
    {"Alt-Z", E+44},
    {"Alt-X", E+45},
    {"Alt-C", E+46},
    {"Alt-V", E+47},
    {"Alt-B", E+48},
    {"Alt-N", E+49},
    {"Alt-M", E+50},
    {"Alt-,", E+51},
    {"Alt-.", E+52},
    {"Alt-/", E+53},

    {"Alt-num*", E+55},
    
    {"F1", E+59},
    {"F2", E+60},
    {"F3", E+61},
    {"F4", E+62},
    {"F5",  E+63},
    {"F6", E+64},
    {"F7", E+65},
    {"F8", E+66},
    {"F9", E+67},
    {"F10", E+68},
    
    {"Home",   E+71},
    {"Up",     E+72},
    {"PgUp",   E+73},
    {"Alt-num-", E+74},
    {"<-",     E+75},
    {"Gold",   E+76},
    {"->",     E+77},
    {"Alt-num+", E+78},
    {"End",    E+79},
    {"Down",   E+80},
    {"PgDn",   E+81},
    {"Insert", E+82},
    {"Delete", E+83},

    {"Shift-F1",  E+84},
    {"Shift-F2",  E+85},
    {"Shift-F3",  E+86},
    {"Shift-F4",  E+87},
    {"Shift-F5",  E+88},
    {"Shift-F6",  E+89},
    {"Shift-F7",  E+90},
    {"Shift-F8",  E+91},
    {"Shift-F9",  E+92},
    {"Shift-F10", E+93},
    
    {"Ctrl-F1",  E+94},
    {"Ctrl-F2",  E+95},
    {"Ctrl-F3",  E+96},
    {"Ctrl-F4",  E+97},
    {"Ctrl-F5",  E+98},
    {"Ctrl-F6",  E+99},
    {"Ctrl-F7",  E+100},
    {"Ctrl-F8",  E+101},
    {"Ctrl-F9",  E+102},
    {"Ctrl-F10", E+103},
    
    {"Alt-F1",  E+104},
    {"Alt-F2",  E+105},
    {"Alt-F3",  E+106},
    {"Alt-F4",  E+107},
    {"Alt-F5",  E+108},
    {"Alt-F6",  E+109},
    {"Alt-F7",  E+110},
    {"Alt-F8",  E+111},
    {"Alt-F9",  E+112},
    {"Alt-F10", E+113},
    
    {"Ctrl-<-",   E+115},
    {"Ctrl-->",  E+116},
    {"Ctrl-end",    E+117},
    {"Ctrl-PgDn",   E+118},
    {"Ctrl-Home",   E+119},
    
    {"Alt-1", E+120}, 
    {"Alt-2", E+121}, 
    {"Alt-3", E+122}, 
    {"Alt-4", E+123}, 
    {"Alt-5", E+124}, 
    {"Alt-6", E+125}, 
    {"Alt-7", E+126}, 
    {"Alt-8", E+127}, 
    {"Alt-9", E+128}, 
    {"Alt-0", E+129}, 
        
    {"Alt-+", E+130},
    {"Alt-minus", E+131},
    {"Ctrl-PgUp",   E+132},
    
    {"F11", E+133},
    {"F12", E+134},
    {"Shift-F11", E+135},
    {"Shift-F12", E+136},
    {"Ctrl-F11", E+137},
    {"Ctrl-F12", E+138},
    {"Alt-F11", E+139},
    {"Alt-F12", E+140},
    
    {"Ctrl-Up",     E+141},
    {"Ctrl-num-", E+142},
    {"Ctrl-Gold", E+143},
    {"Ctrl-num+", E+144},
    {"Ctrl-Down",   E+145},
    {"Ctrl-Insert", E+146},
    {"Ctrl-Delete", E+147},
    {"Ctrl-Tab", E+148},
    {"Ctrl-num/", E+149},
    {"Ctrl-num*", E+150},
    {"Alt-Home",   E+151},
    {"Alt-Up",     E+152},
    {"Alt-PgUp",   E+153},
    {"Alt-<-",   E+155},
    {"Alt-->",  E+157},
    {"Alt-End",    E+159},
    {"Alt-Down",   E+160},
    {"Alt-PgDn",   E+161},
    {"Alt-Insert", E+162},
    {"Alt-Delete", E+163},
    {"Alt-num/",   E+164},
    {"Alt-Tab",      E+165},
    {"Alt-numenter", E+166},
    {"Shift-Insert", E+167},
};

/* --------------------------------------------------------------------- */

char *fly_keyname2 (int key)
{
    int i, nkv = sizeof(kv2)/sizeof(struct _kv);
    
    for (i=0; i<nkv; i++)
        if (kv2[i].v == key)
            return kv2[i].n;
    return NULL;
}

/* --------------------------------------------------------------------- */

int getkey (int interval)
{
    int     message;
    double  rem, target;

    // prevent strange behaviour due to programmer mistakes
    if (interval < 0) interval = -1;
        
    switch (interval)
    {
    case -1:
        while (1)
        {
            message = getmessage (-1);
            if (IS_KEY (message) && message > 0) break;
        }
        break;

    case 0:
        while (TRUE)
        {
            message = getmessage (0);
            if (message == 0) break;
            if (IS_KEY (message)) break;
        }
        break;

    default:
        target = clock1 () + ((double)interval)/1000.;
        message = -1;
        while (TRUE)
        {
            rem = target - clock1();
            if (rem <= 0.) break;
            message = getmessage ((int)(rem*1000.));
            if (IS_KEY(message) && message > 0) break;
        }
    }
    
    return message;
}
