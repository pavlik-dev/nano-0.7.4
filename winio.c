/**************************************************************************
 *   winio.c                                                              *
 *                                                                        *
 *   Copyright (C) 1999 Chris Allegretta                                  *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 1, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *   GNU General Public License for more details.                         *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                        *
 **************************************************************************/

#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "proto.h"
#include "nano.h"

/* Window I/O */

void do_first_line(void)
{
    current = fileage;
    placewewant = 0;
    current_x = 0;
    edit_update(current);
}

void do_last_line(void)
{
    current = filebot;
    placewewant = 0;
    current_x = 0;
    edit_update(current);
}


/* Return the actual place on the screen of current->data[current_x], which 
   should always be > current_x */
int xplustabs(void)
{
    int i, tabs = 0;

    if (current == NULL || current->data == NULL)
	return 0;

    for (i = 1; i <= current_x && current->data[i - 1] != 0; i++) {
	tabs++;

	if (current->data[i - 1] == NANO_CONTROL_I)
	    tabs += 8 - (tabs % 8);
    }

#ifdef DEBUG
    fprintf(stderr, "xplustabs for current_x=%d returned %d\n", current_x,
	    tabs);
#endif
    return tabs;
}

/* Like xplustabs, but for a specifc index of a speficific filestruct */
int xpt(filestruct * fileptr, int index)
{
    int i, tabs = 0;

    if (fileptr == NULL || fileptr->data == NULL)
	return 0;

    for (i = 1; i <= index && fileptr->data[i - 1] != 0; i++) {
	tabs++;

	if (fileptr->data[i - 1] == NANO_CONTROL_I)
	    tabs += 8 - (tabs % 8);
    }

    return tabs;
}

/* Return what current_x should be, given xplustabs() for the line.
   Opposite of xplustabs */
int actual_x(filestruct * fileptr, int xplus)
{
    int i, tot = 0;

    if (fileptr == NULL || fileptr->data == NULL)
	return 0;

    for (i = 1; i + tot <= xplus && fileptr->data[i - 1] != 0; i++)
	if (fileptr->data[i - 1] == NANO_CONTROL_I)
	    tot += 8 - ((i + tot) % 8);

    if (fileptr->data[i - 1] != 0)
	i--;			/* Im sure there's a good reason why this is needed for
				   it to work, I just cant figure out why =-) */

#ifdef DEBUG
    fprintf(stderr, "actual_x for xplus=%d returned %d\n", xplus, i);
#endif
    return i;
}

/* a strlen with tabs factored in, similar to xplustabs() */
int strlenpt(char *buf)
{
    int i, tabs = 0;

    if (buf == NULL)
	return 0;

    for (i = 1; buf[i - 1] != 0; i++) {
	tabs++;

	if (buf[i - 1] == NANO_CONTROL_I)
	    tabs += 8 - (tabs % 8);
    }

    return tabs;
}


/* resets current_y based on the position of current and puts the cursor at 
   (current_y, current_x) */
void reset_cursor(void)
{
    filestruct *ptr = edittop;

    current_y = 0;

    while (ptr != current && ptr != editbot && ptr->next != NULL) {
	ptr = ptr->next;
	current_y++;
    }

    if (xplustabs() <= COLS - 2)
	wmove(edit, current_y, xplustabs());
    else
	wmove(edit, current_y, xplustabs() % (COLS - 1) + 5);

    wrefresh(edit);
}

void blank_bottombars(void)
{
    int i, j;

    for (j = 1; j <= 2; j++)
	for (i = 0; i <= COLS - 1; i++)
	    mvwaddch(bottomwin, j, i, ' ');

    reset_cursor();
}

void blank_statusbar(void)
{
    int i;

    for (i = 0; i <= COLS - 1; i++)
	mvwaddch(bottomwin, 0, i, ' ');

    reset_cursor();
}

void blank_statusbar_refresh(void)
{
    blank_statusbar();
    wrefresh(bottomwin);
}

void check_statblank(void)
{

    if (statblank > 1)
	statblank--;
    else if (statblank == 1) {
	statblank--;
	blank_statusbar_refresh();
    }
}

/* Get the input from the kb, this should only be called from statusq */

int nanogetstr(char *buf, char *def, shortcut s[], int slen, int start_x)
{
    int kbinput = 0, j = 0, x = 0, xend;
    char inputstr[132], inputbuf[132] = "";

    blank_statusbar();
    mvwaddstr(bottomwin, 0, 0, buf);
    if (strlen(def) > 0)
	waddstr(bottomwin, def);
    wrefresh(bottomwin);

    x = strlen(def) + strlen(buf);

    /* Get the input! */
    if (strlen(def) > 0) {
	strcpy(answer, def);
	strcpy(inputbuf, def);
    }
    /* Go into raw mode so we can actually get ^C, for example */
    raw();

    while ((kbinput = wgetch(bottomwin)) != 13) {
	for (j = 0; j <= slen - 1; j++) {
	    if (kbinput == s[j].val) {
		noraw();
		cbreak();
		strcpy(answer, "");
		return s[j].val;
	    }
	}
	xend = strlen(buf) + strlen(inputbuf);

	if (kbinput >= 32)
	    switch (kbinput) {
	    case KEY_RIGHT:

		if (x < xend)
		    x++;
		wmove(bottomwin, 0, x);
		break;
	    case KEY_BACKSPACE:
	    case KEY_DC:
	    case 127:
		if (strlen(inputbuf) > 0)
		    inputbuf[strlen(inputbuf) - 1] = 0;
		blank_statusbar();
		mvwaddstr(bottomwin, 0, 0, buf);
		waddstr(bottomwin, inputbuf);
	    case KEY_LEFT:
		if (x > strlen(buf))
		    x--;
		wmove(bottomwin, 0, x);
		break;
	    case KEY_UP:
	    case KEY_DOWN:
		break;
	    default:
		strcpy(inputstr, inputbuf);
		inputstr[x - strlen(buf)] = kbinput;
		strcpy(&inputstr[x - strlen(buf) + 1],
		       &inputbuf[x - strlen(buf)]);
		strcpy(inputbuf, inputstr);
		x++;

		mvwaddstr(bottomwin, 0, 0, buf);
		waddstr(bottomwin, inputbuf);
		wmove(bottomwin, 0, x);

#ifdef DEBUG
		fprintf(stderr, "input \'%c\' (%d)\n", kbinput, kbinput);
#endif
	    }
	wrefresh(bottomwin);
    }

    strncpy(answer, inputbuf, 132);

    noraw();
    cbreak();
    if (!strcmp(answer, ""))
	return -2;
    else
	return 0;
}

void horizbar(WINDOW * win, int y)
{
    int i = 0;

    wattron(win, A_REVERSE);
    for (i = 0; i <= COLS - 1; i++)
	mvwaddch(win, y, i, ' ');
    wattroff(win, A_REVERSE);
}

void titlebar(void)
{
    horizbar(topwin, 0);
    wattron(topwin, A_REVERSE);
    mvwaddstr(topwin, 0, 3, VERMSG);
    if (!strcmp(filename, ""))
	mvwaddstr(topwin, 0, center_x - 6, "New Buffer");
    else {
	mvwaddstr(topwin, 0, center_x - 3, "File: ");
	waddstr(topwin, filename);
    }

    if (modified)
	mvwaddstr(topwin, 0, COLS - 10, "Modified");
    wattroff(topwin, A_REVERSE);
    wrefresh(topwin);
    reset_cursor();
}

void onekey(char *keystroke, char *desc)
{
    char description[80];

    snprintf(description, 12, " %-11s", desc);
    wattron(bottomwin, A_REVERSE);
    waddstr(bottomwin, keystroke);
    wattroff(bottomwin, A_REVERSE);
    waddstr(bottomwin, description);
}

void clear_bottomwin(void)
{
    int i;

    for (i = 0; i <= COLS - 1; i++) {
	mvwaddch(bottomwin, 1, i, ' ');
	mvwaddch(bottomwin, 2, i, ' ');
    }
    wrefresh(bottomwin);
}

void bottombars(shortcut s[], int slen)
{
    int i;
    char keystr[10];

    clear_bottomwin();
    wmove(bottomwin, 1, 0);
    for (i = 0; i <= slen - 1; i += 2) {
	sprintf(keystr, "^%c", s[i].val + 64);
	onekey(keystr, s[i].desc);
    }

    wmove(bottomwin, 2, 0);
    for (i = 1; i <= slen - 1; i += 2) {
	sprintf(keystr, "^%c", s[i].val + 64);
	onekey(keystr, s[i].desc);
    }
    wrefresh(bottomwin);

}

/* If modified is not already set, set it and update titlebar */
void set_modified(void)
{
    if (!modified) {
	modified = 1;
	titlebar();
	wrefresh(topwin);
    }
}

/* Just update one line in the edit buffer */
void update_line(filestruct * fileptr)
{
    filestruct *filetmp;
    int line = 0;

    for (filetmp = edittop; filetmp != fileptr && filetmp != editbot;
	 filetmp = filetmp->next)
	line++;

    mvwaddnstr(edit, line, 0, filetmp->data, COLS);
    reset_cursor();
    wrefresh(edit);

}

void center_cursor(void)
{
    current_y = editwinrows / 2;
    wmove(edit, current_y, current_x);
    wrefresh(edit);
}

/* Refresh the screen without changing the position of lines */
void edit_refresh(void)
{
    int lines = 0, i = 0, col = 0;
    filestruct *temp;

    if (current == NULL)
	return;

    temp = edittop;

    while (lines <= editwinrows - 1 && lines <= totlines &&
	   temp != NULL && temp != filebot) {

	mvwaddstr(edit, lines, 0, hblank);

	if (mark_isset) {

	    if ((temp->lineno > mark_beginbuf->lineno
		 && temp->lineno > current->lineno)
		|| (temp->lineno < mark_beginbuf->lineno
		    && temp->lineno < current->lineno)) {

		/* We're on a normal, unselected line */
		mvwaddnstr(edit, lines, 0, temp->data, COLS);

		if (strlenpt(temp->data) > COLS)
		    mvwaddch(edit, lines, COLS - 1, '$');
	    } else {

		/* We're on selected text */
		if (temp != mark_beginbuf && temp != current) {
		    wattron(edit, A_REVERSE);
		    mvwaddstr(edit, lines, 0, temp->data);
		    if (strlenpt(temp->data) > COLS)
			mvwaddch(edit, lines, COLS - 1, '$');

		    wattroff(edit, A_REVERSE);
		}
		/* Special case, we're still on the same line we started marking */
		else if (temp == mark_beginbuf && temp == current) {
		    if (current_x < mark_beginx) {
			mvwaddnstr(edit, lines, 0, temp->data, current_x);
			wattron(edit, A_REVERSE);
			mvwaddnstr(edit, lines, xplustabs(),
			&temp->data[current_x], mark_beginx - current_x);
			wattroff(edit, A_REVERSE);
			mvwaddnstr(edit, lines, xpt(temp, mark_beginx),
				   &temp->data[mark_beginx],
				   COLS - xpt(temp, mark_beginx));
		    } else {
			mvwaddnstr(edit, lines, 0, temp->data, mark_beginx);
			wattron(edit, A_REVERSE);
			mvwaddnstr(edit, lines, xpt(temp, mark_beginx),
				   &temp->data[mark_beginx], current_x - mark_beginx);
			wattroff(edit, A_REVERSE);
			mvwaddnstr(edit, lines, xplustabs(),
			     &temp->data[current_x], COLS - xplustabs());
		    }

		} else if (temp == mark_beginbuf) {
		    if (mark_beginbuf->lineno > current->lineno)
			wattron(edit, A_REVERSE);

		    mvwaddnstr(edit, lines, 0, temp->data, mark_beginx);

		    if (mark_beginbuf->lineno < current->lineno)
			wattron(edit, A_REVERSE);
		    else
			wattroff(edit, A_REVERSE);

		    mvwaddnstr(edit, lines, xpt(temp, mark_beginx),
			       &temp->data[mark_beginx], COLS - xpt(temp, mark_beginx));

		    if (mark_beginbuf->lineno < current->lineno)
			wattroff(edit, A_REVERSE);

		    if (strlenpt(temp->data) > COLS)
			mvwaddch(edit, lines, COLS - 1, '$');
		} else if (temp == current) {
		    if (mark_beginbuf->lineno < current->lineno)
			wattron(edit, A_REVERSE);

		    /* Thank GOD for waddnstr, this can now be much cleaner */
		    mvwaddnstr(edit, lines, 0, temp->data, current_x);

		    if (mark_beginbuf->lineno > current->lineno)
			wattron(edit, A_REVERSE);
		    else
			wattroff(edit, A_REVERSE);
/*
   mvwaddstr(edit, lines, xplustabs(), &temp->data[current_x]);
 */
		    mvwaddstr(edit, lines, xpt(current, current_x),
			      &temp->data[current_x]);

		    if (mark_beginbuf->lineno > current->lineno)
			wattroff(edit, A_REVERSE);

		    if (strlenpt(temp->data) > COLS)
			mvwaddch(edit, lines, COLS - 1, '$');
		}
	    }

	} else if (lines == current_y && xplustabs() > COLS - 2) {
	    col = (COLS - 1) * (xplustabs() / (COLS - 1));
	    mvwaddnstr(edit, lines, 1,
		       &temp->data[actual_x(current, col - 4)], COLS);

	    if (strlenpt(&temp->data[col]) > COLS)
		mvwaddch(edit, lines, COLS - 1, '$');
	    mvwaddch(edit, lines, 0, '$');
	} else {
	    mvwaddnstr(edit, lines, 0, temp->data, COLS);

	    if (strlenpt(temp->data) > COLS)
		mvwaddch(edit, lines, COLS - 1, '$');
	}

	temp = temp->next;
	lines++;
    }

    if (temp == filebot) {
	mvwaddstr(edit, lines, 0, hblank);
	mvwaddnstr(edit, lines, 0, filebot->data, COLS);
	lines++;
    }
    if (lines <= editwinrows - 1)
	while (lines <= editwinrows - 1) {
	    mvwaddstr(edit, lines, i, hblank);
	    lines++;
	}
    editbot = temp;
}

/*
 * Nice generic routine to update the edit buffer given a pointer to the
 * file struct =) 
 */
void edit_update(filestruct * fileptr)
{
    int lines = 0, i = 0;
    filestruct *temp;

    if (fileptr == NULL)
	return;

    temp = fileptr;
    while (i <= editwinrows / 2 && temp->prev != NULL) {
	i++;
	temp = temp->prev;
    }
    edittop = temp;

    while (lines <= editwinrows - 1 && lines <= totlines && temp != NULL
	   && temp != filebot) {
	mvwaddstr(edit, lines, 0, hblank);
	mvwaddnstr(edit, lines, 0, temp->data, COLS);
	temp = temp->next;
	lines++;
    }

    if (temp == filebot) {
	mvwaddstr(edit, lines, 0, hblank);
	mvwaddstr(edit, lines, 0, filebot->data);
	lines++;
	for (i = lines; i <= editwinrows - 1; i++)
	    mvwaddstr(edit, i, 0, hblank);
    }
    editbot = temp;
    wrefresh(edit);
}

/* This function updates current based on where current_y is, reset_cursor 
   does the opposite */
void update_cursor(void)
{
    int i = 0;

    wmove(edit, current_y, current_x);

#ifdef DEBUG
    fprintf(stderr, "Moved to (%d, %d) in edit buffer\n", current_y,
	    current_x);
#endif

    current = edittop;
    while (i <= current_y - 1 && current->next != NULL) {
	current = current->next;
	i++;
    }

#ifdef DEBUG
    fprintf(stderr, "current->data = \"%s\"\n", current->data);
#endif
/*   wrefresh(edit); */

}

/*
 * Ask a question on the statusbar.  Answer will be stored in answer
 * global.  Returns -1 on aborted enter, -2 on a blank string, and 0
 * otherwise, the valid shortcut key caught, Def is any editable text we
 * want to put up by default.
 */
int statusq(shortcut s[], int slen, char *def, char *msg,...)
{
    va_list ap;
    char foo[133];
    int ret;

    bottombars(s, slen);

    va_start(ap, msg);
    vsnprintf(foo, 132, msg, ap);
    strncat(foo, ": ", 132);
    va_end(ap);

    wattron(bottomwin, A_REVERSE);
    ret = nanogetstr(foo, def, s, slen, (strlen(foo) + 3));
    wattroff(bottomwin, A_REVERSE);


    switch (ret) {

    case NANO_FIRSTLINE_KEY:
	do_first_line();
	break;
    case NANO_LASTLINE_KEY:
	do_last_line();
	break;
    case NANO_CANCEL_KEY:
	return -1;
    default:
	blank_statusbar_refresh();
    }

#ifdef DEBUG
    fprintf(stderr, "I got \"%s\"\n", answer);
#endif

    return ret;
}

/*
 * Ask a simple yes/no question on the statusbar.  Returns 1 for Y, 0 for
 * N, 2 for All (if all is non-zero when passed in) and -1 for abort (^C)
 */
int do_yesno(int all, int leavecursor, char *msg,...)
{
    va_list ap;
    char foo[133];
    int kbinput, ok = -1;

    /* Write the bottom of the screen */
    clear_bottomwin();
    wattron(bottomwin, A_REVERSE);
    blank_statusbar_refresh();
    wattroff(bottomwin, A_REVERSE);

    wmove(bottomwin, 1, 0);
    onekey(" Y", "Yes");
    if (all)
	onekey(" A", "All");
    wmove(bottomwin, 2, 0);
    onekey(" N", "No");
    onekey("^C", "Cancel");

    va_start(ap, msg);
    vsnprintf(foo, 132, msg, ap);
    va_end(ap);
    wattron(bottomwin, A_REVERSE);
    mvwaddstr(bottomwin, 0, 0, foo);
    wattroff(bottomwin, A_REVERSE);
    wrefresh(bottomwin);

    if (leavecursor == 1)
	reset_cursor();

    raw();

    while (ok == -1) {
	kbinput = wgetch(edit);

	switch (kbinput) {
	case 'Y':
	case 'y':
	    ok = 1;
	    break;
	case 'N':
	case 'n':
	    ok = 0;
	    break;
	case 'A':
	case 'a':
	    if (all)
		ok = 2;
	    break;
	case NANO_CONTROL_C:
	    ok = -2;
	    break;
	}
    }
    noraw();
    cbreak();

    /* Then blank the screen */
    blank_statusbar_refresh();

    if (ok == -2)
	return -1;
    else
	return ok;
}

void statusbar(char *msg,...)
{
    va_list ap;
    char foo[133];
    int start_x = 0;

    va_start(ap, msg);
    vsnprintf(foo, 132, msg, ap);
    va_end(ap);

    start_x = center_x - strlen(foo) / 2 - 1;

    /* Blank out line */
    blank_statusbar_refresh();

    wmove(bottomwin, 0, start_x);
    wattron(bottomwin, A_REVERSE);

    waddstr(bottomwin, "[ ");
    waddstr(bottomwin, foo);
    waddstr(bottomwin, " ]");
    wattroff(bottomwin, A_REVERSE);
    wrefresh(bottomwin);

    statblank = 25;
}

void total_refresh(void)
{
    int i, j;

    bottombars(main_list, MAIN_LIST_LEN);
    for (i = 0; i != COLS; i++)
	mvwaddch(topwin, 0, i, ' ');
    wrefresh(topwin);
    titlebar();

    for (i = 0; i <= LINES - 1; i++)
	for (j = 0; j <= COLS; j++)
	    mvwaddch(edit, i, j, ' ');
    wrefresh(edit);

    edit_refresh();
    wrefresh(edit);
}

void previous_line(void)
{
    if (current_y > 0)
	current_y--;
}

void do_cursorpos(int sig)
{
    float linepct, bytepct;
    int i;

    if (current == NULL || fileage == NULL)
	return;

    /* FIXME - This is gardly elegant */
    if (current == fileage && strlen(current->data) == 0)
	i = 0;
    else
	i = current->bytes - strlen(current->data) + current_x - 1;

    if (totlines > 0)
	linepct = 100 * current->lineno / totlines;
    else
	linepct = 0;

    if (totsize > 0)
	bytepct = 100 * i / totsize;
    else
	bytepct = 0;

#ifdef DEBUG
    fprintf(stderr, "do_cursorpos: linepct = %f, bytepct = %f\n", linepct,
	    bytepct);
#endif

    statusbar("line %d of %d (%.0f%%), character %d of %d (%.0f%%)",
	      current->lineno, totlines, linepct, i, totsize, bytepct);
    reset_cursor();
}

/* Dump the current file structure to stderr */
void dump_buffer(filestruct * inptr)
{
#ifdef DEBUG
    filestruct *fileptr;

    if (inptr == fileage)
	fprintf(stderr, "Dumping file buffer to stderr...\n");
    else if (inptr == cutbuffer)
	fprintf(stderr, "Dumping cutbuffer to stderr...\n");
    else
	fprintf(stderr, "Dumping a buffer to stderr...\n");

    fileptr = inptr;
    while (fileptr != NULL) {
	fprintf(stderr, "(%ld) %s\n", fileptr->lineno, fileptr->data);
	fflush(stderr);
	fileptr = fileptr->next;
    }
#endif				/* DEBUG */
}

void dump_buffer_reverse(filestruct * inptr)
{
#ifdef DEBUG
    filestruct *fileptr;

    fileptr = filebot;
    while (fileptr != NULL) {
	fprintf(stderr, "(%ld) %s\n", fileptr->lineno, fileptr->data);
	fflush(stderr);
	fileptr = fileptr->prev;
    }
#endif				/* DEBUG */
}
