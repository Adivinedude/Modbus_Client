/* This file has been modified to fit my needs. added scalls to function pointers during idle()
* added a lot of comments describing what functions do.
* 
* Original Credit
 * 'textual user interface'
 *
 * Author : P.J. Kunst <kunst@prl.philips.nl>
 * Date   : 1993-02-25
 */

#ifndef _TUI_H_
#define _TUI_H_

    #include <curses.h>
    #include <stdint.h>
    #include <string>
    #include <list>

    #ifdef A_COLOR
        #define A_ATTR  (A_ATTRIBUTES ^ A_COLOR)  /* A_BLINK, A_REVERSE, A_BOLD */
    #else
        #define A_ATTR  (A_ATTRIBUTES)            /* standard UNIX attributes */
    #endif

    #ifdef A_COLOR
        # define TITLECOLOR       1       /* color pair indices */
        # define MAINMENUCOLOR    (2 | A_BOLD)
        # define MAINMENUREVCOLOR (3 | A_BOLD | A_REVERSE)
        # define SUBMENUCOLOR     (4 | A_BOLD)
        # define SUBMENUREVCOLOR  (5 | A_BOLD | A_REVERSE)
        # define BODYCOLOR        6
        # define STATUSCOLOR      (7 | A_BOLD)
        # define INPUTBOXCOLOR    8
        # define EDITBOXCOLOR     9//(9 | A_BOLD | A_REVERSE)
    #else
        # define TITLECOLOR       0       /* color pair indices */
        # define MAINMENUCOLOR    (A_BOLD)
        # define MAINMENUREVCOLOR (A_BOLD | A_REVERSE)
        # define SUBMENUCOLOR     (A_BOLD)
        # define SUBMENUREVCOLOR  (A_BOLD | A_REVERSE)
        # define BODYCOLOR        0
        # define STATUSCOLOR      (A_BOLD)
        # define INPUTBOXCOLOR    0
        # define EDITBOXCOLOR     (A_BOLD | A_REVERSE)
    #endif

    #define MAXSTRLEN  256
    #define KEY_ESC    0x1b     /* Escape */

    typedef void (*FUNC)(const uint16_t);               //menu function pointer
    #define END_OF_MENU {0, (FUNC)0, std::string()}     //menu terminator
    #define MENU_DISABLE (uint16_t) 0xFFFF              //menu disabled flag. causes item to be non selectable

    extern void     NotYetImplemented(const uint16_t value);// prints message saying feature is not implemented
    extern void     DoNothing(uint16_t value);              // blank function pointer for menus
    extern int      (*_keyLoop)(int);                       // pre processing for each key event. 
    extern void     (*_loop) (void);                        // Loop function pointer for running external code in each idle() call
    extern time_t   current_time;

    class menu_text{
         public:
            menu_text(){};
            virtual             ~menu_text(){};
            virtual const char* GetValue()=0;
    };

    class simple_menu_text : public menu_text {
    private:
        std::string content;
        simple_menu_text();
    public:
        simple_menu_text(const char* s): content(s){};
        simple_menu_text(std::string s): content(s){};
         ~simple_menu_text(){};
        virtual const char* GetValue(){
            return content.c_str(); 
        };
    };

    typedef struct menu
    {
        menu_text*      pName;      /* item test                        */
        FUNC            func;       /* (pointer to) OnClick function    */
        std::string     desc;       /* function description             */
        uint16_t        value;      /* options value                    */
        FUNC            isSelected; /* (pointer to) OnSelect function   */
        menu() :pName(0), func(0), desc(std::string()), value(0), isSelected(0) {};
        //3 arguments
        menu(const char* n, FUNC f, const char* d) :pName(new simple_menu_text(n)), func(f), desc(std::string(d)), value(0), isSelected(0) {};
        menu(menu_text*  n, FUNC f, const char* d) :pName(n), func(f), desc(d), value(0), isSelected(0) {};
        menu(menu_text*  n, FUNC f, std::string d) :pName(n), func(f), desc(d), value(0), isSelected(0) {};
        //4 arguments
        menu(const char* n, FUNC f, const char* d, uint16_t v) :pName(new simple_menu_text(n)), func(f), desc(std::string(d)), value(v), isSelected(0) {};
        menu(menu_text*  n, FUNC f, const char* d, uint16_t v) :pName(n), func(f), desc(d), value(v), isSelected(0) {};
        menu(menu_text*  n, FUNC f, std::string d, uint16_t v) :pName(n), func(f), desc(d), value(v), isSelected(0) {};
        //5 arguments
        menu(const char* n, FUNC f, const char* d, uint16_t v, FUNC i) :pName(new simple_menu_text(n)), func(f), desc(d), value(v), isSelected(i) {};
        menu(menu_text*  n, FUNC f, const char* d, uint16_t v, FUNC i) :pName(n), func(f), desc(d), value(v), isSelected(i) {};
        menu(menu_text*  n, FUNC f, std::string d, uint16_t v, FUNC i) :pName(n), func(f), desc(d), value(v), isSelected(i) {};

        ~menu(){
            if(pName)
                delete pName; 
        };
        // move constructor 
        menu(menu&& m) noexcept
        {
            menu::menu();
            *this = std::move(m);
        }
        // move assignment operator
        menu& operator=(menu&& m) noexcept 
        {
            if (this != &m) {
                pName = m.pName;
                m.pName = 0;
                func    = m.func;
                desc    = std::move(m.desc);
                value   = m.value;
                isSelected = m.isSelected;
            }
            return *this;
        }

    } menu;

    typedef struct menu_state
    {
        menu    *pMenu;
        int     nitems;
        int     cur;
        int     hotkey;
        WINDOW* pWindow;
        bool    isSelected;
        bool    alwaysSelected;
        bool    alwaysDrawBox;
        menu_state()                            :pMenu(0), nitems(0), cur(0), hotkey(0), pWindow(0), isSelected(0), alwaysSelected(0), alwaysDrawBox(0) {};
        menu_state(WINDOW* w)                   :pMenu(0), nitems(0), cur(0), hotkey(0), pWindow(w), isSelected(0), alwaysSelected(0), alwaysDrawBox(0) {};
        menu_state(menu* a, int b, int c, int d):pMenu(a), nitems(b), cur(c), hotkey(d), pWindow(0), isSelected(0), alwaysSelected(0), alwaysDrawBox(0) {};
    } menu_state;

    /* ANSI C function prototypes: */
    void colorbox(WINDOW* win, chtype color, int hasbox);
    /// <summary>
    /// Idle time function, addition processing performed here
    /// </summary>
    /// <param name=""></param>
    void idle(void);
    /// <summary>
    /// Returns the dimensions of the menu
    /// </summary>
    /// <param name="mp">Menu object to size</param>
    /// <param name="lines">(pointer to) where to store the lines</param>
    /// <param name="columns">(pointer to) where to store the columns</param>
    void menudim(menu* mp, int* lines, int* columns);
    /// <summary>
    /// Draw a submenu
    /// </summary>
    /// <param name="wmenu">pointer to parent window</param>
    /// <param name="mp">menu object to draw</param>
    void repaintmenu(menu_state *ms);

    void    clsbody(void);
    int     bodylen(void);
    WINDOW *bodywin(void);
    extern  std::list<menu_state*> BodyRedrawStack;
    void    repaintBody(void);

    void    rmerror(void);
    void    rmstatus(void);

    void    titlemsg(const std::string &msg);
    /// <summary>
    /// Prints text to main body
    /// </summary>
    /// <param name="msg">Text to print</param>
    void    bodymsg(const std::string &msg);

    /// <summary>
    /// Prints text to error bar
    /// </summary>
    /// <param name="msg">Error Message</param>
    void    errormsg(const std::string &msg);

    /// <summary>
    /// Prints text to status bar
    /// </summary>
    /// <param name="msg">Status Message</param>
    void    statusmsg(const std::string &msg);

    //bool    keypressed(void);
    int     getkey(void);
    int     waitforkey(WINDOW* win);

    /// <summary>
    /// Exits the Menu Subroutine
    /// </summary>
    /// <param name=""></param>
    void DoExit(const uint16_t value);
    void MenuStateErrorCheck(menu_state* ms);
    void mainmenu(menu_state* ms);

    void setupmenu(menu_state* mp, const std::string &title);
    /// <summary>
    /// Setup main menu
    /// </summary>
    /// <param name="mp">Main Menu</param>
    /// <param name="mtitle">Application Title</param>
    void startmenu(menu_state *mp, const std::string &title);

    /// <summary>
    /// Draws and key processing for drop down menus
    /// </summary>
    /// <param name="mp">Menu definition</param>
    void domenu(menu_state *ms);

    /// <summary>
    /// Same as domenu except location of menu can be set
    /// </summary>
    /// <param name="mp">Menu definition</param>
    /// <param name="y">position</param>
    /// <param name="x">position</param>
    WINDOW* exSetupMenu(menu_state* ms, int y, int x);
    void    exMenu(menu_state* ms, int y, int x);
    void    exDone(menu_state* ms);
    void    exClean(menu_state* ms);
    /// <summary>
    /// The initial value of 'str' with a maximum length of 'field' - 1,
    /// which is supplied by the calling routine, is editted.The user's
    /// erase(^ H), kill(^ U) and delete word(^ W) chars are interpreted.
    /// The PC insert or Tab keys toggle between insert and edit mode.
    /// Escape aborts the edit session, leaving 'str' unchanged.
    /// Enter, Up or Down Arrow are used to accept the changes to 'str'.
    /// NOTE: editstr(), mveditstr(), and mvweditstr() are macros.
    /// Errors:
    ///  It is an error to call this function with a NULL window pointer.
    ///  The length of the initial 'str' must not exceed 'field' - 1.
    /// </summary>
    /// <param name="win">Parent Window</param>
    /// <param name="buf">Buffer</param>
    /// <param name="field">Buffer size</param>
    /// <returns>Returns the input terminating character on success (Escape,
    /// Enter, Up or Down Arrow) and ERR on error.</returns>
    int weditstr(WINDOW* win, char* buf, int field);

    /// <summary>
    /// Creates a new input box
    /// </summary>
    /// <param name="win">Parent Window</param>
    /// <param name="nlines">height</param>
    /// <param name="ncols">width</param>
    /// <returns>Pointer to new window</returns>
    WINDOW *winputbox(WINDOW *win, int nlines, int ncols);

    /// <summary>
    /// Get many Strings from the user.
    /// </summary>
    /// <param name="desc">Description of each field, null terminated array</param>
    /// <param name="buf">Buffers to store each input</param>
    /// <param name="field">Size of buffers</param>
    /// <returns>Last key entered</returns>
    int getstrings(const char *desc[], char *buf[], int field);

    void cleanup(void);

    #define editstr(s,f)           (weditstr(stdscr,s,f))
    #define mveditstr(y,x,s,f)     (move(y,x)==ERR?ERR:editstr(s,f))
    #define mvweditstr(w,y,x,s,f)  (wmove(w,y,x)==ERR?ERR:weditstr(w,s,f))

    #define inputbox(l,c)          (winputbox(stdscr,l,c))
    #define mvinputbox(y,x,l,c)    (move(y,x)==ERR?w:inputbox(l,c))

    /// <summary>
    /// Creates a new window at position y,x
    /// </summary>
    /// <param name="win">Parent Window</param>
    /// <param name="nlines">height</param>
    /// <param name="ncols">width</param>
    /// <returns>Pointer to new window</returns>
    #define mvwinputbox(w,y,x,l,c) (wmove(w,y,x)==ERR?w:winputbox(w,l,c))

    extern WINDOW* wtitl, * wmain, * wbody, * wstat; /* title, menu, body, status win*/
    extern int nexty, nextx, key, ch;

    /// <summary>
    /// Displays a user input dialog
    /// </summary>
    /// <param name="fieldname">(pointer to) char*[] of item descriptions</param>
    /// <param name="fieldsize">size of buffers to store the user input in</param>
    /// <param name="ProcessData">callback function to process user input, Returns true on error, false on sucess</param>
    /// <param name="defaultfield">(pointer to) char*[] of default user input</param>
    void TextInputBox_Fields(   const char** fieldname, 
                                size_t fieldsize, 
                                bool(*ProcessData)(char** fieldbuf, size_t fieldsize), 
                                const char** defaultfield);
#endif
