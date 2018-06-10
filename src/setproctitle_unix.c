
static char  *argv_start;
static int   argv_length; // includes terminating NULL

/* ------------------------------------------------------------- */

static char *current_title = NULL;

void set_window_name (char *format, ...)
{
    char    buf[2048];
    va_list ap;
    
    // compose new title
    snprintf1 (buf, sizeof(buf), "%s: ", fl_opt.appname);
    va_start (ap, format);
    vsnprintf1 (buf+strlen(buf), sizeof(buf)-strlen(buf), format, ap);
    va_end (ap);

    if (current_title != NULL) free (current_title);
    current_title = strdup (buf);

#if defined(__FreeBSD__)
    setproctitle ("-%s", buf);
#else
    /* clear the place where new title will be */
    memset (argv_start, ' ', argv_length);
    
    // copy title to new place
    strncpy (argv_start, buf, argv_length);

    // and make sure it's NULL-terminated
    argv_start[argv_length-1] = '\0';
#endif
}

/* ------------------------------------------------------------- */

char *get_window_name (void)
{
    if (current_title == NULL) return strdup ("FLY application");
    
    return strdup (current_title);
}

/* ------------------------------------------------------------- */

void fly_process_args (int *argc, char **argv[], char ***envp)
{
    int           n_vars, i;
    extern char   **environ;
    char          **argv1;

    // save argv[] array for late option processing
    argv1 = malloc ((*argc+1)*sizeof (void *));
    for (i=0; i<*argc+1; i++)
        argv1[i] = (*argv)[i] == NULL ? NULL : strdup ((*argv)[i]);

    //  Move the environment so setproctitle can use the space at
    //  the top of memory.

    // count number of environment variables
    n_vars = 0;
    while ((*envp)[n_vars] != NULL) n_vars++;

    // allocate space for new envp[] array
    environ = (char **) malloc ( sizeof (char *) * (n_vars+1) );

    // copy variables to new place
    for (i = 0; (*envp)[i] != NULL; i++)
        environ[i] = strdup ((*envp)[i]);
    environ[i] = NULL;

    // save start of argv area
    argv_start = (*argv)[0];

    // compute length of argv area
    if (n_vars > 0)
        argv_length = (*envp)[n_vars-1] + strlen ((*envp)[n_vars-1]) - argv_start - 1;
    else
        argv_length = (*argv)[*argc-1] + strlen ((*argv)[*argc-1]) - argv_start - 1;

    // kill pointers to the rest of argv array
    argv[1] = NULL;

    *argv = argv1;
}
