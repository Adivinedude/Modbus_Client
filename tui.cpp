/********************************* tui.c ************************************/
/*this file has been modified to fit my needs, 
 * setup pointer called
 * loop pointer called inside idle();
 * 'textual user interface'
 *
 * Author : P.J. Kunst <kunst@prl.philips.nl>
 * Date   : 1993-02-25
 */

#include <ctype.h>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tui.h"

WINDOW* wtitl, * wmain, * wbody, * wstat; /* title, menu, body, status win*/
int nexty, nextx;
int key = ERR, ch = ERR;
bool quit = FALSE;
bool incurses = FALSE;
time_t current_time;

int (*_keyLoop)(int) = 0;// called inside getkey()
void (*_loop) (void) = 0; // called inside idle()

void NotYetImplemented(const uint16_t value) /* display friendly message */
{
    char buf[] = "Not Yet Implemented";
    errormsg(buf);
}
void DoNothing(uint16_t value) {}

#if defined(unix) && !defined(DJGPP)
#include <unistd.h>
#endif

#define th 1     /* title window height */
#define mh 1     /* main menu height */
#define sh 2     /* status window height */
#define bh (LINES - th - mh - sh)   /* body window height */
#define bw COLS  /* body window width */


/******************************* STATIC ************************************/

#ifndef PDCURSES
static char wordchar(void)
{
    return 0x17;    /* ^W */
}
#endif


static char *padstr(const std::string& s, unsigned int length)
{
    static char buf[MAXSTRLEN];
    char fmt[10];

    snprintf(fmt, sizeof(fmt), s.size() > length ? "%%.%ds" : "%%-%ds", length);
    snprintf(buf, sizeof(buf), fmt, s.c_str());

    return buf;
}
static char *prepad(char *s, int length)
{
    int i;
    char *p = s;

    if (length > 0)
    {
        memmove((void *)(s + length), (const void *)s, strlen(s) + 1);

        for (i = 0; i < length; i++)
            *p++ = ' ';
    }

    return s;
}
static void rmline(WINDOW *win, int nr)   /* keeps box lines intact */
{
    mvwaddstr(win, nr, 1, padstr(" ", bw - 2));
    wrefresh(win);
}
static void initcolor(void)
{
#ifdef A_COLOR
    if (has_colors())
        start_color();

    /* foreground, background */

    init_pair(TITLECOLOR        & ~A_ATTR, COLOR_BLACK, COLOR_CYAN);
    init_pair(MAINMENUCOLOR     & ~A_ATTR, COLOR_WHITE, COLOR_CYAN);
    init_pair(MAINMENUREVCOLOR  & ~A_ATTR, COLOR_WHITE, COLOR_BLACK);
    init_pair(SUBMENUCOLOR      & ~A_ATTR, COLOR_WHITE, COLOR_CYAN);
    init_pair(SUBMENUREVCOLOR   & ~A_ATTR, COLOR_WHITE, COLOR_BLACK);
    init_pair(BODYCOLOR         & ~A_ATTR, COLOR_WHITE, COLOR_BLUE);
    init_pair(STATUSCOLOR       & ~A_ATTR, COLOR_WHITE, COLOR_CYAN);
    init_pair(INPUTBOXCOLOR     & ~A_ATTR, COLOR_BLACK, COLOR_CYAN);
    init_pair(EDITBOXCOLOR      & ~A_ATTR, COLOR_WHITE, COLOR_BLACK);
#endif
}
static void setcolor(WINDOW *win, chtype color)
{
    chtype attr = color & A_ATTR;  /* extract Bold, Reverse, Blink bits */

#ifdef A_COLOR
    attr &= ~A_REVERSE;  /* ignore reverse, use colors instead! */
    wattrset(win, COLOR_PAIR(color & A_CHARTEXT) | attr);
#else
    attr &= ~A_BOLD;     /* ignore bold, gives messy display on HP-UX */
    wattrset(win, attr);
#endif
}
void colorbox(WINDOW *win, chtype color, int hasbox)
{
    int maxy;
#ifndef PDCURSES
    int maxx;
#endif
    chtype attr = color & A_ATTR;  /* extract Bold, Reverse, Blink bits */

    setcolor(win, color);

#ifdef A_COLOR
    if (has_colors())
        wbkgd(win, COLOR_PAIR(color & A_CHARTEXT) | (attr & ~A_REVERSE));
    else
#endif
        wbkgd(win, attr);

    werase(win);

#ifdef PDCURSES
    maxy = getmaxy(win);
#else
    getmaxyx(win, maxy, maxx);
#endif
    if (hasbox && (maxy > 2))
        box(win, 0, 0);

    touchwin(win);
    wrefresh(win);
}
void idle(void)
{
    char buf[MAXSTRLEN];
    struct tm *tp;

    if (_loop) { _loop(); }

    if (time (&current_time) == -1)
        return;  /* time not available */

    tp = localtime(&current_time);
    sprintf(buf, " %.4d-%.2d-%.2d  %.2d:%.2d:%.2d",
            tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday,
            tp->tm_hour, tp->tm_min, tp->tm_sec);

    mvwaddstr(wtitl, 0, bw - strlen(buf) - 2, buf);
    wrefresh(wtitl);
}

/// <summary>
/// Store the size of the menu needed in *lines and *columns
/// </summary>
/// <param name="mp">Menu object to size</param>
/// <param name="lines">place to store the line count</param>
/// <param name="columns">place to store the column count</param>
void menudim(menu *mp, int *lines, int *columns)
{
    int n, l, mmax = 0;
    if(!mp) 
        return;
    for (n = 0; mp->func; n++, mp++)
        if ((l = mp->name.size()) > mmax) mmax = l;

    *lines = n;
    *columns = mmax + 2;
}

static void setmenupos(int y, int x)
{
    nexty = y;
    nextx = x;
}
static void getmenupos(int *y, int *x)
{
    *y = nexty;
    *x = nextx;
}
static int hotkey(const std::string &str)
{
    int c0 = str.size()?str.at(0):0;    /* if no upper case found, return first char */
    const char* s = str.c_str();
    for (; *s; s++)
        if (isupper((unsigned char)*s))
            break;
    return *s ? *s : c0;
}
void repaintmenu(menu_state *ms)
{
    if (!ms->pWindow || !ms->pMenu)
        return;
    int i;
    menu *p = ms->pMenu;

    for (i = 0; p->func; i++, p++) {
        mvwaddstr(ms->pWindow, i + 1, 2, p->name.c_str());
    }

    touchwin(ms->pWindow);
    wrefresh(ms->pWindow);
}
static void repaintmainmenu(int width, menu_state *ms)
{
    int i;
    menu *p = ms->pMenu;

    for (i = 0; p->func; i++, p++)
        mvwaddstr(ms->pWindow, 0, i * width, prepad(padstr(p->name, width - 1), 1));

    touchwin(ms->pWindow);
    wrefresh(ms->pWindow);
}
static void mainhelp(void)
{
    fprintf(stderr, "mainhelp");
#ifdef ALT_X
    statusmsg("Use arrow keys and Enter to select (Alt-X to quit)");
#else
    statusmsg("Use arrow keys and Enter to select");
#endif
}

void MenuStateErrorCheck(menu_state* ms) {
    if (!ms)
        throw std::exception("Invalid 'ms' pointer");
    if( !ms->pMenu )
        throw std::exception("Invalid 'ms->pMenu' pointer");
    if( !ms->pWindow )
        throw std::exception("Invalid 'ms->pWindow' pointer");
    int col;
    menudim(ms->pMenu, &ms->nitems, &col );
    if( ms->cur < 0 ){
        ms->cur = 0;
    }else if( ms->cur >= ms->nitems ){
        ms->cur = ms->nitems - 1;
    }
}
/// <summary>
/// Process Main menu drawing, key press, and selection
/// </summary>
/// <param name="mp">Menu declaration</param>
void mainmenu( menu_state* ms)
{
    int barlen, c, cur0, old = -1;
    MenuStateErrorCheck(ms);
    quit = false;
    menudim(ms->pMenu, &ms->nitems, &barlen);
    repaintmainmenu(barlen, ms);

    while (!quit)
    {
        if (ms->cur != old)
        {
            if (old != -1)
            {
                mvwaddstr(wmain, 0, old * barlen,
                          prepad(padstr(ms->pMenu[old].name, barlen - 1), 1));
                statusmsg(ms->pMenu[ms->cur].desc);
            }
            //else
            //    mainhelp();
            setcolor(wmain, MAINMENUREVCOLOR);
            mvwaddstr(wmain, 0, ms->cur * barlen,
                      prepad(padstr(ms->pMenu[ms->cur].name, barlen - 1), 1));
            setcolor(wmain, MAINMENUCOLOR);
            old = ms->cur;
            wrefresh(wmain);
        }

        switch (c = (key != ERR ? key : waitforkey(wmain)))
        {
        case KEY_DOWN:
            quit = 1; //exit menu
            //restore non selected menu colors
            mvwaddstr(wmain, 0, old * barlen,
                prepad(padstr(ms->pMenu[old].name, barlen - 1), 1));
            wrefresh(wmain);
            break; 

        case '\n':              /* menu item selected */
            touchwin(wbody);
            wrefresh(wbody);
            rmerror();
            setmenupos(th + mh, ms->cur * barlen);
            curs_set(1);
            if(ms->pMenu[ms->cur].func )
                (ms->pMenu[ms->cur].func)(ms->pMenu[ms->cur].value);   /* perform function */
            MenuStateErrorCheck(ms);
            curs_set(0);
            touchwin(wbody);
            wrefresh(wbody);
            switch (key)
            {
            case KEY_LEFT:
                ms->cur = (ms->cur + ms->nitems - 1) % ms->nitems;
                key = '\n';
                break;

            case KEY_RIGHT:
                ms->cur = (ms->cur + 1) % ms->nitems;
                key = '\n';
                break;

            default:
                key = ERR;
            }

            repaintmainmenu(barlen, ms);
            old = -1;
            break;

        case KEY_LEFT:
            ms->cur = (ms->cur + ms->nitems - 1) % ms->nitems;
            key = ERR;
            break;

        case KEY_RIGHT:
            ms->cur = (ms->cur + 1) % ms->nitems;
            key = ERR;
            break;

        case KEY_ESC:
            mainhelp();
            break;

        default:
            //detect hot key press
            key = ERR;
            if(!ms->hotkey)
                break;
            cur0 = ms->cur; //store current positon
            do
            {
                ms->cur = (ms->cur + 1) % ms->nitems;   //iterate through items

            } while ((ms->cur != cur0) && (hotkey(ms->pMenu[ms->cur].name) != toupper(c))); //check for end condition or hotkey

            if (hotkey(ms->pMenu[ms->cur].name) == toupper(c)) //if hotkey was found
                key = '\n';
        }

    }

    rmerror();
    touchwin(wbody);
    wrefresh(wbody);
}
void cleanup(void)   /* cleanup curses settings */
{
    if (incurses)
    {
        delwin(wtitl);
        delwin(wmain);
        delwin(wbody);
        delwin(wstat);
        curs_set(1);
        endwin();
        incurses = FALSE;
    }
}
/******************************* EXTERNAL **********************************/
void clsbody(void)
{
    werase(wbody);
    wmove(wbody, 0, 0);
}
int bodylen(void)
{
#ifdef PDCURSES
    return getmaxy(wbody);
#else
    int maxy, maxx;

    getmaxyx(wbody, maxy, maxx);
    return maxy;
#endif
}
/// <summary>
/// Get main body pointer
/// </summary>
/// <param name=""></param>
/// <returns>pointer to main body window</returns>
WINDOW* bodywin(void)
{
    return wbody;
}
void rmerror(void)
{
    rmline(wstat, 0);
}
void rmstatus(void)
{
    rmline(wstat, 1);
}
void titlemsg(const std::string &msg)
{
    mvwaddstr(wtitl, 0, 2, padstr(msg, bw - 3));
    wrefresh(wtitl);
}
void bodymsg(const std::string &msg)
{
    waddstr(wbody, msg.c_str());
    wrefresh(wbody);
}
void errormsg(const std::string &msg)
{
    if (!msg.size())
        return;
    beep();
    mvwaddstr(wstat, 0, 2, padstr(msg, bw - 3));
    wrefresh(wstat);
}
void statusmsg(const std::string &msg)
{
    if (!msg.size())
        return;
    mvwaddstr(wstat, 1, 2, padstr(msg, bw - 3));
    wrefresh(wstat);
}
bool keypressed(WINDOW* win)
{
    ch = wgetch(win);

    return ch != ERR;
}

int getkey(void)
{
    int c = ch;
    //all keyboard input pipes through here
    if (_keyLoop) { c = _keyLoop(c); }
    ch = ERR;
#ifdef ALT_X
    if (c == ALT_X) {  /* PC only ! */
        DoExit(0);
    }
#endif
    return c;
}

int waitforkey(WINDOW* win)
{
    do {
        idle();
    } while (!keypressed(win));
    return getkey();
}

void DoExit(const uint16_t value)   /* terminate program */
{
    quit = TRUE;
}


void domenu(menu_state *ms)
{
    int y, x;
    bool stop = FALSE;
    curs_set(0);
    getmenupos(&y, &x); // get menu position
    exSetupMenu(ms, y, x);
    exMenu(ms, y, x);   // run menu at this position
    exDone(ms);             // clean up overwriten text
    exClean(ms);            // free up resources
    touchwin(wbody);
    wrefresh(wbody);
    
}
WINDOW* exSetupMenu(menu_state* ms, int y, int x) {
    int barlen, mheight, mw;
    WINDOW* rt;
    menudim(ms->pMenu, &ms->nitems, &barlen);   //get dimensions of the menu
    mheight = ms->nitems + 2;                   //adjust menu height
    mw = barlen + 2;                            //and width
    rt = newwin(mheight, mw, y, x);             //make new window
    keypad(rt, TRUE);
    ms->pWindow = rt;
    return rt;
}
void exMenu(menu_state* ms, int y, int x) {
    int barlen, mheight, mw, old = -1, cur0;
    bool stop = FALSE;
    MenuStateErrorCheck(ms);
    colorbox(ms->pWindow, SUBMENUCOLOR, 1);     // set color and outline
    repaintmenu(ms);                            // paint new window
    menudim(ms->pMenu, &ms->nitems, &barlen);   // get dimensions of the menu
    key = ERR;
    if (!ms->nitems)
        return;
    mheight = ms->nitems + 2;           //adjust menu height
    mw = barlen + 2;                //and width

    while ((ms->pMenu[ms->cur].value == MENU_DISABLE)) {   // advance selection to the first enabled menu item.
        ms->cur++;
        if (ms->cur >= ms->nitems) {
            ms->cur--;
            //key = KEY_ESC;
            //return; //all menu items are disabled, exit subroutine
            break;
        }
    }
    quit = false;
    while (!stop && !quit)
    {
        cur0 = ms->cur; //store current selection
        if (ms->cur != old) //has the selection changed
        {
            if (old != -1) {    //if NOT the (first iteration or returning from OnClick event)
                //restore the previous selections text and color
                mvwaddstr(ms->pWindow, old + 1, 1,
                    prepad(padstr(ms->pMenu[old].name, barlen - 1), 1));
            }
            //call OnSelect callback
            if (ms->pMenu[ms->cur].isSelected)
                ms->pMenu[ms->cur].isSelected(ms->pMenu[ms->cur].value);
            //change color of new selection.
            setcolor(ms->pWindow, SUBMENUREVCOLOR);
            mvwaddstr(ms->pWindow, ms->cur + 1, 1,
                prepad(padstr(ms->pMenu[ms->cur].name, barlen - 1), 1));

            setcolor(ms->pWindow, SUBMENUCOLOR);
            statusmsg(ms->pMenu[ms->cur].desc); //display menu item description

            old = ms->cur;
            wrefresh(ms->pWindow);
        }

        //process user input
        switch (key = ((key != ERR) ? key : waitforkey(ms->pWindow)))
        {
        /* menu item selected, ENTER key pressed*/
        case '\n':                      
            setmenupos(y + 1, x + 1);
            rmerror();                  //clear error message if any

            key = ERR;
            curs_set(1);
            //edit to include passing an optional value to the function.
            if (ms->pMenu[ms->cur].func) {
                (ms->pMenu[ms->cur].func)(ms->pMenu[ms->cur].value);   /* perform function */
                MenuStateErrorCheck(ms);
                //stop = 1;
            }
            curs_set(0);
            touchwin(bodywin());
            wrefresh(bodywin());
            repaintmenu(ms);
            old = -1;
            break;

        case KEY_UP:
            key = ERR;  //Clear key press
            //scroll up the list until a non-disabled item has been found.
            //if the end of the list has been reached, restore the origional selection.
            do {
                ms->cur--;
            } while ( (ms->cur >= 0 ) && (ms->pMenu[ms->cur].value == MENU_DISABLE));
            //end of list has been reached.
            if (ms->cur < 0) {       //if we are at the end of the list
                stop = true;         //exit the menu
                key = KEY_ESC;
                //restore the previous selections text and color
                setcolor(ms->pWindow, SUBMENUCOLOR);
                mvwaddstr(ms->pWindow, cur0 + 1, 1,
                    prepad(padstr(ms->pMenu[cur0].name, barlen - 1), 1));
                //touchwin(wmenu);
                wrefresh(ms->pWindow);
            }
            break;

        case KEY_DOWN:
             key = ERR; //clear key press
            //scroll down the list until a non-disabled item has been found.
            //if the end of the list has been reached, restore the origional selection.
            do {
                ms->cur++;
            } while ( (ms->cur < ms->nitems) && (ms->pMenu[ms->cur].value == MENU_DISABLE) );
            //end of list has been reached.
            if (ms->cur >= ms->nitems)      //if we are at the end of the list
                ms->cur = cur0;//restore previous selection
            break;

        case KEY_LEFT:
        case KEY_RIGHT:    
            stop = true;
            break;

        case KEY_ESC:

            stop = TRUE;
            key = KEY_ESC;
            break;

        default:
            //check for hotkey presses
            key = ERR;
            if (!ms->hotkey)
                break;
            cur0 = ms->cur;
            do
            {
                ms->cur = (ms->cur + 1) % ms->nitems;

            } while ((ms->cur != cur0) &&
                (hotkey(ms->pMenu[ms->cur].name) != toupper((int)key)));

            key = (hotkey(ms->pMenu[ms->cur].name) == toupper((int)key)) ? '\n' : ERR;
        }

    }
}

void exDone(menu_state* ms) {
    rmerror();
    touchwin(ms->pWindow);
    wrefresh(ms->pWindow);
}

void exClean(menu_state* ms) {
    delwin(ms->pWindow);
    ms->pWindow = 0;
}

void setupmenu(menu_state* ms, const std::string &title) {
    quit = FALSE;
    initscr();
    incurses = TRUE;
    initcolor();

    wtitl = subwin(stdscr, th, bw, 0, 0);
    wmain = subwin(stdscr, mh, bw, th, 0);
    wbody = subwin(stdscr, bh, bw, th + mh, 0);

    wstat = subwin(stdscr, sh, bw, th + mh + bh, 0);
    ms->pWindow = wmain;

    colorbox(wtitl, TITLECOLOR, 0);
    colorbox(wmain, MAINMENUCOLOR, 0);
    colorbox(wbody, BODYCOLOR, 0);
    colorbox(wstat, STATUSCOLOR, 0);

    if (title.size())
        titlemsg(title);

    cbreak();              /* direct input (no newline required)... */
    noecho();              /* ... without echoing */
    curs_set(0);           /* hide cursor (if possible) */
    nodelay(wbody, TRUE);  /* don't wait for input... */
    halfdelay(10);         /* ...well, no more than a second, anyway */
    keypad(wmain, TRUE);   /* enable cursor keys */
    keypad(wbody, TRUE);   
    scrollok(wbody, TRUE); /* enable scrolling in main window */

    leaveok(stdscr, TRUE);
    leaveok(wtitl, TRUE);
    leaveok(wmain, TRUE);
    leaveok(wstat, TRUE);
}
void startmenu(menu_state*mp, const std::string &title)
{
    setupmenu(mp, title);
    mainmenu(mp);
    cleanup();
}

static void repainteditbox(WINDOW *win, int x, const char *buf)
{
#ifndef PDCURSES
    int maxy;
#endif
    int maxx;

#ifdef PDCURSES
    maxx = getmaxx(win);
#else
    getmaxyx(win, maxy, maxx);
#endif
    werase(win);
    mvwprintw(win, 0, 0, "%s", padstr(buf, maxx));
    wmove(win, 0, x);
    wrefresh(win);
}

int weditstr(WINDOW *win, char *buf, int field)
{
    char org[MAXSTRLEN], *tp, *bp = buf;
    bool defdisp = TRUE, stop = FALSE, insert = FALSE;
    int cury, curx, begy, begx;
    chtype oldattr;
    WINDOW *wedit;
    int c = 0;

    if ((field >= MAXSTRLEN) || (buf == NULL) ||
        ((int)strlen(buf) > field - 1))
        return ERR;

    strcpy(org, buf);   /* save original */

    wrefresh(win);
    getyx(win, cury, curx);
    getbegyx(win, begy, begx);

    wedit = subwin(win, 1, field, begy + cury, begx + curx);
    if (!wedit) {
        int new_field = getmaxx(win);
        wedit = subwin(win, 1, new_field, begy + cury, begx + curx);
        if (!wedit) {
            errormsg("Failed to open subwindow");
            return ERR;
        }
    }
    oldattr = getattrs(wedit);
    colorbox(wedit, EDITBOXCOLOR, 0);

    keypad(wedit, TRUE);
    curs_set(1);

    while (!stop)
    {
        repainteditbox(wedit, bp - buf, buf);

        switch (c = waitforkey(wedit))
        {

        case KEY_ESC:
            strcpy(buf, org);   /* restore original */
            stop = TRUE;
            break;

        case '\n':
        case KEY_UP:
        case KEY_DOWN:
            stop = TRUE;
            break;

        case KEY_LEFT:
            if (bp > buf)
                bp--;
            break;

        case KEY_RIGHT:
            defdisp = FALSE;
            if (bp - buf < (int)strlen(buf))
                bp++;
            break;

        case '\t':            /* TAB -- because insert
                                  is broken on HPUX */
        case KEY_IC:          /* enter insert mode */
        case KEY_EIC:         /* exit insert mode */
            defdisp = FALSE;
            insert = !insert;

            curs_set(insert ? 2 : 1);
            break;

        default:
            /* check KEY_BACKSPACE manually, because erasechar() can only
             * return char, and KEY_BACKSPACE is an int, so on systems
             * that truly return KEY_BACKSPACE from wgetch(), we'll never
             * catch it with erasechar() */
            if (c == erasechar() || c == KEY_BACKSPACE)
            {
                if (bp > buf)
                {
                    memmove((void *)(bp - 1), (const void *)bp, strlen(bp) + 1);
                    bp--;
                }
            }
            else if (c == killchar())   /* ^U */
            {
                bp = buf;
                *bp = '\0';
            }
            else if (c == wordchar())   /* ^W */
            {
                tp = bp;

                while ((bp > buf) && (*(bp - 1) == ' '))
                    bp--;
                while ((bp > buf) && (*(bp - 1) != ' '))
                    bp--;

                memmove((void *)bp, (const void *)tp, strlen(tp) + 1);
            }
            //else if (isprint(c))
            //fix assert error
            else if ( c > -1 && c < 255 )
            {
                if (defdisp)
                {
                    bp = buf;
                    *bp = '\0';
                    defdisp = FALSE;
                }

                if (insert)
                {
                    if ((int)strlen(buf) < field - 1)
                    {
                        memmove((void *)(bp + 1), (const void *)bp,
                                strlen(bp) + 1);

                        *bp++ = c;
                    }
                }
                else if (bp - buf < field - 1)
                {
                    /* append new string terminator */

                    if (!*bp)
                        bp[1] = '\0';

                    *bp++ = c;
                }
            }
        }
    }

    curs_set(0);

    wattrset(wedit, oldattr);
    repainteditbox(wedit, bp - buf, buf);
    delwin(wedit);

    return c;
}


WINDOW *winputbox(WINDOW *win, int nlines, int ncols)
{
    WINDOW *winp;
    int cury, curx, begy, begx;

    getyx(win, cury, curx);
    getbegyx(win, begy, begx);

    winp = newwin(nlines, ncols, begy + cury, begx + curx);
    colorbox(winp, INPUTBOXCOLOR, 1);

    return winp;
}

int getstrings(const char *desc[], char *buf[], int field)
{
    WINDOW *winput;
    int oldy, oldx, maxy, maxx, nlines, ncols, i, n, l, mmax = 0;
    int c = 0;
    bool stop = FALSE;

    //find the longest description
    for (n = 0; desc[n]; n++) {
        l = strlen(desc[n]);
        if (l > mmax)
            mmax = l;
    }
    
    nlines = n + 2;             //rows needed for display
    ncols = mmax + field + 4;   //columns needed for display.
    getyx(wbody, oldy, oldx);   //store old curcer position
    getmaxyx(wbody, maxy, maxx);//find maxium curcer position

    //Draw a box centered of body window, big enough to hold the descritpions and fields
    winput = mvwinputbox(wbody, (maxy - nlines) / 2, (maxx - ncols) / 2,
        nlines, ncols);

    //make a line break for spacers
    char* line_string = (char*)malloc(mmax + 1);
    memset(line_string, '-', mmax);
    line_string[mmax] = '\0';

    //List each description within the window
    for (i = 0; i < n; i++){
        size_t length_of_desc;
        const char* p = desc[i]; //set pointer to desc[]

        //if we are given an empty char[], print a spacer
        if ( (length_of_desc = strlen(p)) == 0) {
            p = line_string;     
            length_of_desc = mmax;
        }
        //right align the text and print to window
        size_t offset = mmax - length_of_desc;
        mvwprintw(winput, i + 1, offset + 2, "%s", p);

        //print user input
        mvwprintw(winput, i + 1, mmax + 3, "%s", buf[i]);
    }
    free(line_string);
    wrefresh(winput);

    i = 0;

    //handle key presses until exit condition is met
    while (!stop)
    {
        //pass field editing to handler. Unprocessed keys are returned to us for handling.
        switch (c = mvweditstr(winput, i+1, mmax+3, buf[i], field))
        {
        case KEY_ESC:
            stop = TRUE;
            break;

        case KEY_UP:
            do {
                i = (i + n - 1) % n;
            } while (!strlen(desc[i]));
            break;

        case '\n':
        case '\t':
        case KEY_DOWN:
            do {
                if (++i == n) {
                    stop = TRUE;    /* all passed? */
                    break;
                }
            } while (!strlen(desc[i]));
        }
    }

    delwin(winput);
    touchwin(wbody);
    wmove(wbody, oldy, oldx);
    wrefresh(wbody);

    return c;
}

void TextInputBox_Fields(   const char** fieldname, 
                            size_t fieldsize, 
                            bool(*ProcessData)(char** fieldbuf, size_t fieldsize), 
                            const char** defaultField) {
    size_t num_of_fields = 0;
    //count how many input fields there are.
    for (; fieldname[num_of_fields] != 0; num_of_fields++) {}

    //associate memory for the array of user input buffers
    char** fieldbuf = new char* [num_of_fields];
    //associate memory for the user input buffer
    for (size_t i = 0; i < num_of_fields; i++) {
        fieldbuf[i] = (char*)calloc(1, fieldsize + 1);
        //load default user input if available
        if (defaultField && defaultField[i]) {
            strcpy(fieldbuf[i], defaultField[i]);
        }
    }
    //collect user input
    bool tryagain;
    do {
        tryagain = false;
        if (getstrings(fieldname, fieldbuf, fieldsize) != KEY_ESC)
        {
            if (ProcessData) {
                tryagain = ProcessData(fieldbuf, num_of_fields);
            }
        }
    } while (tryagain);

    for (size_t i = 0; i < num_of_fields; i++)
        free(fieldbuf[i]);
    delete[] fieldbuf;
}
