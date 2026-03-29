/**************************************************************************
 *   nano.h                                                               *
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

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else /* Uh oh */
#include <curses.h> 
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifndef NANO_H
#define NANO_H 1

#define VERMSG "nano " VERSION

/* Structure types */
typedef struct filestruct {
    char *data;
    struct filestruct *next;	/* Next node */
    struct filestruct *prev;	/* Previous node */
    long bytes;			/* # of Bytes before this line */
    long lineno;		/* The line number */
} filestruct;

typedef struct shortcut {
   int val;		/* Actual sequence that generates the keystroke */
   char desc[50];       /* Description, e.g. "Page Up" */
} shortcut;

/* Control key sequences, chaning these would be very very bad */

#define NANO_CONTROL_A 1
#define NANO_CONTROL_B 2
#define NANO_CONTROL_C 3
#define NANO_CONTROL_D 4
#define NANO_CONTROL_E 5
#define NANO_CONTROL_F 6
#define NANO_CONTROL_G 7
#define NANO_CONTROL_H 8
#define NANO_CONTROL_I 9
#define NANO_CONTROL_J 10
#define NANO_CONTROL_K 11
#define NANO_CONTROL_L 12
#define NANO_CONTROL_M 13
#define NANO_CONTROL_N 14
#define NANO_CONTROL_O 15
#define NANO_CONTROL_P 16
#define NANO_CONTROL_Q 17
#define NANO_CONTROL_R 18
#define NANO_CONTROL_S 19
#define NANO_CONTROL_T 20
#define NANO_CONTROL_U 21
#define NANO_CONTROL_V 22
#define NANO_CONTROL_W 23
#define NANO_CONTROL_X 24
#define NANO_CONTROL_Y 25

#define NANO_CONTROL_4 28
#define NANO_CONTROL_5 29
#define NANO_CONTROL_6 30
#define NANO_CONTROL_7 31

#define NANO_ALT_A 'a'
#define NANO_ALT_B 'b'
#define NANO_ALT_C 'c'
#define NANO_ALT_D 'd'
#define NANO_ALT_E 'e'
#define NANO_ALT_F 'f'
#define NANO_ALT_G 'g'
#define NANO_ALT_H 'h'
#define NANO_ALT_I 'i'
#define NANO_ALT_J 'j'
#define NANO_ALT_K 'k'
#define NANO_ALT_L 'l'
#define NANO_ALT_M 'm'
#define NANO_ALT_N 'n'
#define NANO_ALT_O 'o'
#define NANO_ALT_P 'p'
#define NANO_ALT_Q 'q'
#define NANO_ALT_R 'r'
#define NANO_ALT_S 's'
#define NANO_ALT_T 't'
#define NANO_ALT_U 'u'
#define NANO_ALT_V 'v'
#define NANO_ALT_W 'w'
#define NANO_ALT_X 'x'
#define NANO_ALT_Y 'y'
#define NANO_ALT_Z 'z'

/* Some semi-changeable keybindings, dont play with unless you're sure you
know what you're doing */

#define NANO_INSERTFILE_KEY	NANO_CONTROL_R
#define NANO_EXIT_KEY 		NANO_CONTROL_X
#define NANO_WRITEOUT_KEY	NANO_CONTROL_O
#define NANO_GOTO_KEY		NANO_CONTROL_7
#define NANO_ALT_GOTO_KEY	NANO_ALT_G
#define NANO_HELP_KEY		NANO_CONTROL_G
#define NANO_WHEREIS_KEY	NANO_CONTROL_W
#define NANO_REPLACE_KEY	NANO_CONTROL_4
#define NANO_ALT_REPLACE_KEY	NANO_ALT_R
#define NANO_PREVPAGE_KEY	NANO_CONTROL_Y
#define NANO_NEXTPAGE_KEY	NANO_CONTROL_V
#define NANO_CUT_KEY		NANO_CONTROL_K
#define NANO_UNCUT_KEY		NANO_CONTROL_U
#define NANO_CURSORPOS_KEY	NANO_CONTROL_C
#define NANO_SPELLING_KEY	NANO_CONTROL_T
#define NANO_FIRSTLINE_KEY	NANO_PREVPAGE_KEY
#define NANO_LASTLINE_KEY	NANO_NEXTPAGE_KEY
#define NANO_CANCEL_KEY		NANO_CONTROL_C
#define NANO_CASE_KEY		NANO_CONTROL_A
#define NANO_REFRESH_KEY	NANO_CONTROL_L
#define NANO_SPELL_KEY		NANO_CONTROL_T


#define MAIN_LIST_LEN 12
#define WHEREIS_LIST_LEN 4
#define REPLACE_LIST_LEN 4
#define GOTO_LIST_LEN 3
#define WRITEFILE_LIST_LEN 1

#endif                             
