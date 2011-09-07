/*
 * Emulator
 *
 * Copyright (C) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * DoHyung Hong <don.hong@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * JinKyu Kim <fredrick.kim@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YuYeon Oh <yuyeon.oh@samsung.com>
 * WooJin Jung <woojin2.jung@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "gpsnew.h"

GtkWidget *scroll_window;
GtkTextBuffer *text_buffer;
gboolean send_to_device = FALSE;
int netport = 7900;

GtkWidget *window;
GtkWidget *manual;
GtkWidget *latadj;
GtkWidget *longadj;
GtkWidget *altadj;
GtkWidget *veladj;
GtkWidget *browser;
GtkWidget *filename;
GtkWidget *info;
GtkWidget *startstop;

GtkWidget *longitude;
GtkWidget *latitude;
GtkWidget *altitude;
GtkWidget *velocity;

GtkWidget *longname;
GtkWidget *latname;
GtkWidget *altname;
GtkWidget *velname;
GtkWidget *destination;

FILE *input = NULL;
int output = 0;
char buffer[MAX_LEN];

char gpsdata[MAX_LEN];
float longval;
float latval;
float altval;
float velval;

/*
const NMEA_PROTOCOL_DATA[][MAX_LEN] = {
						"$GPGGA,%9.3f,%9.3f,%s,%9.3f,%s,1,07,1.0,9.0,M,,,,0000*0A",
						"GPGLL,%9.4f,%s,%9.4f,%s,%9.3f,A*2C"
						"$GPVTG,%4.1f,T,,M,%4.2f,N,%4.2f,K*6E",
									};
*/

struct  hostent  *ptrh;  /* pointer to a host table entry       */
struct  protoent *ptrp;  /* pointer to a protocol table entry   */
struct  sockaddr_in source; /* structure to hold server's address  */
struct  sockaddr_in dest; /* structure to hold client's address  */
int     client_socket;         /* socket descriptors                  */
int     port;            /* protocol port number                */
#ifdef _WIN32
int 	alen;
#else
socklen_t     alen;            /* length of address                   */
#endif
int     n;               /* number of characters received       */ 
int optval = 1;          /* options set by setsockopt           */


void exit_function()
{
    /* close device */

	if(output>0) {
#ifdef __MINGW32__
            if ( netport == UDP || netport == TCP ) {
                closesocket(output);
                WSACleanup();
            } else {
                close(output);
            }
#else
	    close(output);
#endif
        }
    if( client_socket > 0 )
    {
#ifdef __MINGW32__
                closesocket(output);
                close(output);
#else
	    close(output);
#endif
    }
    /* close log file */
	if(input!=NULL)
	    fclose(input);
}

static void expander_callback (GObject    *object,
							   GParamSpec *param_spec,
							   gpointer    user_data)
{
	GtkExpander *expander = GTK_EXPANDER (object);
	if(gtk_expander_get_expanded (expander))
	{
		gtk_widget_show(scroll_window);
	}
	else
	{
		gtk_widget_hide(scroll_window);
	}
}
GtkWidget * make_progress_log_expander()
{

	GtkWidget *progress_log_table = gtk_table_new( 2, 1, FALSE );
    gtk_table_set_row_spacings( GTK_TABLE( progress_log_table ), 10 );

	GtkWidget *expander = gtk_expander_new_with_mnemonic (PROGRESS_LOG_TEXT);

	g_signal_connect (expander, "notify::expanded", G_CALLBACK (expander_callback), NULL);

	gtk_table_attach( GTK_TABLE( progress_log_table ), expander, 0, 1, 0, 1,
                      GTK_FILL , GTK_FILL, 0, 0 );

	text_buffer = gtk_text_buffer_new(NULL); 
	GtkWidget *text_view = gtk_text_view_new_with_buffer(text_buffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW(text_view),FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(text_view),FALSE);

	scroll_window = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll_window), text_view);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window),GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	
	gtk_table_attach( GTK_TABLE( progress_log_table ), scroll_window, 0, 1, 1, 2, 
								GTK_EXPAND|GTK_FILL , GTK_EXPAND|GTK_FILL, 0, 0 );

	gtk_widget_show(expander);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
	gtk_widget_show(text_view);
	return progress_log_table;
}

void output_function(gchar *ptr)
{
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(text_buffer, &iter);
	gtk_text_buffer_insert(text_buffer,&iter, ptr, -1 );
}

void on_veladj_value_changed(GtkAdjustment  *adjust, gpointer  data)
{
	
}
void on_altadj_value_changed(GtkAdjustment  *adjust, gpointer  data){}
void on_longadj_value_changed(GtkAdjustment  *adjust, gpointer  data){}
void on_latadj_value_changed(GtkAdjustment  *adjust, gpointer  data){}

#ifdef _WIN32
/*
 *  * Sets a socket to non-blocking operation.
 *   */
static int set_nonblock (SOCKET sock)
{
    unsigned long flags = 1;
    return (ioctlsocket(sock, FIONBIO, &flags) != SOCKET_ERROR);
}
#endif

gboolean handle_connection(int mode)
{
	if(input==NULL)
	{
		output_function("\r\nSelect a log file\r\n");
		return FALSE;
	}
	if(mode==OPEN_CONNECTION)
	{
		gchar *devname= (gchar *)gtk_entry_get_text(GTK_ENTRY(destination));
		if(devname==NULL)
		{
			output_function("\r\nPlease choose target\r\n");
			return FALSE;
		}
		if(strstr(devname, "tcp:")!=NULL)
		{
			if(output<=0)
			{
#ifdef __MINGW32__
                WSADATA wsadata;
                if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
                    printf("Error creating socket.");
                    return -1;
                }
#endif
				netport = atoi(devname+4); // 4 pointer offset

				if(netport<=0)
					return FALSE;
				if ((output=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))!=-1)
				{
#ifndef _WIN32
                    // int flags = fcntl(output, F_GETFL, 0);
#else
                    unsigned long flags = 1;
                    ioctlsocket (output, FIONBIO, &flags);
#endif

#ifndef _WIN32
                    if(fcntl(output, F_SETFL, O_NONBLOCK) == -1)
#else
                    if(!set_nonblock(output))
#endif
					{
						printf("errno = %d\r\n", errno);
   		     	        output_function("\r\nNon Blocking Failed\r\n");
						close(output);
						return FALSE;
					}
					// int on = 0;
					//if(setsockopt(output, SOL_SOCKET, O_NONBLOCK, &on, sizeof(int)) < 0)
					setsockopt(output, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));
					memset((char *)&source, 0, sizeof(source)); /* clear sockaddr structure */
					memset((char *)&dest, 0, sizeof(dest)); /* clear sockaddr structure */
					source.sin_family = AF_INET;             /* set family to Internet     */
					source.sin_port = htons(netport);
					source.sin_addr.s_addr = inet_addr("127.0.0.1");     /* set the local IP address   */
					/* Bind a local address to the socket */
   		     		if (bind(output, (struct sockaddr *)&source, sizeof(source)) < 0) {
   		     	        output_function("\r\nBind failed\n");
   		     	        return FALSE;
   		     		}
					listen(output,5);
				}
				output_function("\r\nListening for connections\r\n");
				netport = TCP;
				return TRUE;
			}
			else
			{
				output_function("Unable to create socket\r\n");
				return FALSE;
			}
		}
		else if(strstr(devname, "udp:")!=NULL)
		{
#ifdef __MINGW32__
            WSADATA wsadata;
            if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
                printf("Error creating socket.");
                return -1;
            }
#endif
			netport = atoi(devname+4); // 4 pointer offset
			if(netport<=0)
				return FALSE;
			if ((output=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))!=-1)
			{
				memset((char *)&source, 0, sizeof(source)); /* clear sockaddr structure */
				memset((char *)&dest, 0, sizeof(dest)); /* clear sockaddr structure */
				source.sin_family = AF_INET;
				source.sin_addr.s_addr = inet_addr("127.0.0.1");
				source.sin_port = htons(7899);
    			dest.sin_family = AF_INET; /* set family to Internet     */
				dest.sin_port = htons(netport);
    			dest.sin_addr.s_addr = inet_addr("127.0.0.1"); /* set the local IP address   */		
				/* Bind a local address to the socket */
        		if (bind(output, (struct sockaddr *)&source, sizeof(source)) < 0) {
        	        output_function("\r\nBind failed\n");
        	        return FALSE;
        		}
        	    output_function("\r\nUDP Sock Created\r\n");
				send_to_device = TRUE;
				netport = UDP;
				return TRUE;
			}
			else
			{
				output_function("Unable to create socket\r\n");
				return FALSE;
			}
		}
		else
		{
#ifndef _WIN32
			memset(buffer, '\0', MAX_LEN);
			sprintf(buffer, "%s", SLP_GPS_DEVICE);
			printf("buffer=%s\r\n", buffer);
			if(mkfifo(buffer, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)!=0)
			{
				printf("Cannot create %s - %d\r\n", buffer, errno);
				if(errno!=EEXIST)
					return FALSE;
			}
			output_function("\r\nPipe Created\r\n");
				
			output= open(buffer, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXG);
			if(output<=0)
			{
				output_function("\r\nCannot open device file\r\n");
				return FALSE;
			}
			send_to_device = TRUE;
			netport = PIPE;
			return TRUE;
#else
			output_function("Cannot use named pipe in windows\r\n");
			return FALSE;
#endif
#if 0 // TTY implementation
			output_function("\r\nOpening device \r\n");
			output=open(devname,O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXG);
			if(output<=0)
			{
				output_function("\r\nCannot open device file\r\n");
				return FALSE;
			}
			send_to_device = TRUE;
			netport = TTY;
			return TRUE;
#endif
		}
	}
	else
	{
		if(netport==TCP)
		{
            if ( client_socket > 0 )
            {
#ifdef __MINGW32__
			    closesocket(client_socket);
                WSACleanup();
#else
			    close(client_socket);
#endif
			    client_socket = 0;
            }

            if ( output > 0 )
            {
#ifdef __MINGW32__
			closesocket(output);
            WSACleanup();
#else
			close(output);
#endif
            }
		}
		else if(netport==UDP)
        {
            if ( output > 0 )
            {
#ifdef __MINGW32__
			closesocket(output);
            WSACleanup();
#else
			close(output);
#endif
            }
        }
		else if(netport==PIPE)
		{
			if ( output > 0 ) 
                close(output);
			unlink(SLP_GPS_DEVICE);
		}

		netport = NONE;
		output_function("\r\nDevice Connection Closed\r\n");
		return TRUE;
	}
}


void on_startstop_toggled(GtkWidget *widget, gpointer data)
{
	if(GTK_TOGGLE_BUTTON (widget)->active)
	{
		if(!handle_connection(OPEN_CONNECTION))
		{
			output_function("\r\nConnection Open Failed\r\n");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), FALSE);
			return;
		}
		gtk_button_set_label(GTK_BUTTON(startstop), "Disconnect" );
		gtk_button_set_relief(GTK_BUTTON(startstop),GTK_RELIEF_NONE);
	}
	else
	{
		send_to_device = FALSE;
		gtk_button_set_label(GTK_BUTTON(startstop), "Connect" );
		gtk_button_set_relief(GTK_BUTTON(startstop),GTK_RELIEF_NORMAL);
		handle_connection(CLOSE_CONNECTION);
	}
}

/*void on_manual_toggled(GtkWidget *widget, gpointer data)
{
	on_startstop_toggled(startstop, NULL);
	if(GTK_TOGGLE_BUTTON (widget)->active)
	{
		gtk_widget_hide(info);
		gtk_widget_hide(filename);
		gtk_widget_hide(browser);

		gtk_widget_show(latitude);
		gtk_widget_show(longitude);
		gtk_widget_show(altitude);
		gtk_widget_show(velocity);

		gtk_widget_show(longname);
		gtk_widget_show(latname);
		gtk_widget_show(altname);
		gtk_widget_show(velname);
	}
	else
	{
		gtk_widget_show(info);
		gtk_widget_show(filename);
		gtk_widget_show(browser);

		gtk_widget_hide(latitude);
		gtk_widget_hide(longitude);
		gtk_widget_hide(altitude);
		gtk_widget_hide(velocity);

		gtk_widget_hide(longname);
		gtk_widget_hide(latname);
		gtk_widget_hide(altname);
		gtk_widget_hide(velname);

	}
}*/


void on_browser_clicked(void)
{
	gchar *logfile=NULL;
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Select File",
				      GTK_WINDOW(window),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);
	char full_gps_data_path[MAX_LEN];
	sprintf(full_gps_data_path, "%s/data", get_bin_path());
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), full_gps_data_path);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  	{
    	logfile = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		if(logfile!=NULL)
		{
			input=fopen(logfile, "r");
			if(input==NULL)
			{
				output_function("\r\nCannot open input file\r\n");
				return;
			}
			output_function(logfile);
			gtk_label_set_text(GTK_LABEL(filename), logfile);
    		g_free (logfile);
		}
  	}
	else
	{
		if(logfile!=NULL)
		{
    		g_free (logfile);
			logfile=NULL;
		}
	}
	gtk_widget_destroy (dialog);
}

gboolean send_gps_data(gpointer data)
{
	switch(netport)
	{
		case TCP:
				if(input!=NULL && output>0)
				{
					if(client_socket==0)
					{
						if ((client_socket = accept(output, (struct sockaddr *)&dest, &alen)) > 0) 
						{
							output_function("\r\nClient Connected\r\n");
							send_to_device=TRUE;
							return TRUE;
						}
						else
						{
							return TRUE;
						}
					}
				}
				break;
		case UDP:
		case TTY:
		case PIPE:
				if(!(send_to_device && input!=NULL && output>0))
				{
					return TRUE;
				}
				break;
		default:
				return TRUE;
	}

    /* send data to device */
	memset(buffer, '\0', MAX_LEN);
	if(fgets(buffer,MAX_LEN, input)==NULL)
		rewind(input);
	else
	{
		int count = 0;
		if(buffer[0] != '#')
		{
			output_function(buffer);
			if(netport == UDP)
			{
				count = sendto(output, buffer, MAX_LEN, 0, \
									(struct sockaddr *)&dest, sizeof(dest));
				/*int temp = 0;
				count = recvfrom(output, buffer, MAX_LEN, 0, \
									(struct sockaddr *)&source, &temp);*/
			}
			else if (netport==TCP)
				count = send(client_socket, buffer, MAX_LEN, 0);
			else if(netport == TTY) 
				count = write(output, buffer, MAX_LEN);
			else if(netport == PIPE) 
				count = write(output, buffer, MAX_LEN);
		}
		//sprintf(buffer, "\r\nWriting %d Bytes\r\n", strlen(buffer));
		//output_function(buffer);
	}
	return TRUE;
}

void menu_create_gps(GtkWidget* parent)
{
    if(window==NULL)
    {
	GtkBuilder *builder = gtk_builder_new();
	char full_glade_path[MAX_LEN];
	sprintf(full_glade_path, "%s/gps.glade", get_bin_path());

	gtk_builder_add_from_file(builder, full_glade_path, NULL);

	//get objects from the UI
	window = (GtkWidget *)gtk_builder_get_object(builder, "window1");
	GtkWidget *layout = (GtkWidget *)gtk_builder_get_object(builder, "layout1");
	/*manual = (GtkWidget *)gtk_builder_get_object(builder, "manual");
	latadj = (GtkWidget *)gtk_builder_get_object(builder, "latadj");
	longadj = (GtkWidget *)gtk_builder_get_object(builder, "longadj");
	altadj = (GtkWidget *)gtk_builder_get_object(builder, "altadj");
	veladj = (GtkWidget *)gtk_builder_get_object(builder, "veladj"); */
	browser = (GtkWidget *)gtk_builder_get_object(builder, "browser");
	filename = (GtkWidget *)gtk_builder_get_object(builder, "filename");
	info = (GtkWidget *)gtk_builder_get_object(builder, "info");
	startstop = (GtkWidget *)gtk_builder_get_object(builder, "startstop");

	/*longitude = (GtkWidget *)gtk_builder_get_object(builder, "longitude");
	latitude = (GtkWidget *)gtk_builder_get_object(builder, "latitude");
	altitude = (GtkWidget *)gtk_builder_get_object(builder, "altitude");
	velocity = (GtkWidget *)gtk_builder_get_object(builder, "velocity");

	longname = (GtkWidget *)gtk_builder_get_object(builder, "longname");
	latname = (GtkWidget *)gtk_builder_get_object(builder, "latname");
	altname = (GtkWidget *)gtk_builder_get_object(builder, "altname");
	velname = (GtkWidget *)gtk_builder_get_object(builder, "velname"); */

	destination = (GtkWidget *)gtk_builder_get_object(builder, "destination");

	/*gtk_widget_hide(latitude);
	gtk_widget_hide(longitude);
	gtk_widget_hide(altitude);
	gtk_widget_hide(velocity);

	gtk_widget_hide(longname);
	gtk_widget_hide(latname);
	gtk_widget_hide(altname);
	gtk_widget_hide(velname);*/

	gtk_builder_connect_signals(builder, NULL);
	send_to_device = FALSE;

	//add the progress log expander
	GtkWidget *progress_log_table=make_progress_log_expander();
	gtk_widget_show(progress_log_table);
	gtk_widget_set_size_request(progress_log_table, 480, 200);
	gtk_layout_put(GTK_LAYOUT(layout), progress_log_table, 30, 315);
	gtk_widget_show(window);
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(manual), FALSE );
	g_signal_connect (GTK_OBJECT(window), "delete_event", G_CALLBACK(exit_function), window);
	g_timeout_add(1000, &send_gps_data, NULL );
    }
    else
    {
	gtk_widget_show(window);
    }

}
