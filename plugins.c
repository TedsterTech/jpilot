/* plugins.c
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

#include "config.h"
#ifdef  ENABLE_PLUGINS
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "log.h"
#include "plugins.h"
#include <dlfcn.h>

GList *plugins = NULL;

static int get_plugin_info(struct plugin_s *p, char *path);
static int get_plugin_sync_bits();

/*
 * Write out the jpilot.plugins file that tells which plugins to sync
 */
void write_plugin_sync_file()
{
   FILE *out;
   GList *temp_list;
   struct plugin_s *Pplugin;

   out=jp_open_home_file("jpilot.plugins", "w");
   if (!out) {
      return;
   }
   fwrite("Version 1\n", strlen("Version 1\n"), 1, out);
   for (temp_list = plugins; temp_list; temp_list = temp_list->next) {
      Pplugin = temp_list->data;
      if (Pplugin) {
	 if (Pplugin->sync_on) {
	    fwrite("Y ", 2, 1, out);
	 } else {
	    fwrite("N ", 2, 1, out);
	 }
	 fwrite(Pplugin->full_path, strlen(Pplugin->full_path), 1, out);
	 fwrite("\n", strlen("\n"), 1, out);
      }
   }
   fclose(out);
}


/* 
 * This is just a repeated subroutine to load_plugins not needing 
 * a name of its own.
 * Assumes dir has already been checked
 */
int load_plugins_sub1(DIR *dir, char *path, int *number, unsigned char user_only)
{
   int i, r;
   int count;
   struct dirent *dirent;
   char full_name[256];
   struct plugin_s temp_plugin, *new_plugin;
   
   count = 0;
   for (i=0; (dirent = readdir(dir)); i++) {
      if (i>1000) {
	 jpilot_logf(LOG_WARN, "load_plugins_sub1(): infinite loop\n");
	 return 0;
      }
      if (strcmp(&(dirent->d_name[strlen(dirent->d_name)-3]), ".so")) {
	 continue;
      } else {
	 jpilot_logf(LOG_DEBUG, "found plugin %s\n", dirent->d_name);
	 /* We know path has a trailing slash after it */
	 g_snprintf(full_name, 250, "%s%s", path, dirent->d_name);
	 r = get_plugin_info(&temp_plugin, full_name);
	 temp_plugin.number = *number;
	 temp_plugin.user_only = user_only;
	 if (r==0) {
	    if (temp_plugin.name) {
	       jpilot_logf(LOG_DEBUG, "plugin name is [%s]\n", temp_plugin.name);
	    }
	    new_plugin = malloc(sizeof(struct plugin_s));
	    if (!new_plugin) {
	       jpilot_logf(LOG_DEBUG, "load plugins(): Out of memory\n");
	       return count;
	    }
	    memcpy(new_plugin, &temp_plugin, sizeof(struct plugin_s));
	    plugins = g_list_append(plugins, new_plugin);
	    count++;
	    (*number)++;
	 }
      }
   }
   return count;
}
      
int load_plugins()
{
   DIR *dir;
   char path[256];
   int count, number;
   GList *temp_list;
   
   count = 0;
   number = DATEBOOK + 100; /* I just made up this number */
   plugins = NULL;
   
   g_snprintf(path, 250, "%s/%s/%s/%s/", BASE_DIR, "lib", EPN, "plugins");
   jpilot_logf(LOG_DEBUG, "opening dir %s\n", path);
   cleanup_path(path);
   dir = opendir(path);
   if (dir) {
      count += load_plugins_sub1(dir, path, &number, 0);
      closedir(dir);
   }
   
   get_home_file_name("plugins/", path, 240);
   cleanup_path(path);
   jpilot_logf(LOG_DEBUG, "opening dir %s\n", path);
   dir = opendir(path);
   if (dir) {
      count += load_plugins_sub1(dir, path, &number, 1);
      closedir(dir);
   }
   /* Go to first entry in the list */
   for (temp_list = plugins; temp_list; temp_list = temp_list->prev) {
      plugins = temp_list;
   }
         
   get_plugin_sync_bits();

   return count;
}

/* Now we need to look in the jpilot_plugins file to see which plugins
 * are enabled to sync and which are not
 */
static int get_plugin_sync_bits()
{
   int i;
   GList *temp_list;
   struct plugin_s *Pplugin;
   char line[256];
   char *Pline;
   char *Pc;
   FILE *in;

   in=jp_open_home_file("jpilot.plugins", "r");
   if (!in) {
      return 0;
   }
   for (i=0; (!feof(in)); i++) {   
      if (i>1000) {
	 jpilot_logf(LOG_WARN, "load_plugins(): infinite loop\n");
	 fclose(in);
	 return 0;
      }
      line[0]='\0';
      Pc = fgets(line, 1000, in);
      if (!Pc) {
	 break;
      }
      if (line[strlen(line)-1]=='\n') {
	 line[strlen(line)-1]='\0';
      }
      if ((!strncmp(line, "Version", 7)) && (strcmp(line, "Version 1"))) {
	 jpilot_logf(LOG_WARN, "While reading jpilot.plugins line 1:[%s]\n", line);
	 jpilot_logf(LOG_WARN, "Wrong Version\n");
	 jpilot_logf(LOG_WARN, "Check preferences->conduits\n");
	 fclose(in);
	 return 0;
      }
      if (i>0) {
	 if (toupper(line[0])=='N') {
	    Pline = line + 2;
	    for (temp_list = plugins; temp_list; temp_list = temp_list->next) {
	       Pplugin = temp_list->data;
	       if (!strcmp(Pline, Pplugin->full_path)) {
		  Pplugin->sync_on=0;
	       }
	    }
	 }
      }
   }
   fclose(in);
   return 0;
}

static int get_plugin_info(struct plugin_s *p, char *path)
{
   void *h;
   const char *err;
   char name[52];
   char db_name[52];
   int version, major_version, minor_version;
   void (*plugin_versionM)(int *major_version, int *minor_version);
   
   p->full_path = NULL;
   p->handle = NULL;
   p->sync_on = 1;
   p->name = NULL;
   p->db_name = NULL;
   p->number = 0;
   p->plugin_get_name = NULL;
   p->plugin_get_menu_name = NULL;
   p->plugin_get_help_name = NULL;
   p->plugin_get_db_name = NULL;
   p->plugin_startup = NULL;
   p->plugin_gui = NULL;
   p->plugin_help = NULL;
   p->plugin_print = NULL;
   p->plugin_gui_cleanup = NULL;
   p->plugin_pre_sync = NULL;
   p->plugin_sync = NULL;
   p->plugin_post_sync = NULL;
   p->plugin_exit_cleanup = NULL;
   
   h = dlopen(path, RTLD_NOW);
   if (!h) {
      jpilot_logf(LOG_WARN, "open failed on plugin [%s]\n error [%s]\n", path,
		  dlerror());
      return -1;
   }
   jpilot_logf(LOG_DEBUG, "opened plugin [%s]\n", path);
   p->handle=h;
   
   p->full_path = strdup(path);

   /* plugin_versionM */
   plugin_versionM = dlsym(h, "plugin_version");
   if (plugin_versionM==NULL)  {
      err = dlerror();
      jpilot_logf(LOG_WARN, "plugin_version: [%s]\n", err);
      jpilot_logf(LOG_WARN, " plugin is invalid: [%s]\n", path);
      dlclose(h);
      p->handle=NULL;
      return -1;
   }
   plugin_versionM(&major_version, &minor_version);
   version=major_version*1000+minor_version;
   if ((major_version <= 0) && (minor_version < 99)) {
      jpilot_logf(LOG_WARN, "Plugin:[%s]\n", path);
      jpilot_logf(LOG_WARN, "This plugin is version (%d.%d).\n",
		  major_version, minor_version);
      jpilot_logf(LOG_WARN, "It is too old to work with this version of J-Pilot.\n");
      dlclose(h);
      p->handle=NULL;
      return -1;
   }
   jpilot_logf(LOG_DEBUG, "This plugin is version (%d.%d).\n",
	       major_version, minor_version);


   /* plugin_get_name */
   jpilot_logf(LOG_DEBUG, "getting plugin_get_name\n");
   p->plugin_get_name = dlsym(h, "plugin_get_name");
   if (p->plugin_get_name==NULL)  {
      err = dlerror();
      jpilot_logf(LOG_WARN, "plugin_get_name: [%s]\n", err);
      jpilot_logf(LOG_WARN, " plugin is invalid: [%s]\n", path);
      dlclose(h);
      p->handle=NULL;
      return -1;
   }

   if (p->plugin_get_name) {
      p->plugin_get_name(name, 50);
      name[50]='\0';
      p->name = strdup(name);
   } else {
      p->name = NULL;
   }
   
   
   /* plugin_get_menu_name */
   jpilot_logf(LOG_DEBUG, "getting plugin_get_menu_name\n");
   p->plugin_get_menu_name = dlsym(h, "plugin_get_menu_name");
   if (p->plugin_get_menu_name) {
      p->plugin_get_menu_name(name, 50);
      name[50]='\0';
      p->menu_name = strdup(name);
   } else {
      p->menu_name = NULL;
   }
   

   /* plugin_get_help_name */
   jpilot_logf(LOG_DEBUG, "getting plugin_get_help_name\n");
   p->plugin_get_help_name = dlsym(h, "plugin_get_help_name");
   if (p->plugin_get_help_name) {
      p->plugin_get_help_name(name, 50);
      name[50]='\0';
      p->help_name = strdup(name);
   } else {
      p->help_name = NULL;
   }

   /* plugin_get_db_name */
   jpilot_logf(LOG_DEBUG, "getting plugin_get_db_name\n");
   p->plugin_get_db_name = dlsym(h, "plugin_get_db_name");

   if (p->plugin_get_db_name) {
      p->plugin_get_db_name(db_name, 50);
      db_name[50]='\0';
   } else {
      db_name[0]='\0';
   }
  
   p->db_name = strdup(db_name);

   
   /* plugin_gui */
   p->plugin_gui = dlsym(h, "plugin_gui");

   /* plugin_help */
   p->plugin_help = dlsym(h, "plugin_help");

   /* plugin_help */
   p->plugin_print = dlsym(h, "plugin_print");

   /* plugin_gui_cleanup */
   p->plugin_gui_cleanup = dlsym(h, "plugin_gui_cleanup");

   /* plugin_startup */
   p->plugin_startup = dlsym(h, "plugin_startup");

   /* plugin_pre_sync */
   p->plugin_pre_sync = dlsym(h, "plugin_pre_sync");

   /* plugin_sync */
   p->plugin_sync = dlsym(h, "plugin_sync");

   /* plugin_post_sync */
   p->plugin_post_sync = dlsym(h, "plugin_post_sync");

   /* plugin_search */
   p->plugin_search = dlsym(h, "plugin_search");

   /* plugin_exit_cleanup */
   p->plugin_exit_cleanup = dlsym(h, "plugin_exit_cleanup");

   return 0;
}

/* This will always return the first plugin list entry */
GList *get_plugin_list()
{
   return plugins;
}

void free_plugin_list(GList **plugin_list)
{
   GList *temp_list;
   struct plugin_s *p;
   
   /* Go to first entry in the list */
   for (temp_list = *plugin_list; temp_list; temp_list = temp_list->prev) {
      *plugin_list = temp_list;
   }
   for (temp_list = *plugin_list; temp_list; temp_list = temp_list->next) {
      if (temp_list->data) {
	 p=temp_list->data;
	 if (p->full_path) {
	    free(p->full_path);
	 }
	 if (p->name) {
	    free(p->name);
	 }
	 if (p->db_name) {
	    free(p->db_name);
	 }
	 free(p);
      }
   }
   g_list_free(*plugin_list);
   *plugin_list=NULL;
}

void free_search_result(struct search_result **sr)
{
   struct search_result *temp_sr, *temp_sr_next;

   for (temp_sr = *sr; temp_sr; temp_sr=temp_sr_next) {
      if (temp_sr->line) {
	 free(temp_sr->line);
      }
      temp_sr_next = temp_sr->next;
      free(temp_sr);
   }
   *sr = NULL;
}


/* Jason Day contributed code - Start */
/*
 * WARNING
 * Caller must ensure that which is not out of range!
 */
int jp_get_pref (prefType prefs[], int which, long *n, const char **ret)
{
    if (which < 0) {
        return -1;
    }
    *n = prefs[which].ivalue;
    if (prefs[which].usertype == CHARTYPE) {
        if (ret != NULL) {
            *ret = prefs[which].svalue;
        }
    }
    else {
        if (ret !=NULL) {
            *ret = NULL;
        }
    }
    return 0;
}

/*
 * WARNING
 * Caller must ensure that which is not out of range!
 */
int jp_set_pref (prefType prefs[], int which, long n, const char *string)
{
    if (which < 0) {
        return -1;
    }
    prefs[which].ivalue = n;
    if (string == NULL) {
        prefs[which].svalue[0] = '\0';
        return 0;
    }
    if (prefs[which].filetype == CHARTYPE) {
        strncpy (prefs[which].svalue, string, MAX_PREF_VALUE);
        prefs[which].svalue[MAX_PREF_VALUE - 1] = '\0';
    }
    return 0;
}

/*
 * WARNING
 * Caller must ensure that which is not out of range!
 */
int jp_set_pref_int (prefType prefs[], int which, long n)
{
    if (which < 0) {
        return -1;
    }
    prefs[which].ivalue = n;
    /*
    if (prefs[which]->usertype == CHARTYPE) {
        get_pref_possibility(which, glob_prefs[which].ivalue, glob_prefs[which].svalue);
    }
    */
    return 0;
}

/*
 * WARNING
 * Caller must ensure that which is not out of range!
 */
int jp_set_pref_char (prefType prefs[], int which, char *string)
{
    if (which < 0) {
        return -1;
    }
    if (string == NULL) {
        prefs[which].svalue[0] = '\0';
        return 0;
    }
    if (prefs[which].filetype == CHARTYPE) {
        strncpy (prefs[which].svalue, string, MAX_PREF_VALUE);
        prefs[which].svalue[MAX_PREF_VALUE - 1] = '\0';
    }
    return 0;
}


int jp_read_rc_file (char *filename, prefType prefs[], int num_prefs)
{
    int i;
    FILE *in;
    char line[256];
    char *field1, *field2;
    char *pc;

    in = jp_open_home_file (filename, "r");
    if (!in) {
        return -1;
    }

    while (!feof (in)) {
        fgets (line, 255, in);
        line[254] = ' ';
        line[255] = '\0';
        field1 = strtok (line, " ");
        field2 = (field1 != NULL) ? strtok (NULL, "\n") : NULL;/* jonh */
        if ((field1 == NULL) || (field2 == NULL)) {
            continue;
        }
        if ((pc = (char *)index (field2, '\n'))) {
            pc[0] = '\0';
        }
        for (i = 0; i < num_prefs; i++) {
            if (!strcmp (prefs[i].name, field1)) {
                if (prefs[i].filetype == INTTYPE) {
                    prefs[i].ivalue = atoi (field2);
                }
                if (prefs[i].filetype == CHARTYPE) {
                    strncpy (prefs[i].svalue, field2, MAX_PREF_VALUE);
                    prefs[i].svalue[MAX_PREF_VALUE - 1] = '\0';
                }
            }
        }
    }
    fclose (in);

    return 0;
}

int jp_write_rc_file (char *filename, prefType prefs[], int num_prefs)
{
    int i;
    FILE *out;

    out = jp_open_home_file (filename, "w" );
    if (!out) {
        return -1;
    }

    for (i = 0; i < num_prefs; i++) {

        if (prefs[i].filetype == INTTYPE) {
            fprintf (out, "%s %ld\n", prefs[i].name, prefs[i].ivalue);
        }

        if (prefs[i].filetype == CHARTYPE) {
            fprintf (out, "%s %s\n", prefs[i].name, prefs[i].svalue);
        }
    }
    fclose (out);

    return 0;
}
/* Jason Day contributed code - End */

#endif  /* ENABLE_PLUGINS */
