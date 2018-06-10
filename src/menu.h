
#define MAX_ITEMS_PER_MENU       128

struct menu_group
{
    struct _item  *I;
    char          *name;
};

extern struct menu_group   mn[64];
extern int                 ngroup;
extern int *keytable0, n_keytable0;
extern int menu_enabled;
