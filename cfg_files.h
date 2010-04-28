/* cfg_files.h : config files parser - header
 * $Id: cfg_files.h 2 2005-07-24 23:28:07Z magicaltux $
 *
 *  oledkbd - OLED-based keyboard driver
 *  Copyright (C) 2006 Mark Karpeles
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _CFG_FILES_H
#define _CFG_FILES_H

#define CONF_VAR_CHAR 0
#define CONF_VAR_SHORT 1
#define CONF_VAR_INT 2
#define CONF_VAR_STRING 3
#define CONF_VAR_STRING_POINTER 4
#define CONF_VAR_CALLBACK 5
#define CONF_VAR_CHARBOOL 6
#define CONF_VAR_ARRAY 7

#define CONF_OK 0
#define CONF_ERROR 1

#define CONF_MAX_FILES 10

#define CONFIG_CORE 0

#define CONF_LINE_MAXSIZE 1024
/* Max number of required variables */
#define CONF_MAX_REQUIRED 64

/* FreePointer : shall we free the pointer when closing? */
#define CONF_ENTRY_FLAG_FREEPOINTER 1

struct conf_entry {
	struct conf_entry *prev,*next;
	char *param_name; // 4 bytes + malloc'd string
	void *save_pointer; // 4 bytes
	int min_limit,max_limit; // 8 bytes : min. and max limit of the value (zero = not used)
	char type; // 1 byte
	char flags; // 1 byte
	// 2 bytes lost
};

struct config_file {
	struct conf_entry *first;
	const char *filename;
	int loader; // source
	char *required[CONF_MAX_REQUIRED];
};

int config_add(const char *, unsigned char);
bool config_parse(unsigned char);
int config_add_var(unsigned char, char *, void *, char, int, int, bool);
void *config_get_entry(int, char *, int);

#endif

