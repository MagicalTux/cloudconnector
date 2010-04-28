/* cfg_files.c : configuration files parser
 * $Id$
 *
 *  cloudconnector
 *  Copyright (C) 2009 Mark Karpeles
 *
 */

/* System includes */
#include <stdlib.h>
#include <stdio.h> /* printf */
#include <string.h>

/* C99 */
#include <stdint.h>
#include <stdbool.h>

/* local includes */
#include "cfg_files.h"

struct config_file config_file[CONF_MAX_FILES];

/******
 * config_get_entry : search for an entry and return its pointer
 ******/

void *config_get_entry(int configid, char *entry, int index) {
	int i=0;
	struct conf_entry *temp_entry=NULL;

#if CONF_MAX_FILES < 255
	if (configid > CONF_MAX_FILES) return NULL;
#endif
	if (!config_file[configid].filename) return NULL;

	temp_entry=config_file[configid].first;
	while(temp_entry) {
		if(strcmp(temp_entry->param_name,entry)==0) {
			i++;
			if (i>index) return temp_entry->save_pointer;
		}
		temp_entry=temp_entry->next;
	}
	return NULL;
}

/******
 * config_add : add a config file to the config files list
 * The list of all config files is maintained in memory to help performing reloading...
 ******/
int config_add(const char *filename, unsigned char configid) {
	/* Add a config file to the system */
#if CONF_MAX_FILES < 255
	if (configid > CONF_MAX_FILES) return -1;
#endif
	if (config_file[configid].filename) return -1;
	config_file[configid].filename=filename;
	return 0;
}

/******
 * Return "bool" value of a switch configuration
 * on/off, english, français, deutsch, español
 ******/
static inline char config_read_bool(const char *str) {
	if (strcmp(str, "on") == 0 ||
			strcmp(str, "yes") == 0 ||
			strcmp(str, "oui") == 0 ||
			strcmp(str, "ja") == 0 ||
			strcmp(str, "si") == 0)
		return 1;
	if (strcmp(str, "off") == 0 ||
			strcmp(str, "no") == 0 ||
			strcmp(str, "non") == 0 ||
			strcmp(str, "nein") == 0)
		return 0;
	return (atoi(str))?1:0;
}

/******
 * Convert a string value to a numeric value. Supports hexadecimal and octal
 ******/
static inline int64_t config_read_numeric(const char *mystr,int min_limit, int max_limit) {
	int64_t myint;
	
	myint=strtoll(mystr, (char **)NULL, 0);
	if (min_limit && myint<min_limit) myint=min_limit;
	if (max_limit && myint>max_limit) myint=max_limit;
	return myint;
}

static void config_read_string(struct conf_entry *dest, struct conf_entry *src) {
	int len;

	len=strlen((char *)src->save_pointer);

	if (dest->min_limit && len<dest->min_limit) return; /* string too short */
	if (dest->max_limit && len>dest->max_limit) len=dest->max_limit; /* string too long */
	memcpy(dest->save_pointer, src->save_pointer, len);
	*((char *)dest->save_pointer+len)=0;
}

static void config_read_string2(struct conf_entry *dest, struct conf_entry *src) {
	int len;
	void *addr;

	len=strlen((char *)src->save_pointer);

	if (dest->min_limit && len<dest->min_limit) return; /* string too short */
	if (dest->max_limit && len>dest->max_limit) return; /* string too long */
	
	if ( (*(void **)dest->save_pointer) != NULL) {
		if ((dest->flags & CONF_ENTRY_FLAG_FREEPOINTER) != 0) free((*(void **)dest->save_pointer));
	}
	addr=strdup((char*)src->save_pointer);
	dest->flags = dest->flags | CONF_ENTRY_FLAG_FREEPOINTER;
	memcpy(dest->save_pointer, &addr, sizeof(void *));
}

void config_free_conf_entry(struct conf_entry *pointer) {
	/* check flags... */
	if (!pointer) {
#ifdef __DEBUG
		puts("WARNING: Trying to free an unallocated configuration entry !");
#endif
		return;
	}
	if (pointer->prev && pointer->next) {
		/* this node is in the middle of a chain. Let's relink the chain... */
		pointer->prev->next=pointer->next;
		pointer->next->prev=pointer->prev;
		pointer->prev=NULL;
		pointer->next=NULL;
	} else if (pointer->next) {
		pointer->next->prev=NULL;
	} else if (pointer->prev) {
		pointer->prev->next=NULL;
	}
	if (pointer->flags & CONF_ENTRY_FLAG_FREEPOINTER) {
		/* we allocated ourself the "save_pointer", unallocate it */
		free(pointer->save_pointer);
	}
	free(pointer->param_name);
	free(pointer);
}

struct conf_entry *config_make_conf_entry(char *entry_name, char dup_name, void *entry_pointer, char dup_pointer, int type) {
	struct conf_entry *new_entry;
	new_entry = calloc(1, sizeof(struct conf_entry));
	int size=0;
	new_entry->flags=0;
	new_entry->type=type;
	new_entry->prev=NULL;
	new_entry->next=NULL;
	if (dup_name) {
		new_entry->param_name=calloc(strlen(entry_name), sizeof(char));
		strcpy(new_entry->param_name, entry_name);
	} else {
		new_entry->param_name=entry_name;
	}
	if (!dup_pointer) {
		new_entry->save_pointer=entry_pointer;
		return new_entry;
	}
	switch(type) {
		case CONF_VAR_CHAR: case CONF_VAR_CHARBOOL: size=sizeof(char); break;
		case CONF_VAR_SHORT: size=sizeof(short); break;
		case CONF_VAR_INT: size=sizeof(int); break;
		case CONF_VAR_CALLBACK: return new_entry;
		case CONF_VAR_STRING: case CONF_VAR_ARRAY: size=strlen((char *)entry_pointer);
	}
	if (!size) return new_entry;
	new_entry->save_pointer=calloc(size, sizeof(char));
	memcpy(new_entry->save_pointer, entry_pointer, size);
	return new_entry;
}
	
struct conf_entry *config_dup_conf_entry(struct conf_entry *pointer) {
	struct conf_entry *new_entry;
	if (!pointer) return pointer;
	new_entry=calloc(1, sizeof(struct conf_entry));
	if (pointer->flags & CONF_ENTRY_FLAG_FREEPOINTER) {
		new_entry->save_pointer=calloc(strlen((char *)pointer->save_pointer), sizeof(char));
		strcpy((char *)new_entry->save_pointer, (char *)pointer->save_pointer);
	} else {
		new_entry->save_pointer = pointer->save_pointer;
	}
	new_entry->param_name=calloc(strlen((char *)pointer->param_name),sizeof(char));
	strcpy((char *)new_entry->param_name, (char *)pointer->param_name);
	new_entry->flags=pointer->flags;
	new_entry->type=pointer->type;
	return new_entry;
}

void config_clean_var_name(char *str) {
	int i,j;
	char start=1;
	for (i=0; i<CONF_LINE_MAXSIZE && str[i]; i++) {
		if (start && str[i]==32) {
			str[i]=1;
		} else {
			start=0;
		}
		if (str[i]<32) {
			for (j=i;j<CONF_LINE_MAXSIZE && str[j];j++) str[j]=str[j+1];
			i--;
			continue;
		} else if(str[i]==32) {
			str[i]='_';
			continue;
		}
		if ( (str[i]>='A') && (str[i]<='Z')) str[i]=str[i]+32;
	}
	for(i--; i>0; i--) {
		if(str[i]!='_') break;
		str[i]=0;
	}
}

void config_clean_var_value(char *str) {
	int i=0,j=0;
	int inquote=0,quoteisdouble=0,wasinquote=0;
	int lastwasspace=1;

	for(i=0;i<CONF_LINE_MAXSIZE && str[i];i++) {
		if (str[i]<32) str[i]=32;
		if (lastwasspace && str[i]==32) {
			for(j=i;j<CONF_LINE_MAXSIZE && str[j];j++) str[j]=str[j+1];
			i--;
			continue;
		}
		lastwasspace=(str[i]==32 && !inquote);
		wasinquote=inquote;
		if ((str[i]==34) || (str[i]==39)) {
			if (!inquote || (quoteisdouble && str[i]==34) || (!quoteisdouble && str[i]==39)) {
				inquote=1-inquote;
				if (inquote) {
					quoteisdouble=(str[i]==34);
				} else {
					lastwasspace=1;
				}
				for(j=i;j<CONF_LINE_MAXSIZE && str[j];j++) str[j]=str[j+1];
				i--;
				continue;
			}
		}
		if (inquote && str[i]==92) {
			if (quoteisdouble) {
				if (str[i+1]=='a') str[i+1]=7;
				if (str[i+1]=='b') str[i+1]=8;
				if (str[i+1]=='t') str[i+1]=9;
				if (str[i+1]=='n') str[i+1]=10;
				if (str[i+1]=='v') str[i+1]=11;
				if (str[i+1]=='f') str[i+1]=12;
				if (str[i+1]=='r') str[i+1]=13;
			}
			if ( ((str[i+1]>=7) && (str[i+1]<=13)) || str[i+1]==92 || str[i+1]==34 || str[i+1]==39) {
				for(j=i;j<CONF_LINE_MAXSIZE && str[j];j++) str[j]=str[j+1];
				continue;
			}
			if (!quoteisdouble) continue;
			if (str[i+1]=='x') {
				/* Hexadecimal code */
				if ( (str[i+2]>=48) && (str[i+2]<=57)) {
					j=str[i+2]-48;
				} else if ( (str[i+2]>=97) && (str[i+2]<=102)) {
					j=str[i+2]-87;
				} else if ( (str[i+2]>=65) && (str[i+2]<=70)) {
					j=str[i+2]-55;
				} else continue;
				j=j*16;
				if ((str[i+3]>=48) && (str[i+3]<=57)) {
					j=j+(str[i+3]-48);
				} else if ( (str[i+3]>=97) && (str[i+3]<=102)) {
					j=j+(str[i+3]-87);
				} else if ( (str[i+3]>=65) && (str[i+3]<=70)) {
					j=j+(str[i+3]-55);
				} else continue;
				str[i]=j;
				for(j=i+1;j<CONF_LINE_MAXSIZE && str[j];j++) str[j]=str[j+3];
			}
		}
	}
	if (!wasinquote && lastwasspace) for(i--;i>0;i--) {
		if (str[i]!=' ') break;
		str[i]=0;
	}
}

struct conf_entry *config_parse_line(const char *line) {
	struct conf_entry *tmp_entry;
	char w1[CONF_LINE_MAXSIZE], w2[CONF_LINE_MAXSIZE];
	int i=0,j=0;
	char *save_pointer;

	if ((line[0]=='/') && (line[1]=='/')) return NULL;
	if (line[0]==';') return NULL;
	/* attempt to locate ":" or "=" */
	w1[0]=0;
	w2[0]=0;
	for(i=0;i<CONF_LINE_MAXSIZE;i++) {
		if (!line[i]) break;
		if (!j) {
			if ((line[i]==34) || (line[i]==39)) {
				w1[i]=0;
				j=i;
				i--;
			} else if ((line[i]!=':') && (line[i]!='=')) {
				w1[i]=line[i];
			} else {
				w1[i]=0;
				j=i+1;
			}
		} else {
			w2[i-j]=line[i];
		}
	}
	if (!j) return NULL;
	w2[i-j]=0;

	config_clean_var_name(w1);
	config_clean_var_value(w2);

	tmp_entry=calloc(1, sizeof(struct conf_entry));
	tmp_entry->param_name=calloc(strlen(w1)+1,sizeof(char));
	save_pointer=calloc(strlen(w2)+1,sizeof(char));
	strcpy(tmp_entry->param_name, w1);
	strcpy(save_pointer, w2);
	tmp_entry->save_pointer=save_pointer;
	tmp_entry->type=CONF_VAR_STRING;
	tmp_entry->flags=CONF_ENTRY_FLAG_FREEPOINTER;
	return tmp_entry;
}

int config_add_var(unsigned char configid, char *var, void *pointer, char var_type, int min_limit, int max_limit, bool required) {
	struct conf_entry *temp_entry;
	struct conf_entry *base_entry;
	int i;

#if CONF_MAX_FILES < 255
	if (configid > CONF_MAX_FILES) return -1;
#endif
	if (!config_file[configid].filename) return -1;
	
	temp_entry=calloc(1,sizeof(struct conf_entry));
	temp_entry->param_name=var;
	temp_entry->save_pointer=pointer;
	temp_entry->type=var_type;
	temp_entry->min_limit=min_limit;
	temp_entry->max_limit=max_limit;
	if (required) {
		for(i=0;i<CONF_MAX_REQUIRED;i++) {
			if (config_file[configid].required[i] && strcmp(temp_entry->param_name,config_file[configid].required[i])==0) return 0;
		}
		for(i=0;i<CONF_MAX_REQUIRED;i++) {
			if (!config_file[configid].required[i]) {
				config_file[configid].required[i]=temp_entry->param_name;
				break;
			}
		}
	}
	if (!config_file[configid].first) {
		config_file[configid].first=temp_entry;
		return 0;
	}
	base_entry=config_file[configid].first;
	while(base_entry->next) base_entry=base_entry->next;
	base_entry->next=temp_entry;
	base_entry->next->prev=base_entry;
	return 0;
}

int config_set_var(unsigned char configid, struct conf_entry *pointer) {
	/* return 0 if we don't want our pointer to be freed */
	struct conf_entry *temp_entry=NULL;
	struct conf_entry *tmp2_entry=NULL;
	void (*callback)(void *,int,int);
	void *temp_pointer;
	int new_is_array=0;
#if CONF_MAX_FILES < 255
	if (configid > CONF_MAX_FILES) return 1;
#endif
	if (!config_file[configid].filename) return 1;

	temp_entry=config_file[configid].first;
	if (temp_entry && temp_entry->prev) {
#ifdef __DEBUG
		printf("WARNING: config_file[x].first wasn't pointing to first entry !\n");
#endif
		while(temp_entry->prev) temp_entry=temp_entry->prev;
		config_file[configid].first=temp_entry;
	}

	if (strcmp(pointer->param_name,"array")==0) {
		free(pointer->param_name);
		pointer->param_name=(char *)pointer->save_pointer;
		temp_pointer=(void *)pointer->param_name;
		while(*(char *)temp_pointer) {
			if (*(char *)temp_pointer>='A' && *(char *)temp_pointer<='Z') *(char *)temp_pointer+=32;
			temp_pointer++;
		}
		pointer->save_pointer=NULL;
		pointer->flags=pointer->flags & ~CONF_ENTRY_FLAG_FREEPOINTER;
		pointer->type=CONF_VAR_ARRAY;
		if (temp_entry) {
			while(temp_entry->next) temp_entry=temp_entry->next;
			temp_entry->next=pointer;
			temp_entry->next->prev=temp_entry;
			return 0;
		}
		config_file[configid].first=pointer;
		return 0;
	}

	/* try to find another entry with same name... */
	if (temp_entry) while(temp_entry) {
		if (strcmp(temp_entry->param_name,pointer->param_name)==0) {
			/* found something... check type... */
			switch(temp_entry->type) {
				case CONF_VAR_CHAR:
					*(char *)temp_entry->save_pointer=config_read_numeric((char *)pointer->save_pointer,temp_entry->min_limit,temp_entry->max_limit);
					return 1;
				case CONF_VAR_SHORT:
					*(short *)temp_entry->save_pointer=config_read_numeric((char *)pointer->save_pointer,temp_entry->min_limit,temp_entry->max_limit);
					return 1;
				case CONF_VAR_INT:
					*(int *)temp_entry->save_pointer=config_read_numeric((char *)pointer->save_pointer,temp_entry->min_limit,temp_entry->max_limit);
					return 1;
				case CONF_VAR_STRING:
					config_read_string(temp_entry,pointer);
					return 1;
				case CONF_VAR_STRING_POINTER:
					config_read_string2(temp_entry,pointer);
					return 1;
				case CONF_VAR_CALLBACK:
					callback=temp_entry->save_pointer;
					(*callback)(pointer->save_pointer,temp_entry->min_limit,temp_entry->max_limit);
					return 1;
				case CONF_VAR_CHARBOOL:
					*(char *)temp_entry->save_pointer=config_read_bool((char *)pointer->save_pointer);
					return 1;
				case CONF_VAR_ARRAY:
					if (temp_entry->save_pointer && (strcmp((char *)pointer->save_pointer,"clear")==0)) {
						new_is_array=1;
						tmp2_entry=temp_entry->prev;
						if (!tmp2_entry) tmp2_entry=temp_entry->next;
						config_free_conf_entry(temp_entry);
						temp_entry=tmp2_entry;
						break;
					}
					pointer->type=CONF_VAR_ARRAY;
					if (!temp_entry->save_pointer) {
						tmp2_entry=NULL;
						if (temp_entry->next) tmp2_entry=temp_entry->next;
						if (!tmp2_entry) tmp2_entry=temp_entry->prev;
						config_free_conf_entry(temp_entry);
						if (tmp2_entry) {
							temp_entry=tmp2_entry;
						} else {
							temp_entry=pointer;
							config_file[configid].first=temp_entry;
							return 0;
						}
					}
					while(temp_entry->next) temp_entry=temp_entry->next;
					temp_entry->next=pointer;
					temp_entry->next->prev=temp_entry;
					return 0;
				#
			}
		}
		if (temp_entry) temp_entry=temp_entry->next;
	}
	if (new_is_array) {
		/* A clear was received
		 * config_make_conf_entry(char *entry_name, char dup_name, void *entry_pointer, char dup_pointer, int type)
		 */
		temp_entry=config_file[configid].first;
		if (!temp_entry) {
			config_file[configid].first=config_make_conf_entry(pointer->param_name, 1, NULL, 0, CONF_VAR_ARRAY);
			return 1;
		}
		while(temp_entry->next) temp_entry=temp_entry->next;
		temp_entry->next=config_make_conf_entry(pointer->param_name, 1, NULL, 0, CONF_VAR_ARRAY);
		temp_entry->next->prev=temp_entry;
		return 1;
	}
	/* register var as string... */
	temp_entry=config_file[configid].first;
	if (!temp_entry) {
		config_file[configid].first=pointer;
	} else {
		while(temp_entry->next) temp_entry=temp_entry->next;
		temp_entry->next=pointer;
		temp_entry->next->prev=temp_entry;
	}
/*	printf("Register [%d] : \"%s\" = \"%s\"\n",configid, pointer->param_name, (char *)pointer->save_pointer); */
	return 0;
}

struct conf_entry *config_parse_file(const char *filename, char level, char *required[]) {
	char line[CONF_LINE_MAXSIZE];
	FILE *fp;
	struct conf_entry *base_entry=NULL;
	struct conf_entry *current_entry=NULL;
	struct conf_entry *temp_entry=NULL;
	struct conf_entry *import_entry=NULL;
	int i;

	if (level>15) {
		puts("WARNING: Too many levels of imported files! Aborting!!");
		return 0;
	}

	fp=fopen(filename,"r");
	if (fp == NULL) {
		fprintf(stderr, "Warning: couldn't open config file `%s'!\n", filename);
		return 0;
	}

	while(fgets(line, CONF_LINE_MAXSIZE - 1, fp)) {
		temp_entry=config_parse_line(line);
		if (!temp_entry) continue; /* parse error or empty line */
		if (strcmp(temp_entry->param_name,"import")==0) {
			import_entry=config_parse_file((char*)temp_entry->save_pointer, level+1, required);
			config_free_conf_entry(temp_entry);
			if (!import_entry) continue;
			if (!base_entry) {
				base_entry=import_entry;
			} else {
				current_entry->next=import_entry;
				current_entry->next->prev=current_entry;
			}
			while(current_entry->next) current_entry=current_entry->next; /* move to the end of the list */
			continue;
		}
		for(i=0;i<CONF_MAX_REQUIRED;i++) {
			if (required[i] && strcmp(temp_entry->param_name,required[i])==0) required[i]=NULL;
		}
		if (!base_entry) {
			base_entry=temp_entry;
		} else {
			current_entry->next=temp_entry;
			current_entry->next->prev=current_entry;
		}
		current_entry=temp_entry;
	}
	fclose(fp);

	return base_entry;
}

/* config_parse : run parse event on config file
 * This will parse the specified config file (open if from disk, etc...)
 */
bool config_parse(unsigned char configid) {
	struct conf_entry *base_entry=NULL;
	struct conf_entry *current_entry=NULL;
	struct conf_entry *next_entry=NULL;
	char *required[CONF_MAX_REQUIRED];
	int i,j=0;

#if CONF_MAX_FILES < 255
	if (configid > CONF_MAX_FILES) return false;
#endif
	if (!config_file[configid].filename) return false;

	for(i=0;i<CONF_MAX_REQUIRED;i++) required[i]=config_file[configid].required[i];
	base_entry=config_parse_file(config_file[configid].filename, 0, required);
	if (!base_entry) return false; /* parsing failed : no values found */

	for(i=0;i<CONF_MAX_REQUIRED;i++) {
		if (required[i]) {
			fprintf(stderr, "ERROR: Required variable %s not found!\n",required[i]);
			j++;
		}
	}
	if (j) {
		fprintf(stderr, "%d error(s) found. Parsing canceled.\n",j);
		current_entry=base_entry;
		while(current_entry) {
			base_entry=current_entry;
			current_entry=current_entry->next;
			config_free_conf_entry(base_entry);
		}
		return false;
	}

	current_entry=base_entry;
	while(current_entry) {
		base_entry=current_entry;
		next_entry=current_entry->next;
		current_entry->next=current_entry->prev=NULL;
		i=config_set_var(configid, current_entry);
		current_entry=next_entry;
		if (i>0) config_free_conf_entry(base_entry);
	}

#if 0
	printf("DEBUG: Dumping configuration\n");
	current_entry=config_file[configid].first;
	if (!current_entry) {
		printf("NO CONF\n");
		return true;
	}
	while(current_entry) {
		switch(current_entry->type) {
			case CONF_VAR_CHAR:
				printf("VAR [%s] = (char) %d\n",current_entry->param_name,*(char *)current_entry->save_pointer);
				break;
			case CONF_VAR_SHORT:
				printf("VAR [%s] = (short) %d\n",current_entry->param_name,*(short *)current_entry->save_pointer);
				break;
			case CONF_VAR_INT:
				printf("VAR [%s] = (int) %d\n",current_entry->param_name,*(int *)current_entry->save_pointer);
				break;
			case CONF_VAR_STRING:
				printf("VAR [%s] = (string) %s\n",current_entry->param_name,(char *)current_entry->save_pointer);
				break;
			case CONF_VAR_CHARBOOL:
				printf("VAR [%s] = (bool) %s\n",current_entry->param_name,(*(char *)current_entry->save_pointer)?"yes":"no");
				break;
			case CONF_VAR_ARRAY:
				printf("VAR [%s] = (array) %s\n",current_entry->param_name,(char *)current_entry->save_pointer);
				break;
			#
		}
		current_entry=current_entry->next;
	}
#endif

	return true;
}

