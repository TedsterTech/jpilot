/* rcfile.h
 *
 * Copyright (C) 1999 by Judd Montgomery
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __PREFS_H__
#define __PREFS_H__

#define PREF_RCFILE 0
#define PREF_TIME 1
#define PREF_SHORTDATE 2
#define PREF_LONGDATE 3
#define PREF_FDOW 4 /*First Day Of the Week */
#define PREF_SHOW_DELETED 5
#define PREF_SHOW_MODIFIED 6
#define PREF_HIDE_COMPLETED 7
#define PREF_HIGHLIGHT 8
#define PREF_PORT 9
#define PREF_RATE 10
#define PREF_USER 11
#define PREF_USER_ID 12
#define PREF_PC_ID 13
#define PREF_NUM_BACKUPS 14
#define PREF_WINDOW_WIDTH 15
#define PREF_WINDOW_HEIGHT 16
#define PREF_DATEBOOK_PANE 17
#define PREF_ADDRESS_PANE 18
#define PREF_TODO_PANE 19
#define PREF_MEMO_PANE 20
#define PREF_USE_DB3 21
#define PREF_LAST_APP 22
#define PREF_PRINT_THIS_MANY 23
#define PREF_PRINT_ONE_PER_PAGE 24
#define PREF_NUM_BLANK_LINES 25
#define PREF_PRINT_COMMAND 26
#define PREF_CHAR_SET 27
#define PREF_SYNC_DATEBOOK 28
#define PREF_SYNC_ADDRESS 29
#define PREF_SYNC_TODO 30
#define PREF_SYNC_MEMO 31
#define PREF_ADDRESS_NOTEBOOK_PAGE 32
#define PREF_OUTPUT_HEIGHT 33
#define PREF_OPEN_ALARM_WINDOWS 34
#define PREF_DO_ALARM_COMMAND 35
#define PREF_ALARM_COMMAND 36
#define PREF_REMIND_IN 37
#define PREF_REMIND_UNITS 38
#define PREF_PASSWORD 39

#define NUM_PREFS 40

#define NUM_SHORTDATES  7
#define NUM_LONGDATES  6
#define NUM_TIMES  10
#define NUM_RATES  11

#define MAX_PREF_NUM_BACKUPS 99
#define INTTYPE 0
#define CHARTYPE 1

#define PREF_MDY 0
#define PREF_DMY 1
#define PREF_YMD 2

#define CHAR_SET_ENGLISH  0
#define CHAR_SET_JAPANESE 1
#define CHAR_SET_1250     2 /* Czech */
#define CHAR_SET_1251     3 /* Russian; palm koi8-r, host win1251 */
#define CHAR_SET_1251_B   4 /* Russian; palm win1251, host koi8-r */
#define CHAR_SET_TRADITIONAL_CHINESE  5 /* Taiwan Chinese */
#define CHAR_SET_KOREAN   6 /* Korean Hangul */
#define NUM_CHAR_SETS     7

#define MAX_PREF_VALUE 80

typedef struct {
   char *name;
   int usertype;
   int filetype;
   long ivalue;
   char svalue[MAX_PREF_VALUE+2];
} prefType;

/*extern prefType glob_prefs[NUM_PREFS]; */

int read_rc_file();
int write_rc_file();
int get_pref_possibility(int which, int n, char *ret);
int get_pref(int which, long *n, const char **ret);
int get_pref_dmy_order();
int get_pref_time_no_secs(char *datef);
int get_pref_time_no_secs_no_ampm(char *datef);
int set_pref(int which, long n);
int set_pref_char(int which, char *string);
/*This function is used externally to free up any memory that prefs is using */
void free_prefs();

/*
 * Get the preference value as long. If failed to do so, return the
 * specified default.
 */
long get_pref_int_default(int which, long defval);

#endif