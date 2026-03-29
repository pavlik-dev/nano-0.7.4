/**************************************************************************
 *   global.c                                                             *
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


#define GLOBAL_FILE 1

#include <sys/stat.h>
#include "nano.h"

/*
 * Global variables
 */
int center_x = 0, center_y = 0;	/* Center of screen */
WINDOW *edit;			/* The file portion of the editor  */
WINDOW *topwin;			/* Top line of screen */
WINDOW *bottomwin;		/* Bottom buffer */
int file;			/* Actual file pointer */
char filename[132] = "";	/* Name of the file */
int modified = 0;		/* Has file been modified? */
struct stat fileinfo;		/* Informatio about the file */
int editwinrows = 0;		/* How many rows long is the edit
				   window? */
int editwineob = 0;		/* Last Line in edit buffer
				   (0 - editwineob) */
filestruct *current;		/* Current buffer pointer */
int current_x = 0, current_y = 0;	/* Current position of X and Y in
					   the editor - relative to edit
					   window (0,0) */
filestruct *fileage = NULL;	/* Our file buffer */
filestruct *edittop = NULL;	/* Pointer to the top of the edit
				   buffer with respect to the
				   file struct */
filestruct *editbot = NULL;	/* Same for the bottom */
filestruct *filebot = NULL;	/* Last node in the file struct */
filestruct *cutbuffer = NULL;	/* A place to store cut text */
filestruct *cutbottom = NULL;	/* Pointer to end of cutbuffer */
int keep_cutbuffer = 0;		/* Clear out the cutbuffer? */

char answer[132] = "";		/* Answer str to many questions */
char last_search[132] = "";	/* Last string we searched for */
char last_replace[132] = "";	/* Last replacement string */
int totlines = 0;		/* Total number of lines in the file */
int totsize = 0;		/* Total number of bytes in the file */
int suspend = 0;		/* Can nano be suspended */
int case_sensitive = 0;		/* Are we doing case sensitive
				   searches */
int placewewant = 0;		/* The collum we'd like the cursor
				   to jump to when we go to the
				   next or previous line */

int statblank = 0;		/* Number of keystrokes left after
				   we call statubar() before we
				   actually blank the statusbar */
int mark_isset = 0;		/* Is the marker currently active */
int no_wrap = 0;		/* Is wrapping turned on? */
int samelinewrap = 0;		/* Wrap to the beginning of the next line 
				   instead of starting a new one? */
char *hblank;			/* A horizontal blank line */

/* More stuff for the marker select */

filestruct *mark_beginbuf;	/* the begin marker buffer */
int mark_beginx;		/* X value in the string to start */
int marked_cut;			/* Is the cutbuffer from a mark */

shortcut main_list[MAIN_LIST_LEN] =
{
    {NANO_WRITEOUT_KEY, "Write Out"},
    {NANO_EXIT_KEY, "Exit"},
    {NANO_GOTO_KEY, "Goto Line"},
    {NANO_REPLACE_KEY, "Replace"},
    {NANO_INSERTFILE_KEY, "Read File"},
    {NANO_WHEREIS_KEY, "Where Is"},
    {NANO_PREVPAGE_KEY, "Prev Page"},
    {NANO_NEXTPAGE_KEY, "Next Page"},
    {NANO_CUT_KEY, "Cut Text"},
    {NANO_UNCUT_KEY, "Uncut Txt"},
    {NANO_CURSORPOS_KEY, "Cur Pos"},
    {NANO_SPELLING_KEY, "To Spell"}
};

shortcut whereis_list[WHEREIS_LIST_LEN] =
{
    {NANO_CASE_KEY, "Case Sens"},
    {NANO_CANCEL_KEY, "Cancel"},
    {NANO_FIRSTLINE_KEY, "First Line"},
    {NANO_LASTLINE_KEY, "Last Line"},
};

shortcut replace_list[REPLACE_LIST_LEN] =
{
    {NANO_CASE_KEY, "Case Sens"},
    {NANO_CANCEL_KEY, "Cancel"},
    {NANO_FIRSTLINE_KEY, "First Line"},
    {NANO_LASTLINE_KEY, "Last Line"},
};

shortcut goto_list[GOTO_LIST_LEN] =
{
    {NANO_FIRSTLINE_KEY, "First Line"},
    {NANO_LASTLINE_KEY, "Last Line"},
    {NANO_CANCEL_KEY, "Cancel"},
};

shortcut writefile_list[WRITEFILE_LIST_LEN] =
{
    {NANO_CANCEL_KEY, "Cancel"},
};
