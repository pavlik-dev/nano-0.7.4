/**************************************************************************
 *   nano.c                                                               *
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#include "config.h"
#include "proto.h"
#include "nano.h"

/* What we do when we're all set to exit */
void finish(int sigage)
{
    blank_bottombars();
    wrefresh(bottomwin);
    endwin();

    exit(sigage);
}

/* Die (gracefully?) */
void die(char *msg,...)
{
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    clear();
    refresh();
    resetty();
    endwin();

    fprintf(stderr, msg);

    exit(1);			/* We have a problem: exit w/ errorlevel(1) */
}

/* Thanks BG, many ppl have been asking for this... */
void *nmalloc(size_t howmuch)
{
    void *r;

    /* Panic save? */

    if (!(r = malloc(howmuch)))
	die("nano: malloc: out of memory!");

    return r;
}

/* Initialize global variables - no better way for now */
void global_init(void)
{
    int i;

    center_x = COLS / 2;
    center_y = LINES / 2;
    current_x = 0;
    current_y = 0;
    editwinrows = LINES - 5;
    editwineob = editwinrows - 1;
    fileage = NULL;
    cutbuffer = NULL;
    current = NULL;
    hblank = nmalloc(COLS + 1);

    /* Thanks BG for this bit... */
    for (i = 0; i <= COLS - 1; i++)
	hblank[i] = ' ';
    hblank[i] = 0;

}

/* Make a copy of a node to a pointer (space will be malloc()ed */
filestruct *copy_node(filestruct * src)
{
    filestruct *dst;

    dst = nmalloc(sizeof(filestruct));
    dst->data = nmalloc(strlen(src->data) + 1);

    dst->next = src->next;
    dst->prev = src->prev;

    strcpy(dst->data, src->data);
    dst->lineno = src->lineno;

    return dst;
}

/* Unlink a node from the rest of the struct */
void unlink_node(filestruct * fileptr)
{
    if (fileptr->prev != NULL)
	fileptr->prev->next = fileptr->next;

    if (fileptr->next != NULL)
	fileptr->next->prev = fileptr->prev;
}

void delete_node(filestruct * fileptr)
{
    if (fileptr->data != NULL)
	free(fileptr->data);
    free(fileptr);
}

/* Okay, now let's duplicate a whole struct! */
filestruct *copy_filestruct(filestruct * src)
{
    filestruct *dst, *tmp, *head, *prev;

    head = copy_node(src);
    dst = head;			/* Else we barf on copying just one  line */
    head->prev = NULL;
    tmp = src->next;
    prev = head;

    while (tmp != NULL) {
	dst = copy_node(tmp);
	dst->prev = prev;
	prev->next = dst;

	prev = dst;
	tmp = tmp->next;
    }

    dst->next = NULL;
    return head;
}

/* Free() a single node */
int free_node(filestruct * src)
{
    if (src == NULL)
	return 0;

    if (src->next != NULL)
	free(src->data);
    free(src);
    return 1;
}

int free_filestruct(filestruct * src)
{
    filestruct *fileptr = src;

    if (src == NULL)
	return 0;

    while (fileptr->next != NULL) {
	fileptr = fileptr->next;
	free_node(fileptr->prev);

#ifdef DEBUG
	fprintf(stderr, "free_node(): free'd a node, YAY!\n");
#endif
    }
    free_node(fileptr);
#ifdef DEBUG
    fprintf(stderr, "free_node(): free'd last node.\n");
#endif

    return 1;
}

int renumber_all(void)
{
    filestruct *temp;
    long i = 1;

    for (temp = fileage; temp != NULL; temp = temp->next) {
	temp->lineno = i++;
	if (temp->prev != NULL)
	    temp->bytes = temp->prev->bytes + strlen(temp->data) + 1;
	else
	    temp->bytes = strlen(temp->data) + 1;

	totsize = temp->bytes;
    }

    return 0;
}

int renumber(filestruct * fileptr)
{
    filestruct *temp;

    if (fileptr == NULL || fileptr->prev == NULL || fileptr == fileage) {
	renumber_all();
	return 0;
    }
    for (temp = fileptr; temp != NULL; temp = temp->next) {
	temp->lineno = temp->prev->lineno + 1;
	temp->bytes = temp->prev->bytes + strlen(temp->data) + 1;
	totsize = temp->bytes;
    }

    return 0;
}

int update_bytes(filestruct * fileptr)
{
    filestruct *temp;

    if (fileptr == NULL) {
	return 0;
    }
    if (fileptr->prev == NULL) {
	if (fileptr->data != NULL)
	    fileptr->bytes = strlen(fileptr->data) + 1;
	else
	    fileptr->bytes = 0;

	totsize = fileptr->bytes;
	fileptr = fileptr->next;
    }
    for (temp = fileptr; temp != NULL; temp = temp->next) {
	temp->bytes = temp->prev->bytes + strlen(temp->data) + 1;
	totsize = temp->bytes;
    }

    return 0;
}

/* Fix the memory allocation for a string */
void align(char **strp)
{
    /* There was a serious bug here:  the new address was never
       stored anywhere... */

    *strp = realloc(*strp, strlen(*strp) + 1);
}

/* Load file into edit buffer - takes data from file struct */
void load_file(void)
{
    current = fileage;
    wmove(edit, current_y, current_x);
}

/* What happens when there is no file to open? aiee! */
void new_file(void)
{
    fileage = nmalloc(sizeof(filestruct));
    fileage->data = nmalloc(1);
    strcpy(fileage->data, "");
    fileage->prev = NULL;
    fileage->next = NULL;
    fileage->lineno = 1;
    filebot = fileage;
    edittop = fileage;
    editbot = fileage;
    current = fileage;
    totlines = 1;
}


int read_file(char *filename)
{
    long size, lines = 0, linetemp = 0;
    char input[2];		/* buffer */
    char buf[2048] = "";
    filestruct *fileptr = current, *tmp = NULL;
    int line1ins = 0;

    if (fileptr != NULL && fileptr->prev != NULL) {
	fileptr = fileptr->prev;
	tmp = fileptr;
    } else if (fileptr != NULL && fileptr->prev == NULL) {
	tmp = fileage;
	current = fileage;
	line1ins = 1;
    }
    /* Read the entire file into file struct */
    while ((size = read(file, input, 1)) > 0) {
	linetemp = 0;
	if (input[0] == '\n') {
	    if (line1ins) {
		/* Special case, insert with cursor on 1st line. */
		fileptr = nmalloc(sizeof(filestruct));
		fileptr->data = nmalloc(strlen(buf) + 2);
		strcpy(fileptr->data, buf);
		fileptr->bytes = strlen(fileptr->data) + 1;
		fileptr->prev = NULL;
		fileptr->next = fileage;
		fileptr->lineno = 1;
		line1ins = 0;
		fileage = fileptr;
	    } else if (fileage == NULL) {
		fileage = nmalloc(sizeof(filestruct));
		fileage->data = nmalloc(strlen(buf) + 2);
		strcpy(fileage->data, buf);
		fileage->lineno = 1;
		fileage->prev = NULL;
		fileage->next = NULL;
		fileage->bytes = strlen(fileage->data) + 1;
		filebot = fileage;
		fileptr = fileage;
	    } else {
		fileptr->next = nmalloc(sizeof(filestruct));
		fileptr->next->data = nmalloc(strlen(buf) + 2);
		strcpy(fileptr->next->data, buf);
		fileptr->next->prev = fileptr;
		fileptr->next->next = NULL;
		fileptr->next->lineno = fileptr->lineno + 1;
		fileptr->next->bytes = fileptr->bytes +
		    strlen(fileptr->next->data) + 1;
		fileptr = fileptr->next;
	    }
	    lines++;
	    buf[0] = 0;
	} else {
	    strncat(buf, input, 1);
	}
	totsize += size;
    }

    /* Did we not get a newline but still have stuff to do? */
    /* I am 100% sure this can be nicer, but there you go */
    if (buf[0] && line1ins) {
	/* Special case, insert with cursor on 1st line. */
	fileptr = nmalloc(sizeof(filestruct));
	fileptr->data = nmalloc(strlen(buf) + 2);
	strcpy(fileptr->data, buf);
	fileptr->bytes = strlen(fileptr->data) + 1;
	fileptr->prev = NULL;
	fileptr->next = fileage;
	fileptr->lineno = 1;
	line1ins = 0;
	fileage = fileptr;
	lines++;
	buf[0] = 0;
    } else if (buf[0] && fileage == NULL) {
	fileage = nmalloc(sizeof(filestruct));
	fileage->data = nmalloc(strlen(buf) + 2);
	strcpy(fileage->data, buf);
	fileage->lineno = 1;
	fileage->prev = NULL;
	fileage->next = NULL;
	fileage->bytes = strlen(fileage->data) + 1;
	filebot = fileage;
	fileptr = fileage;
    } else if (buf[0]) {
	fileptr->next = nmalloc(sizeof(filestruct));
	fileptr->next->data = nmalloc(strlen(buf) + 2);
	strcpy(fileptr->next->data, buf);
	fileptr->next->prev = fileptr;
	fileptr->next->next = NULL;
	fileptr->next->lineno = fileptr->lineno + 1;
	fileptr->next->bytes = fileptr->bytes +
	    strlen(fileptr->next->data) + 1;
	fileptr = fileptr->next;
	lines++;
	buf[0] = 0;
    }
    /* Did we even GET a file? */
    if (totsize == 0) {
	new_file();
	statusbar("Read %d lines", lines);
	return 1;
    }
    if (current != NULL) {
	fileptr->next = current;
	current->prev = fileptr;
	renumber(current);
	/* FIXME, update the bytes portion for the rest of the file also */
	current_x = 0;
	placewewant = 0;
	edit_update(fileptr);
    } else if (fileptr->next == NULL) {
	filebot = fileptr;
	load_file();
    }
    statusbar("Read %d lines", lines);
    totlines += lines;

    close(file);

    return 1;
}

/* Open the file (and decide if it exists) */
int open_file(char *filename, int insert)
{

    if (!strcmp(filename, "") || stat(filename, &fileinfo) == -1) {
	if (insert) {
	    statusbar("\"%s\" not found", filename);
	    return -1;
	} else {
	    /* We have a new file */
	    statusbar("New File");
	    new_file();
	}
    } else if ((file = open(filename, O_RDONLY)) == -1) {
	statusbar("%s: %s", strerror(errno), filename);
	return -1;
    } else {			/* File is A-OK */

	statusbar("Reading File");
	read_file(filename);
    }

    return 1;
}

int do_insertfile(void)
{
    int i;

    i = statusq(writefile_list, WRITEFILE_LIST_LEN, "",
		"File to insert [from ./] ");
    if (i != -1) {

#ifdef DEBUG
	fprintf(stderr, "filename is %s", answer);
#endif

	i = open_file(answer, 1);

	dump_buffer(fileage);
	set_modified();
	return i;
    } else {
	statusbar("Cancelled");
	return 0;
    }
}

void usage(void)
{
    printf(" Usage: nano -[vwz] +LINE <file>\n");
    printf(" -v: Print version information and exit\n");
    printf(" -w: Don't wrap long lines\n");
    printf(" -z: Enable suspend\n");
    exit(0);
}

void version(void)
{
    printf(" nano version %s by Chris Allegretta\n", VERSION);
}

void page_down(void)
{
    if (editbot->next != NULL && editbot->next != filebot) {
	edit_update(editbot->next);
	center_cursor();
    } else if (editbot != filebot) {
	edit_update(editbot);
	center_cursor();
    } else
	while (current != filebot) {
	    current = current->next;
	    current_y++;
	}

    update_cursor();
}

void do_home(void)
{
    current_x = 0;
    placewewant = 0;
}

void do_end(void)
{
    current_x = strlen(current->data);
    placewewant = xplustabs();
}

filestruct *make_new_node(filestruct * prevnode)
{
    filestruct *newnode;

    newnode = nmalloc(sizeof(filestruct));
    newnode->data = NULL;

    newnode->prev = prevnode;
    newnode->next = NULL;

    if (prevnode != NULL)
	newnode->lineno = prevnode->lineno + 1;

    return newnode;
}

void do_mark()
{
    if (!mark_isset) {
	statusbar("Mark Set");
	mark_isset = 1;
	mark_beginbuf = current;
	mark_beginx = current_x;
    } else {
	statusbar("Mark UNset");
	mark_isset = 0;
	mark_beginbuf = NULL;
	mark_beginx = 0;
    }
}

/* Someone hits return *gasp!* */
void do_enter(filestruct * inptr)
{
    filestruct *new;
    char *tmp;

    new = make_new_node(inptr);

    tmp = &current->data[current_x];
    new->data = nmalloc(strlen(tmp) + 1);
    strcpy(new->data, tmp);
    *tmp = 0;

    new->next = inptr->next;
    new->prev = inptr;
    inptr->next = new;
    if (new->next != NULL)
	new->next->prev = new;
    else
	filebot = new;

    renumber(current);
    current = new;
    current_x = 0;
    align(&current->data);

    if (current_y == editwinrows - 1)
	edit_update(current);

    totlines++;
    set_modified();
}

/* Actually wrap a line, called by check_wrap() */
void do_wrap(filestruct * inptr)
{
    int i, j;
    char *tmp, *foo;

    i = actual_x(inptr, COLS - 1);
    if (inptr->data[i] == ' ')
	while (inptr->data[i] == ' ' && i != 0)
	    i--;

    while (inptr->data[i] != ' ' && i != 0)
	i--;
    i++;

    if (samelinewrap) {
	tmp = &current->data[i];
	foo = nmalloc(strlen(tmp) + strlen(current->next->data) + 1);
	strcpy(foo, tmp);
	strcpy(&foo[strlen(tmp)], current->next->data);
	free(current->next->data);
	current->next->data = foo;
	*tmp = 0;
	if (current_x >= i) {
	    current_x = current_x - i;
	    current = current->next;
	}
	align(&current->next->data);
    } else {
	j = current_x;
	current_x = i;
	do_enter(current);
	if (j > i) {
	    current_x = j - i;
	    samelinewrap = 0;
	} else {
	    current_x = j;
	    current = current->prev;
	    samelinewrap = 1;
	}
    }

}

/* Check to see if we've just caused the line to wrap to a new line */
void check_wrap(filestruct * inptr)
{

#ifdef DEBUG
    fprintf(stderr, "check_wrap called with inptr->data=\"%s\"\n",
	    inptr->data);
#endif

    if ((int) strlenpt(inptr->data) <= COLS)
	return;
    else
	do_wrap(inptr);
}

/* Stuff we do when we abort from programs and want to clean up the
 * screen.  This doesnt do much right now.
 */
void do_early_abort(void)
{
    blank_statusbar_refresh();
}

/* Set up the system variables for a search or replace.  Returns -1 on
   abort, 0 on success, and 1 on rerun calling program */
int search_init(void)
{
    int i;

    if (strcmp(last_search, "")) {	/* There's a previous search stored */
	if (case_sensitive)
	    i = statusq(whereis_list, WHEREIS_LIST_LEN, "",
			"Case Sensitive Search [%s]", last_search);
	else
	    i = statusq(whereis_list, WHEREIS_LIST_LEN, "", "Search [%s]",
			last_search);

	if (i == -1) {		/* Aborted enter */
	    strncpy(answer, last_search, 132);
	    statusbar("Search Cancelled");
	    return -1;
	} else if (i == -2) {	/* Same string */
	    strncpy(answer, last_search, 132);
	} else if (i == 0) {	/* They actually entered something */
	    strncpy(last_search, answer, 132);

	    /* Blow away last_replace because they entered a new search
	       string....uh, right? =) */
	    strcpy(last_replace, "");
	} else if (i == NANO_CASE_KEY) {	/* They asked for case sensitivity */
	    case_sensitive = 1 - case_sensitive;
	    return 1;
	} else {		/* First page, last page, for example could get here */

	    do_early_abort();
	    return -3;
	}
    } else {			/* last_search is empty */

	if (case_sensitive)
	    i = statusq(whereis_list, WHEREIS_LIST_LEN, "",
			"Case Sensititve Search");
	else
	    i = statusq(whereis_list, WHEREIS_LIST_LEN, "", "Search");
	if (i < 0) {
	    statusbar("Search Cancelled");
	    reset_cursor();
	    return -1;
	} else if (i == 0)	/* They entered something new */
	    strncpy(last_search, answer, 132);
	else if (i == NANO_CASE_KEY) {	/* They want it case sensitive */
	    case_sensitive = 1 - case_sensitive;
	    return 1;
	} else {		/* First line key, etc. */

	    do_early_abort();
	    return -3;
	}
    }

    return 0;
}

filestruct *findnextstr(int quiet, filestruct * begin, char *needle)
{
    filestruct *fileptr;
    char *searchstr, *found = NULL, *tmp;

    fileptr = current;

    searchstr = &current->data[current_x + 1];
    /* Look for searchstr until EOF */
    while (fileptr != NULL &&
	   (found = strstrwrapper(searchstr, needle)) == NULL) {
	fileptr = fileptr->next;

	if (fileptr == begin)
	    return NULL;

	if (fileptr != NULL)
	    searchstr = fileptr->data;
    }

    /* If we're not at EOF, we found an instance */
    if (fileptr != NULL) {
	current = fileptr;
	current_x = 0;
	for (tmp = fileptr->data; tmp != found; tmp++)
	    current_x++;

	edit_update(current);
	reset_cursor();
    } else {			/* We're at EOF, go back to the top, once */

	fileptr = fileage;

	while (fileptr != current && fileptr != begin &&
	       (found = strstrwrapper(fileptr->data, needle)) == NULL)
	    fileptr = fileptr->next;

	if (fileptr == begin) {
	    if (!quiet)
		statusbar("\"%s\" not found", needle);

	    return NULL;
	}
	if (fileptr != current) {	/* We found something */
	    current = fileptr;
	    current_x = 0;
	    for (tmp = fileptr->data; tmp != found; tmp++)
		current_x++;

	    edit_update(current);
	    reset_cursor();

	    if (!quiet)
		statusbar("Search Wrapped");
	} else {		/* Nada */

	    if (!quiet)
		statusbar("\"%s\" not found", needle);
	    return NULL;
	}
    }

    return fileptr;
}

/* Search for a string */
void do_search(void)
{
    int i;
    filestruct *fileptr = current;

    if ((i = search_init()) == -1) {
	current = fileptr;
	return;
    } else if (i == -3) {
	return;
    } else if (i == 1) {
	do_search();
	return;
    }
    findnextstr(0, current, answer);
}

void print_replaced(int num)
{
    if (num > 1)
	statusbar("Replaced %d occurences", num);
    else if (num == 1)
	statusbar("Replaced 1 occurence");
}

/* Stuff we do every time we exit do_replace because we can get to it
   via a signal (SIGQUIT) */
void replace_abort(void)
{
    keep_cutbuffer = 0;
    bottombars(main_list, MAIN_LIST_LEN);
    reset_cursor();
}

/* Search for a string */
void do_replace(int signal /* I hate having to do this */ )
{
    int i, j, replaceall = 0, numreplaced = 0, beginx;
    filestruct *fileptr, *begin;
    char *tmp, *copy, prevanswer[132] = "";

    if ((i = search_init()) == -1) {
	statusbar("Replace Cancelled");
	replace_abort();
	return;
    } else if (i == 1) {
	do_replace(0);
	return;
    } else if (i == -3) {
	replace_abort();
	return;
    }
    strncpy(prevanswer, answer, 132);

    if (strcmp(last_replace, "")) {	/* There's a previous replace str */
	i = statusq(replace_list, REPLACE_LIST_LEN, "",
		    "Replace with [%s]", last_replace);

	if (i == -1) {		/* Aborted enter */
	    strncpy(answer, last_replace, 132);
	    statusbar("Replace Cancelled");
	    replace_abort();
	    return;
	} else if (i == 0)	/* They actually entered something */
	    strncpy(last_replace, answer, 132);
	else if (i == NANO_CASE_KEY) {	/* They asked for case sensitivity */
	    case_sensitive = 1 - case_sensitive;
	    do_replace(0);
	    return;
	} else {		/* First page, last page, for example could get here */

	    do_early_abort();
	    replace_abort();
	    return;
	}
    } else {			/* last_search is empty */

	i = statusq(replace_list, REPLACE_LIST_LEN, "", "Replace with");
	if (i == -1) {
	    statusbar("Replace Cancelled");
	    reset_cursor();
	    replace_abort();
	    return;
	} else if (i == 0)	/* They entered something new */
	    strncpy(last_replace, answer, 132);
	else if (i == NANO_CASE_KEY) {	/* They want it case sensitive */
	    case_sensitive = 1 - case_sensitive;
	    do_replace(0);
	    return;
	} else {		/* First line key, etc. */

	    do_early_abort();
	    replace_abort();
	    return;
	}
    }

    begin = current;
    beginx = current_x;
    while (1) {

	if (replaceall)
	    fileptr = findnextstr(1, begin, prevanswer);
	else
	    fileptr = findnextstr(0, begin, prevanswer);

	if (fileptr == NULL) {
	    current = begin;
	    current_x = beginx;
	    edit_update(current);
	    print_replaced(numreplaced);
	    replace_abort();
	    return;
	}
	/* If we're here, we've found the search string */
	if (!replaceall)
	    i = do_yesno(1, 1, "Replace this instance?");

	if (i > 0 || replaceall) {	/* Yes, replace it!!!! */
	    if (i == 2)
		replaceall = 1;

	    /* FIXME - lots of ugly code */
	    copy = nmalloc(strlen(current->data) - strlen(last_search) +
			   strlen(last_replace) + 1);

	    strncpy(copy, current->data, current_x);
	    copy[current_x] = 0;

	    strcat(copy, last_replace);

	    for (j = 1, tmp = current->data; j <=
		 (strlen(last_search) + current_x) && *tmp != 0; j++)
		tmp++;

	    if (*tmp != 0)
		strcat(copy, tmp);

	    tmp = current->data;
	    current->data = copy;

	    free(tmp);

	    edit_refresh();
	    set_modified();
	    numreplaced++;
	} else if (i == -1)	/* Abort, else do nothing and continue loop */
	    break;
    }

    print_replaced(numreplaced);
    replace_abort();
}


/* What happens when we want to go past the bottom of the buffer */
int do_down(void)
{
    if (current->next != NULL) {
	if (placewewant > 0)
	    current_x = actual_x(current->next, placewewant);

	if (current_x > strlen(current->next->data))
	    current_x = strlen(current->next->data);
    } else
	return 0;

    if (current_y < editwineob && current != editbot)
	current_y++;
    else
	page_down();

    return 1;
}

int page_up(void)
{
    if (edittop != fileage) {
	edit_update(edittop);
	center_cursor();
    } else
	current_y = 0;

    update_cursor();

    return 1;
}

void do_up(void)
{
    if (current->prev != NULL) {
	if (placewewant > 0)
	    current_x = actual_x(current->prev, placewewant);

	if (current_x > strlen(current->prev->data))
	    current_x = strlen(current->prev->data);
    }
    if (current_y > 0)
	current_y--;
    else
	page_up();

    wrefresh(edit);
}

void do_right(void)
{
    if (current_x < strlen(current->data)) {
	current_x++;
    } else {
	if (do_down())
	    current_x = 0;
    }

    placewewant = xplustabs();
}

void do_left(void)
{
    if (current_x > 0)
	current_x--;
    else if (current != fileage) {
	placewewant = 0;
	current_x = strlen(current->prev->data);
	do_up();
    }
    placewewant = xplustabs();
}

void delete_buffer(filestruct * inptr)
{
    if (inptr != NULL) {
	delete_buffer(inptr->next);
	free(inptr->data);
	free(inptr);
    }
}

void do_backspace(void)
{
    filestruct *previous;

    if (current_x != 0) {
	/* Let's get dangerous */
	memmove(&current->data[current_x - 1], &current->data[current_x],
		strlen(current->data) - current_x + 2);
#ifdef DEBUG
	fprintf(stderr, "current->data now = \"%s\"\n", current->data);
#endif
	align(&current->data);
	update_bytes(current);
	do_left();
    } else {
	if (current == fileage)
	    return;		/* Can't delete past top of file */

	previous = current->prev;
	current_x = strlen(previous->data);
	previous->data = realloc(previous->data,
				 strlen(previous->data) +
				 strlen(current->data) + 1);
	strcat(previous->data, current->data);
	if (current->next != NULL)
	    current->next->prev = current->prev;
	previous->next = current->next;

	unlink_node(current);
	delete_node(current);
	if (current == edittop)
	    page_up();
	current = previous;
	previous_line();
	totlines--;

	renumber(current);

#ifdef DEBUG
	fprintf(stderr, "After, data = \"%s\"\n", current->data);
#endif

    }

    set_modified();
    keep_cutbuffer = 0;
}

void do_delete(void)
{
    if (current_x != strlen(current->data)) {
	/* Let's get dangerous */
	memmove(&current->data[current_x], &current->data[current_x + 1],
		strlen(current->data) - current_x + 1);
#ifdef DEBUG
	fprintf(stderr, "current->data now = \"%s\"\n", current->data);
#endif
	align(&current->data);
	update_bytes(current);

    } else if (current->next != NULL) {
	current_x = 0;
	current = current->next;
	do_backspace();
    } else
	return;

    set_modified();
    keep_cutbuffer = 0;
}

void do_gotoline(long defline)
{
    long line, i = 1, j = 0;
    filestruct *fileptr;

    if (defline > 0)		/* We already know what line we want to go to */
	line = defline;
    else {			/* Ask for it */

	j =
	    statusq(replace_list, REPLACE_LIST_LEN, "",
		    "Enter line number");
	if (j == -1) {
	    statusbar("Aborted");
	    reset_cursor();
	    return;
	} else if (j != 0) {
	    do_early_abort();
	    return;
	}
	if (!strcmp(answer, "$")) {
	    current = filebot;
	    current_x = 0;
	    edit_update(current);
	    reset_cursor();
	    return;
	}
	line = atoi(answer);
    }

    /* Bounds check */
    if (line <= 0) {
	statusbar("Come on, be reasonable");
	return;
    }
    if (line > totlines) {
	statusbar("Only %d lines available, skipping to last line",
		  filebot->lineno);
	current = filebot;
	current_x = 0;
	edit_update(current);
	reset_cursor();
    } else {
	for (fileptr = fileage; fileptr != NULL && i < line; i++)
	    fileptr = fileptr->next;

	current = fileptr;
	current_x = 0;
	edit_update(current);
	reset_cursor();
    }

}

void wrap_reset(void)
{
    samelinewrap = 0;
}

int write_file(char *name)
{
    long size, lineswritten = 0;
    filestruct *fileptr;

    titlebar();
    fileptr = fileage;

    if ((file = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR
		     | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
	statusbar("Could not open file for writing: %s", strerror(errno));
	return -1;
    }
    dump_buffer(fileage);
    while (fileptr != NULL) {
	size = write(file, fileptr->data, strlen(fileptr->data));
	if (size == -1) {
	    statusbar("Could not open file for writing: %s",
		      strerror(errno));
	    return -1;
	} else {
#ifdef DEBUG
	    fprintf(stderr, "Wrote >%s\n", fileptr->data);
#endif
	}
	write(file, "\n", 1);

	fileptr = fileptr->next;
	lineswritten++;
    }

    strncpy(filename, name, 132);
    statusbar("Wrote %d lines", lineswritten);
    modified = 0;
    titlebar();
    return 1;
}

int do_writeout(void)
{
    int i;

    i = statusq(writefile_list, WRITEFILE_LIST_LEN, filename,
		"File Name to write");
    if (i != -1) {

#ifdef DEBUG
	fprintf(stderr, "filename is %s", answer);
#endif

	i = write_file(answer);

	return i;
    } else {
	statusbar("Cancelled");
	return 0;
    }
}


void do_exit(void)
{
    int i;

    if (!modified)
	finish(0);

    i =
	do_yesno(0, 0,
      "Save modified buffer (ANSWERING \"No\" WILL DESTROY CHANGES) ? ");

#ifdef DEBUG
    dump_buffer(fileage);
#endif

    if (i == 1) {
	if (do_writeout() > 0)
	    finish(0);
    } else if (i == 0)
	finish(0);
    else
	statusbar("Cancelled");

    bottombars(main_list, MAIN_LIST_LEN);
}

int main(int argc, char *argv[])
{
    int optchr;
    int kbinput;		/* Input from keyboard */
    long startline = 0;		/* Line to try and start at */

    while ((optchr = getopt(argc, argv, "h?cwvz")) != EOF) {
	switch (optchr) {
	case 'h':
	case '?':
	    usage();
	    exit(0);
	case 'v':
	    version();
	    exit(0);
	case 'z':
	    suspend = 1;
	    break;
	case 'w':
	    no_wrap = 1;
	    break;
	default:
	    usage();
	}
    }

    /* See if there's a non-option in argv (first non-option is the
       filename, if +LINE is not given) */
    if (argc == 1 || argc <= optind)
	strcpy(filename, "");
    else {
	/* Look for the +line flag... */
	if (argv[optind][0] == '+') {
	    startline = atoi(&argv[optind][1]);
	    optind++;
	    if (argc == 1 || argc <= optind)
		strcpy(filename, "");
	    else
		strncpy(filename, argv[optind], 132);
	} else
	    strncpy(filename, argv[optind], 132);

    }
    initscr();
    savetty();
    nonl();
    cbreak();
    noecho();
    timeout(0);

    /* Set up some global variables */
    global_init();

    signal(SIGINT, do_cursorpos);
    signal(SIGQUIT, do_replace);

    if (!suspend)
	signal(SIGTSTP, SIG_IGN);

#ifdef DEBUG
    fprintf(stderr, "Main: set up windows\n");
#endif

    /* Setup up the main text window */
    edit = newwin(editwinrows, COLS, 2, 0);
    keypad(edit, TRUE);

    /* And the other windows */
    topwin = newwin(2, COLS, 0, 0);
    bottomwin = newwin(3, COLS, LINES - 3, 0);
    keypad(bottomwin, TRUE);

#ifdef DEBUG
    fprintf(stderr, "Main: bottom win\n");
#endif
    /* Set up up bottom of window */
    bottombars(main_list, MAIN_LIST_LEN);

#ifdef DEBUG
    fprintf(stderr, "Main: open file\n");
#endif

    titlebar();
    if (argc == 1)
	new_file();
    else
	open_file(filename, 0);

    if (startline > 0)
	do_gotoline(startline);
    else
	edit_update(fileage);

    edit_refresh();
    reset_cursor();


    while (1) {
	kbinput = wgetch(edit);
	if (kbinput == 27) {	/* Grab Alt-key stuff first */
	    switch (kbinput = wgetch(edit)) {
	    case NANO_ALT_GOTO_KEY:
	    case NANO_ALT_GOTO_KEY - 32:
		kbinput = NANO_GOTO_KEY;
		break;
	    case NANO_ALT_REPLACE_KEY:
	    case NANO_REPLACE_KEY - 32:
		kbinput = NANO_REPLACE_KEY;
		break;
	    case 91:
		switch (kbinput = wgetch(edit)) {
		case 'A':
		    kbinput = KEY_UP;
		    break;
		case 'B':
		    kbinput = KEY_DOWN;
		    break;
		case 'C':
		    kbinput = KEY_RIGHT;
		    break;
		case 'D':
		    kbinput = KEY_LEFT;
		    break;
		default:
#ifdef DEBUG
		    fprintf(stderr, "I got Alt-[-%c! (%d)\n",
			    kbinput, kbinput);
#endif
		    break;
		}
		break;
	    default:
#ifdef DEBUG
		fprintf(stderr, "I got Alt-%c! (%d)\n", kbinput, kbinput);
#endif
		break;
	    }
	}
	switch (kbinput) {
	case NANO_EXIT_KEY:
	    do_exit();
	    break;
	case NANO_WRITEOUT_KEY:
	    do_writeout();
	    bottombars(main_list, MAIN_LIST_LEN);
	    break;
	case NANO_INSERTFILE_KEY:
	    wrap_reset();
	    do_insertfile();
	    keep_cutbuffer = 0;
	    bottombars(main_list, MAIN_LIST_LEN);
	    break;
	case NANO_GOTO_KEY:
	    wrap_reset();
	    do_gotoline(0);
	    keep_cutbuffer = 0;
	    bottombars(main_list, MAIN_LIST_LEN);
	    break;
	case NANO_WHEREIS_KEY:
	    wrap_reset();
	    do_search();
	    keep_cutbuffer = 0;
	    bottombars(main_list, MAIN_LIST_LEN);
	    wrefresh(bottomwin);
	    break;
	case NANO_CUT_KEY:
	    do_cut_text(current);
	    break;
	case NANO_CONTROL_A:
	case KEY_HOME:
	case 362:		/* For rxvt, xterm */
	    do_home();
	    break;
	case NANO_CONTROL_E:
	case KEY_END:
	case 385:		/* For rxvt, aterm */
	    do_end();
	    break;
	case NANO_PREVPAGE_KEY:
	case KEY_PPAGE:
	    wrap_reset();
	    current_x = 0;
	    page_up();
	    keep_cutbuffer = 0;
	    check_statblank();
	    break;
	case NANO_NEXTPAGE_KEY:
	case KEY_NPAGE:
	    wrap_reset();
	    current_x = 0;
	    page_down();
	    keep_cutbuffer = 0;
	    check_statblank();
	    break;
	case NANO_UNCUT_KEY:
	    wrap_reset();
	    do_uncut_text(current);
	    keep_cutbuffer = 0;
	    break;
	case NANO_CONTROL_6:	/* ^^ */
	    do_mark();
	    break;
	case NANO_SPELL_KEY:
	    statusbar("Spelling function not yet implemented");
	    break;
	case NANO_HELP_KEY:
	    statusbar("Help function not yet implemented, nyah!");
	    break;
	case NANO_REPLACE_KEY:
	    do_replace(0);
	    keep_cutbuffer = 0;
	    bottombars(main_list, MAIN_LIST_LEN);
	    break;
	case KEY_UP:
	case NANO_CONTROL_P:
	    wrap_reset();
	    do_up();
	    update_cursor();
	    keep_cutbuffer = 0;
	    check_statblank();
	    break;
	case KEY_DOWN:
	case NANO_CONTROL_N:
	    wrap_reset();
	    do_down();
	    update_cursor();
	    keep_cutbuffer = 0;
	    check_statblank();
	    break;
	case KEY_LEFT:
	case NANO_CONTROL_B:
	    do_left();
	    update_cursor();
	    keep_cutbuffer = 0;
	    check_statblank();
	    break;
	case KEY_RIGHT:
	case NANO_CONTROL_F:
	    do_right();
	    update_cursor();
	    keep_cutbuffer = 0;
	    check_statblank();
	    break;
	case KEY_BACKSPACE:
	case 127:
	    do_backspace();
	    break;
	case KEY_DC:
	case NANO_CONTROL_D:
	    do_delete();
	    break;
	case NANO_REFRESH_KEY:
	    total_refresh();
	    break;
	case KEY_ENTER:
	case NANO_CONTROL_M:	/* Enter (^M) - FIXME - must be more 
				   things bound to enter key */
	    do_enter(current);
	    break;
	case 331:		/* Stuff that we don't want to do squat */
	    break;
	default:
#ifdef DEBUG
	    fprintf(stderr, "I got %c (%d)!\n", kbinput, kbinput);
#endif
	    if (!isprint(kbinput) && kbinput != NANO_CONTROL_I)
		break;		/* Unhandled character sequence */

	    /* More dangerousness fun =) */
	    current->data =
		realloc(current->data, strlen(current->data) + 2);
	    memmove(&current->data[current_x + 1],
		    &current->data[current_x],
		    strlen(current->data) - current_x + 1);
	    current->data[current_x] = kbinput;
	    update_line(current);
	    do_right();
	    update_cursor();
	    update_bytes(current);
	    if (!no_wrap)
		check_wrap(current);
	    set_modified();
	    check_statblank();
	    keep_cutbuffer = 0;
	}
	edit_refresh();
	reset_cursor();

    }

    getchar();
    finish(0);

}
