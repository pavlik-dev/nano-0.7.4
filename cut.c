/**************************************************************************
 *   cut.c                                                                *
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "proto.h"
#include "nano.h"

void add_to_cutbuffer(filestruct * inptr)
{
    filestruct *tmp;

#ifdef DEBUG
    fprintf(stderr, "add_to_cutbuffer called with inptr->data = %s\n",
	    inptr->data);
#endif

    tmp = cutbuffer;
    if (cutbuffer == NULL) {
	cutbuffer = inptr;
    } else {
	while (tmp->next != NULL)
	    tmp = tmp->next;
	tmp->next = inptr;
    }

    inptr->prev = tmp;
    inptr->next = NULL;
    cutbottom = inptr;
}

void cut_marked_segment(filestruct * top, int top_x, filestruct * bot,
			int bot_x)
{
    filestruct *tmp, *next;
    char *tmpstr;

    tmp = copy_node(top);
    tmpstr = nmalloc(strlen(&top->data[top_x]) + 1);
    strcpy(tmpstr, &top->data[top_x]);
    free(tmp->data);
    tmp->data = tmpstr;

    do {
	next = tmp->next;
	add_to_cutbuffer(tmp);
	totlines--;
	tmp = next;
    }
    while (next != bot && next != NULL);

    dump_buffer(cutbuffer);
    if (next == NULL)
	return;
    /* Now, paste bot[bot_x + 1] into top[top_x] */
    tmpstr = nmalloc(strlen(top->data) + strlen(&bot->data[bot_x + 1]));
    strncpy(tmpstr, top->data, top_x);
    strcpy(&tmpstr[top_x], &bot->data[bot_x + 1]);
    free(top->data);
    top->data = tmpstr;

    bot->data[bot_x + 1] = 0;
    align(&bot->data);
    next = bot->next;

    /* We explicitly don't decrement totlines here because we don't snarf
     * up a newline when we're grabbing the last line of the mark */

    add_to_cutbuffer(bot);
    top->next = next;
    if (next != NULL)
	next->prev = top;

    dump_buffer(cutbuffer);
    renumber(top);
    current = top;
    current_x = top_x;

    /* If we're hitting the end of the buffer we should clean that up. */
    if (bot == filebot) {
	if (next != NULL) {
	    filebot = next;
	} else {
	    filebot = top;
	}
    }
    if (top->lineno < edittop->lineno)
	edit_update(top);
}


void do_cut_text(filestruct * fileptr)
{
    filestruct *tmp;
    char *tmpstr;
    int newsize;

    if (fileptr == NULL || fileptr->data == NULL ||
	(filebot == fileptr && !strcmp(fileptr->data, "")))
	return;

    tmp = fileptr->next;

    if (!keep_cutbuffer || mark_isset) {
	free_filestruct(cutbuffer);
	cutbuffer = NULL;
#ifdef DEBUG
	fprintf(stderr, "Blew away cutbuffer =)\n");
#endif
    }
    if (mark_isset) {
	if (current->lineno == mark_beginbuf->lineno) {
	    tmp = copy_node(current);
	    newsize = abs(strlen(&current->data[mark_beginx]) -
			  strlen(&current->data[current_x]));

	    tmpstr = nmalloc(newsize);
	    if (current_x < mark_beginx) {
		strncpy(tmpstr, &current->data[current_x], newsize);
		memmove(&current->data[current_x],
			&current->data[mark_beginx],
			strlen(&current->data[mark_beginx] - newsize));
	    } else {
		strncpy(tmpstr, &current->data[mark_beginx], newsize);
		memmove(&current->data[mark_beginx],
			&current->data[current_x],
			strlen(&current->data[current_x] - newsize));
		current_x = mark_beginx;
		update_cursor();
	    }
	    tmpstr[newsize] = 0;
	    tmp->data = tmpstr;
	    add_to_cutbuffer(tmp);
	    dump_buffer(cutbuffer);
	    align(&current->data);
	} else if (current->lineno < mark_beginbuf->lineno)
	    cut_marked_segment(current, current_x, mark_beginbuf,
			       mark_beginx);
	else
	    cut_marked_segment(mark_beginbuf, mark_beginx, current,
			       current_x);

	placewewant = xplustabs();
	mark_isset = 0;
	marked_cut = 1;
	set_modified();
	return;
    } else if (fileptr == fileage) {
	/* we're cutting the first line */
	if (fileptr->next != NULL) {
	    fileptr = fileptr->next;
	    tmp = fileptr;
	    fileage = fileptr;
	    add_to_cutbuffer(fileptr->prev);
	    totlines--;
	    fileptr->prev = NULL;
	    edit_update(fileage);
	    current = fileptr;
	} else {
	    add_to_cutbuffer(fileptr);
	    totlines--;
	    fileage = make_new_node(NULL);
	    fileage->data = nmalloc(1);
	    strcpy(fileage->data, "");
	    current = fileage;
	}
    } else {
	if (fileptr->prev != NULL)
	    fileptr->prev->next = fileptr->next;

	if (fileptr->next != NULL) {
	    (fileptr->next)->prev = fileptr->prev;
	    current = fileptr->next;
	    totlines--;
	} else {
	    /* we're deleteing the last line
	       and replacing it with a dummy line,
	       so totlines is the same */
	    tmp = fileptr->prev;
	    tmp->next = make_new_node(tmp);
	    tmp = tmp->next;
	    tmp->data = nmalloc(1);
	    strcpy(tmp->data, "");
	    current = tmp;
	    filebot = tmp;
	}

	add_to_cutbuffer(fileptr);
    }

    if (fileptr == edittop)
	edittop = current;

    edit_refresh();

    dump_buffer(cutbuffer);
    reset_cursor();

    set_modified();
    keep_cutbuffer = 1;
    marked_cut = 0;
    current_x = 0;
    placewewant = 0;
    update_cursor();
    renumber(tmp);
}

void do_uncut_text(filestruct * fileptr)
{
    filestruct *tmp = fileptr, *newbuf, *newend;
    char *tmpstr, *tmpstr2;
    int i;

#ifdef DEBUG
    fprintf(stderr, "do_uncut_text called with fileptr->data = %s\n",
	    fileptr->data);
#endif

    if (cutbuffer == NULL || fileptr == NULL)
	return;			/* AIEEEEEEEEEEEE */

    newbuf = copy_filestruct(cutbuffer);
    for (newend = newbuf; newend->next != NULL && newend != NULL;
	 newend = newend->next) {
	totlines++;
    }

    /* Hook newbuf into fileptr */
    if (marked_cut == 1) {
	/* If there's only one line in the cutbuffer */
	if (cutbuffer->next == NULL) {
	    tmpstr =
		nmalloc(strlen(current->data) + strlen(cutbuffer->data) +
			1);
	    strncpy(tmpstr, current->data, current_x);
	    strcpy(&tmpstr[current_x], cutbuffer->data);
	    strcat(tmpstr, &current->data[current_x]);
	    free(current->data);
	    current->data = tmpstr;
	    current_x += strlen(cutbuffer->data);

	    placewewant = xplustabs();
	    update_cursor();
	    renumber(current);
	} else {		/* yuck -- no kidding! */
	    tmp = current->next;
	    /* New beginning */
	    tmpstr = nmalloc(current_x + strlen(newbuf->data) + 1);
	    strncpy(tmpstr, current->data, current_x);
	    strcpy(&tmpstr[current_x], newbuf->data);

	    /* New end */
	    tmpstr2 = nmalloc(strlen(newend->data) +
			      strlen(&current->data[current_x]) + 1);
	    strcpy(tmpstr2, newend->data);
	    strcat(tmpstr2, &current->data[current_x]);

	    free(current->data);
	    current->data = tmpstr;
	    current->next = newbuf->next;
	    newbuf->next->prev = current;
	    delete_node(newbuf);

	    current_x = strlen(newend->data);
	    placewewant = xplustabs();
	    free(newend->data);
	    newend->data = tmpstr2;

	    newend->next = tmp;

	    /* If tmp isn't null, we're in the middle: update the
	     * prev pointer.  If it IS null, we're at the end, update
	     * the filebot pointer */

	    if (tmp != NULL)
		tmp->prev = newend;
	    else
		filebot = newend;

	    i = editbot->lineno;

	    renumber(current);

	    current = newend;
	    if (i <= newend->lineno)
		edit_update(current);
	}

	dump_buffer(fileage);
	dump_buffer(cutbuffer);
	set_modified();
	return;
    } else if (fileptr != fileage) {
	tmp = fileptr->prev;
	tmp->next = newbuf;
	newbuf->prev = tmp;
	totlines++;		/* Unmarked uncuts don't split lines */
    } else {
	fileage = newbuf;
	totlines++;		/* Unmarked uncuts don't split lines */
    }

    /* This is so uncutting at the top of the buffer will work => */
    if (current_y == 0)
	edittop = newbuf;

    /* Connect the end of the buffer to the filestruct */
    newend->next = fileptr;
    fileptr->prev = newend;

    i = editbot->lineno;
    renumber(newbuf);
    if (i < newend->lineno)
	edit_update(fileptr);

    dump_buffer_reverse(fileptr);

    set_modified();
}
