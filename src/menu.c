#include "fly/fly.h"

#include <string.h>
#include <stdlib.h>

#include "menu.h"

struct menu_group   mn[64];
int                 ngroup;
int *keytable0, n_keytable0;

int            menu_enabled = TRUE;
struct _item   *fly_active_menu = NULL;

static void parse_item (struct _item *I, char *line, int n);

/* ------------------------------------------------------------- */

static void parse_item (struct _item *I, char *line, int n)
{
    char *p, *p1, *type;

    // sanity check
    if (line[0] != '{') fly_error ("line %d: menu item does not start with {\n", n);

    // look for end of item definition
    p = str_index1 (line, '}');
    if (p == NULL) fly_error ("line %d: menu item does not have closing }\n", n);
    *p = '\0';

    // initialize all fields to zero values
    I->text = "";
    I->type = ITEM_TYPE_END;
    I->body.action = 0;
    I->keydesc = "";
    I->pos = 0;
    I->enabled = 1;
    I->state = FALSE;
    I->hotkey = 0;
    I->hkpos = 0;

    // extract type
    type = line + 1;
    p = str_index1 (type, ',');
    if (p != NULL) *p = '\0';
    str_strip2 (type, " \t");
    
    if (strcmp (type, "SEP") == 0)
        I->type = ITEM_TYPE_SEP;
    else if (strcmp (type, "ACTION") == 0)
        I->type = ITEM_TYPE_ACTION;
    else if (strcmp (type, "SWITCH") == 0)
        I->type = ITEM_TYPE_SWITCH;
    else if (strcmp (type, "SUBMENU") == 0)
        I->type = ITEM_TYPE_SUBMENU;
    else
        fly_error ("line %d: unknown type `%s'\n", n, type);
    if (I->type == ITEM_TYPE_SEP) return;

    // process title
    if (p == NULL)
        fly_error ("line %d: missing item text\n", n);
    p++;
    while (*p && (*p == ' ')) p++;
    if (*p == '\0')
        fly_error ("line %d: missing item text\n", n);
    // now p should point to first " of item text
    if (*p != '"')
        fly_error ("line %d: item text does not start with double quote\n", n);
    p++;
    p1 = strchr (p, '"');
    if (p1 == NULL)
        fly_error ("line %d: missing closing double quote\n", n);
    *p1 = '\0';
    I->text = strdup (p);

    // process value
    p = p1 + 1; // points to next char after title
    while (*p && (*p == ' ')) p++;
    if (*p == '\0' || *p != ',')
        fly_error ("line %d: missing value\n", n);
    p++;
    str_strip2 (p, " \t");
    if (I->type == ITEM_TYPE_ACTION || I->type == ITEM_TYPE_SWITCH)
    {
        I->body.action = atoi (p);
        if (I->body.action == 0)
            fly_error ("line %d: value is zero, empty or not a number\n", n);
    }
    else if (I->type == ITEM_TYPE_SUBMENU)
    {
        I->body.submenu = (struct _item *) strdup (p);
    }
    else
        fly_error ("internal error in parse_item()\n");
}

/* ------------------------------------------------------------- */

#define OUTSIDE    0
#define GROUP_NAME 1
#define INSIDE     2

struct _item *menu_load (char *menu_filename, int *keytable, int n_keytable)
{
    char  line[256], buffer[128], *p;
    int   nitem, nline, state, i, j, k, pos;
    FILE  *f;
    struct _item *L, *M, *main_menu = NULL;

    keytable0   = keytable;
    n_keytable0 = n_keytable;
    
    // read file line-by-line and analyze it
    
    f = fopen (menu_filename, "r");
    if (f == NULL) fly_error ("Cannot open %s\n\n", menu_filename);

    state = OUTSIDE;
    ngroup = 0;
    nline  = 0;
    nitem  = 0;
    while (fgets (line, sizeof(line), f) != NULL)
    {
        nline++;
        if (line[0] == '#') continue; // skip comments
        
        str_strip2 (line, " \r\n\t");
        if (line[0] == '\0') continue; // skip empty lines

        switch (state)
        {
        case OUTSIDE:
            if (line[0] == '{')
                fly_error ("line %d: missing group name\n", nline);
            mn[ngroup].name = strdup (line);
            mn[ngroup].I = malloc (sizeof (struct _item) * MAX_ITEMS_PER_MENU);
            state = GROUP_NAME;
            break;
            
        case GROUP_NAME:
            if (strcmp (line, "{") != 0)
                fly_error ("line %d: group name must be followed by {\n", nline);
            nitem  = 0;
            state = INSIDE;
            break;
            
        case INSIDE:
            if (strcmp (line, "}") == 0)
            {
                mn[ngroup].I[nitem].type = ITEM_TYPE_END;
                nitem++;
                mn[ngroup].I = realloc (mn[ngroup].I, sizeof (struct _item) * nitem);
                ngroup++;
                state = OUTSIDE;
            }
            else
            {
                parse_item (&(mn[ngroup].I[nitem]), line, nline);
                nitem++;
            }
            break;
        }
    }
    fclose (f);

    // fix submenu pointers
    for (i=0; i<ngroup; i++)
    {
        j = 0;
        while (mn[i].I[j].type != ITEM_TYPE_END)
        {
            if (mn[i].I[j].type == ITEM_TYPE_SUBMENU)
            {
                for (k = 0; k<ngroup; k++)
                    if (strcmp (mn[k].name, (char *)(mn[i].I[j].body.submenu)) == 0)
                    {
                        free (mn[i].I[j].body.submenu);
                        mn[i].I[j].body.submenu = mn[k].I;
                        break;
                    }
                if (k == ngroup)
                    fly_error ("group `%s' is missing\n", (char *)(mn[i].I[j].body.submenu));
            }
            j++;
        }
    }

    // look for main menu
    for (i=0; i<ngroup; i++)
        if (strcmp (mn[i].name, "main") == 0)
        {
            main_menu = mn[i].I;
            break;
        }
    if (main_menu == NULL)
        fly_error ("group `main' is missing\n");

    // set positions for main linear menu
    pos = 1;
    L = main_menu;
    while (L->type != ITEM_TYPE_END)
    {
        L->pos = pos;
        if (L->type == ITEM_TYPE_SEP) L->text = "|";
        pos += strlen (L->text) + 2;
        L++;
    }
    
    // process entries, setting sizes, key names etc.
    for (i=0; i<ngroup; i++)
    {
        M = mn[i].I;
        while (M->type != ITEM_TYPE_END)
        {
            switch (M->type)
            {
            case ITEM_TYPE_ACTION:
            case ITEM_TYPE_SWITCH:
                // look for key name corresponding the action
                buffer[0] = '\0';
                for (j=0; j<n_keytable0; j++)
                {
                    if (keytable0[j] == M->body.action)
                    {
                        p = fly_keyname2 (j);
                        if (p == NULL) continue;
                        if (strlen (buffer) != 0) strcat (buffer, "/");
                        strcat (buffer, p);
                    }
                }
                M->keydesc = strdup (buffer);
                break;
            }
            M++;
        }
    }

    // set hotkeys and their positions
    for (i=0; i<ngroup; i++)
    {
        j = 0;
        while (mn[i].I[j].type != ITEM_TYPE_END)
        {
            if (mn[i].I[j].type == ITEM_TYPE_SUBMENU ||
                mn[i].I[j].type == ITEM_TYPE_SWITCH ||
                mn[i].I[j].type == ITEM_TYPE_ACTION)
            {
                p = strchr (mn[i].I[j].text, '&');
                if (p != NULL)
                {
                    mn[i].I[j].hotkey = *(p+1);
                    mn[i].I[j].hkpos = p-mn[i].I[j].text;
                    str_delete (mn[i].I[j].text, p);
                }
            }
            j++;
        }
    }

    // check for duplicate hotkeys
    for (i=0; i<ngroup; i++)
    {
        j = 0;
        while (mn[i].I[j].type != ITEM_TYPE_END)
        {
            for (k=0; k<j; k++)
                if (mn[i].I[j].hotkey && mn[i].I[k].hotkey &&
                    tolower1(mn[i].I[j].hotkey) == tolower1(mn[i].I[k].hotkey))
                    fly_error ("duplicate hotkey: `%s' and `%s'\n",
                               mn[i].I[j].text, mn[i].I[k].text);
            j++;
        }
    }

    return main_menu;
}

/* ------------------------------------------------------------- */

void menu_unload (struct _item *L)
{
    
}
