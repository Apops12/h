/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>       

#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#else
#ifdef HAVE_NETINET_IN_SYSTEM_H
#include <netinet/in_system.h>
#endif
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include <stdarg.h>

#include <netinet/tcp.h>

#ifdef SOLARIS
#include <pthread.h>
#include <thread.h>
#else
#include <pthread.h>
#endif

#include <gtk/gtk.h>

#include "gtk-gui.h"
#include "gtk-interface.h"
#include "gtk-support.h"

void
gtk_gui (void *args)
{
    int tmp;
    struct term_node *term_node = NULL;
    time_t this_time;
    sigset_t mask;
    struct gtk_s_helper helper;
    struct interface_data *iface_data, *iface;
    GtkWidget *Main;

    pthread_mutex_lock(&terms->gui_gtk_th.finished);

    terms->work_state = RUNNING;

    write_log(0,"\n gtk_gui_th = %X\n",(int)pthread_self());
    iface_data = NULL;

    sigfillset(&mask);

    if (pthread_sigmask(SIG_BLOCK, &mask, NULL))
    {
        thread_error("gtk_gui_th pthread_sigmask()",errno);
        gtk_gui_th_exit(NULL);
    }

    if (pthread_mutex_lock(&terms->mutex) != 0) 
    {
        thread_error("gtk_gui_th pthread_mutex_lock",errno);
        gtk_gui_th_exit(NULL);
    }

    if (term_add_node(&term_node, TERM_CON, 0, pthread_self()) < 0)
    {
        if (pthread_mutex_unlock(&terms->mutex) != 0)
            thread_error("gtk_gui_th pthread_mutex_unlock",errno);
        gtk_gui_th_exit(NULL);
    }

    if (term_node == NULL)
    {
        write_log(0, "Ouch!! No more than %d %s accepted!!\n", term_type[TERM_CON].max, term_type[TERM_CON].name);
        if (pthread_mutex_unlock(&terms->mutex) != 0)
            thread_error("gtk_gui_th pthread_mutex_unlock",errno);
        gtk_gui_th_exit(NULL);
    }

    this_time = time(NULL);

#ifdef HAVE_CTIME_R
#ifdef SOLARIS
    ctime_r(&this_time,term_node->since, sizeof(term_node->since));
#else
    ctime_r(&this_time,term_node->since);
#endif
#else
    pthread_mutex_lock(&mutex_ctime);
    strncpy(term_node->since, ctime(&this_time), sizeof(term_node->since) - 1 );
    pthread_mutex_unlock(&mutex_ctime);
#endif

    /* Just to remove the cr+lf...*/
    term_node->since[sizeof(term_node->since)-2] = 0;

    /* This is a console so, man... ;) */
    strncpy(term_node->from_ip, "127.0.0.1", sizeof(term_node->from_ip) - 1 );

    /* Parse config file */
    if (strlen(tty_tmp->config_file))
    {
        if (parser_read_config_file(tty_tmp, term_node) < 0)
        {
            write_log(0, "Error reading configuration file\n");
            gtk_gui_th_exit(term_node);
        }
    }

    if (pthread_mutex_unlock(&terms->mutex) != 0) {
      thread_error("gtk_gui_th pthread_mutex_unlock",errno);
      gtk_gui_th_exit(term_node);
    }

#ifdef ENABLE_NLS
    bindtextdomain( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
    bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
    textdomain( GETTEXT_PACKAGE );
#endif

    gtk_set_locale();
    gtk_init( NULL, NULL );

    add_pixmap_directory( PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps" );

    if ( interfaces && interfaces->list )
        iface_data = dlist_data( interfaces->list );
    else
    {
        gtk_i_modaldialog( GTK_MESSAGE_ERROR, "Interface not found", "Hmm... you don't have any valid interface. %s is useless. Go and get a life!", PACKAGE);
        gtk_gui_th_exit( term_node );
    }

    /* take the first valid interface */
    if ( strlen( iface_data->ifname ) )
    {
        if ( ( tmp = interfaces_enable( iface_data->ifname ) ) == -1 )
        {
            gtk_i_modaldialog( GTK_MESSAGE_WARNING, "Invalid interface", "Unable to use interface %s!! (Maybe nonexistent?)\n\n", iface_data->ifname );
        }
        else
        {
            iface = (struct interface_data *) calloc(1, sizeof( struct interface_data ) );
            memcpy( (void *)iface, (void *)iface_data, sizeof( struct interface_data ) );
            term_node->used_ints->list = dlist_append( term_node->used_ints->list, iface );
        }
    }
    else
    {
        gtk_i_modaldialog( GTK_MESSAGE_ERROR, "Invalid interfaces", "Hmm... you don't have any valid interface. %s is useless. Go and get a life!", PACKAGE );
        gtk_gui_th_exit( term_node );
    }

    helper.node = (struct term_node *)term_node;
    helper.edit_mode = 0;
    Main = gtk_i_create_Main( &helper );

    gtk_i_modaldialog( GTK_MESSAGE_WARNING, "Alpha version!", "%s", "This is an alpha version of the GTK GUI. Not all the options are implemented but if you're brave enough, you're allowed to test it and tell us all the bugs you could find");

    gtk_widget_show( Main );

    gtk_i_modaldialog( GTK_MESSAGE_WARNING, "Default interface", "Network interface %s selected as the default one", iface_data->ifname );

    gtk_main();

    write_log( 0, "Exiting GTK mode...\n" );

    gtk_gui_th_exit( term_node );
}


/* 
 * GUI destroy. End
 */
void
gtk_gui_th_exit(struct term_node *term_node)
{
   dlist_t *p;
   struct interface_data *iface_data;

   write_log(0, "\n gtk_gui_th_exit start...\n");

   if (term_node)
   {
      for (p = term_node->used_ints->list; p; p = dlist_next(term_node->used_ints->list, p)) {
         iface_data = (struct interface_data *) dlist_data(p);
         interfaces_disable(iface_data->ifname);
      }

      attack_kill_th(term_node,ALL_ATTACK_THREADS);

      if (pthread_mutex_lock(&terms->mutex) != 0)
         thread_error("gtk_gui_th pthread_mutex_lock",errno);

      term_delete_node(term_node, NOKILL_THREAD);               

      if (pthread_mutex_unlock(&terms->mutex) != 0)
         thread_error("gtk_gui_th pthread_mutex_unlock",errno);
   }

   write_log(0," gtk_gui_th_exit finish...\n");   

   terms->gui_gtk_th.id = 0;

   if (pthread_mutex_unlock(&terms->gui_gtk_th.finished) != 0)
      thread_error("gtk_gui_th pthread_mutex_unlock",errno);
   
   terms->work_state = STOPPED;
   
   pthread_exit(NULL);
}
