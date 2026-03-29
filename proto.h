/**************************************************************************
 *   proto.h                                                              *
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

/* Externs */

#include <sys/stat.h>
#include "nano.h"

extern int center_x, center_y, file, modified, editwinrows, editwineob;
extern int current_x, current_y, posible_max, keep_cutbuffer, totlines;
extern int suspend, case_sensitive, placewewant, statblank, mark_isset;
extern int mark_beginx, marked_cut, no_wrap, samelinewrap, totsize;

extern WINDOW *edit, *topwin, *bottomwin;
extern char filename[132], answer[132], last_search[132], last_replace[132];
extern char *hblank;
extern struct stat fileinfo;
extern filestruct *current, *fileage, *edittop, *editbot, *filebot; 
extern filestruct *cutbuffer, *cutbottom, *mark_beginbuf;
extern shortcut *shortcut_list;
extern shortcut main_list[MAIN_LIST_LEN], whereis_list[WHEREIS_LIST_LEN];
extern shortcut replace_list[REPLACE_LIST_LEN], goto_list[GOTO_LIST_LEN];
extern shortcut writefile_list[WRITEFILE_LIST_LEN];

/* Programs we want available */

char *strcasestr(const char *haystack, const char *needle);
char *strstrwrapper(char *haystack, char *needle);
int search_init(void);
int renumber(filestruct * fileptr);
int free_filestruct(filestruct * src);
int xplustabs(void);
int do_yesno(int all, int leavecursor, char *msg, ...);
int actual_x(filestruct * fileptr, int xplus);
int strlenpt(char *buf);
int statusq(shortcut s[], int slen, char *def, char *msg, ...);

void lowercase(char *src);
void do_search(void);
void blank_bottombars(void);
void check_wrap(filestruct * inptr);
void do_cut_text(filestruct * fileptr);
void do_uncut_text(filestruct * fileptr);
void dump_buffer(filestruct * inptr);
void align(char **strp);
void edit_refresh(void);
void edit_update(filestruct * fileptr);
void update_cursor(void);
void delete_node(filestruct * fileptr);
void set_modified(void);
void dump_buffer_reverse(filestruct * inptr);
void reset_cursor(void);
void do_cursorpos(int sig);
void check_statblank(void);
void update_line(filestruct * fileptr);
void statusbar(char *msg, ...);
void titlebar(void);
void previous_line(void);
void center_cursor(void);
void bottombars(shortcut s[], int slen);
void total_refresh(void);
void blank_statusbar_refresh(void);
void *nmalloc (size_t howmuch);

filestruct *copy_node(filestruct * src);
filestruct *copy_filestruct(filestruct * src);
filestruct *make_new_node(filestruct * prevnode);


