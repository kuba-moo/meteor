
/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
 *
 * See the file 'LICENSE' for licensing information.
 *
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: io.c
 * Function: I/O functionality for deltaQ library
 *
 * Author: Razvan Beuran
 *
 * $Id: io.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "io.h"
#include "global.h"
#include "message.h"


#define GRID_SIZE_X            300.0
#define GRID_SIZE_Y            300.0
#define NODE_RADIUS            5.0


////////////////////////////////////////////////
// Text I/O functions
////////////////////////////////////////////////

// write the header of the file in which connection description will be stored
void
io_write_header_to_file (FILE * file_global, char *qomet_name)
{
  fprintf (file_global, "%% Output generated by %s\n", qomet_name);
  fprintf (file_global, "%% time from_id from_node_x from_node_y \
from_node_z to_id to_node_x to_node_y to_node_z distance Pr SNR FER \
num_retr op_rate bandwidth loss_rate delay jitter\n");
}

// write connection description to file
void
io_write_to_file (struct connection_class *connection,
		  struct scenario_class *scenario, double time,
		  int cartesian_coord_syst, FILE * file_global)
{
  struct node_class *from_node, *to_node;

  struct coordinate_class saved_from, saved_to;
  struct coordinate_class point_blh;

  from_node = &(scenario->nodes[connection->from_node_index]);
  to_node = &(scenario->nodes[connection->to_node_index]);

  // if coordinate system is not cartesian, we convert
  // x & y to latitude & longitude before storing
  if (cartesian_coord_syst == FALSE)
    {
      coordinate_copy (&saved_from, &(from_node->position));
      coordinate_copy (&saved_to, &(to_node->position));

      // transform from_node x & y to lat & long
      en2ll (&(from_node->position), &point_blh);
      coordinate_copy (&(from_node->position), &point_blh);

      // transform to_node x & y to lat & long
      en2ll (&(to_node->position), &point_blh);
      coordinate_copy (&(to_node->position), &point_blh);
    }

  /*
     // for nodes with only one interface, use "classical" output
     if ( (from_node->interface_number == 1) &&
     (to_node->interface_number == 1) )
     {
     // write current connection description to file
     fprintf (file_global, "%.2f %d %.6f %.6f %.6f %d %.6f %.6f %.6f %.4f \
     %.4f %.4f %.4f %.4f %.2f %.2f %.4f %.4f %.4f\n", time, from_node->id, 
     from_node->position.c[0], from_node->position.c[1], 
     from_node->position.c[2], to_node->id, to_node->position.c[0], 
     to_node->position.c[1], to_node->position.c[2], 
     connection->distance, connection->Pr, connection->SNR, 
     connection->frame_error_rate, connection->num_retransmissions, 
     connection_get_operating_rate (connection), 
     connection->bandwidth, connection->loss_rate, 
     connection->delay, connection->jitter);
     }
     else // for nodes with multiples interfaces, use "extended" output
     // using node_id.interface_id
     {
     // write current connection description to file
     fprintf (file_global, "%.2f %d.%d %.6f %.6f %.6f %d.%d %.6f %.6f %.6f %.4f \
     %.4f %.4f %.4f %.4f %.2f %.2f %.4f %.4f %.4f\n", time, from_node->id, 
     connection->from_interface_index,
     from_node->position.c[0], from_node->position.c[1], 
     from_node->position.c[2], to_node->id, 
     connection->to_interface_index, to_node->position.c[0], 
     to_node->position.c[1], to_node->position.c[2], 
     connection->distance, connection->Pr, connection->SNR, 
     connection->frame_error_rate, connection->num_retransmissions, 
     connection_get_operating_rate (connection), 
     connection->bandwidth, connection->loss_rate, 
     connection->delay, connection->jitter);
     }
   */
  // workaround to avoind printing "-0.000000"; use always "0.000000" instead
  double from_c0, from_c1, from_c2, to_c0, to_c1, to_c2;

  if ((from_node->position.c[0] < 0) && (from_node->position.c[0] > -1e-6))
    from_c0 = -from_node->position.c[0];
  else
    from_c0 = from_node->position.c[0];
  if ((from_node->position.c[1] < 0) && (from_node->position.c[1] > -1e-6))
    from_c1 = -from_node->position.c[1];
  else
    from_c1 = from_node->position.c[1];
  if ((from_node->position.c[2] < 0) && (from_node->position.c[2] > -1e-6))
    from_c2 = -from_node->position.c[2];
  else
    from_c2 = from_node->position.c[2];

  if ((to_node->position.c[0] < 0) && (to_node->position.c[0] > -1e-6))
    to_c0 = -to_node->position.c[0];
  else
    to_c0 = to_node->position.c[0];
  if ((to_node->position.c[1] < 0) && (to_node->position.c[1] > -1e-6))
    to_c1 = -to_node->position.c[1];
  else
    to_c1 = to_node->position.c[1];
  if ((to_node->position.c[2] < 0) && (to_node->position.c[2] > -1e-6))
    to_c2 = -to_node->position.c[2];
  else
    to_c2 = to_node->position.c[2];

  // write current connection description to file using 
  // from_id and to_id from connection
  fprintf (file_global, "%.2f %d %.6f %.6f %.6f %d %.6f %.6f %.6f %.4f \
%.4f %.4f %.4f %.4f %.2f %.2f %.4f %.4f %.4f\n", time, connection->from_id, from_c0, from_c1, from_c2, connection->to_id, to_c0, to_c1, to_c2, connection->distance, connection->Pr, connection->SNR, connection->frame_error_rate, connection->num_retransmissions, connection_get_operating_rate (connection), connection->bandwidth, connection->loss_rate, connection->delay, connection->jitter);

  // restore coordinates
  if (cartesian_coord_syst == FALSE)
    {
      coordinate_copy (&(from_node->position), &saved_from);
      coordinate_copy (&(to_node->position), &saved_to);
    }
}

// write header of motion file in NAM format;
// return SUCCESS on succes, ERROR on error
int
io_write_nam_motion_header_to_file (struct scenario_class *scenario,
				    FILE * motion_file)
{
  int node_i;
  struct node_class *node;

  //double saved_node_x, saved_node_y;

  // write comment
  fprintf (motion_file, "# NAM trace movement file generated by QOMET\n");

  // write version of nam
  fprintf (motion_file, "%s\n", "V -t * -v 1.0a5 -a 0");

  // write scenario simulation size
  fprintf (motion_file, "W -t * -x %.2f -y %.2f\n", GRID_SIZE_X, GRID_SIZE_Y);

  // write node initial position and display parameters
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    {
      node = &(scenario->nodes[node_i]);

      // should convert to ll before output, but coordinates
      // would become too small!!!!!!!!!!!!!!11

      // if coordinate system is not cartesian, we convert
      // x & y to latitude & longitude before storing
      /*
         if(cartesian_coord_syst == FALSE)
         {
         }
       */

      // write data for current node
      fprintf (motion_file,
	       "n -t * -s %d -x %.9f -y %.9f -z %f -v circle -c black -w\n",
	       node->id, node->position.c[0], node->position.c[1],
	       NODE_RADIUS);

      /*
         // restore coordinates
         if(cartesian_coord_syst == FALSE)
         {
         }
       */
    }

  return SUCCESS;
}

// write motion file information in NAM format;
// return SUCCESS on succes, ERROR on error
int
io_write_nam_motion_info_to_file (struct scenario_class *scenario,
				  FILE * motion_file, float time)
{
  int node_i;
  struct node_class *node;

  // write current positions of nodes
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    {
      node = &(scenario->nodes[node_i]);
      fprintf (motion_file, "n -t %.2f -s %d -x %.9f -y %.9f\n", time,
	       node->id, node->position.c[0], node->position.c[1]);
    }

  return SUCCESS;
}

// write header of motion file in NS-2 format;
// return SUCCESS on succes, ERROR on error
int
io_write_ns2_motion_header_to_file (struct scenario_class *scenario,
				    FILE * motion_file)
{
  int node_i;
  struct node_class *node;

  //double saved_node_x, saved_node_y;

  // write comment
  fprintf (motion_file, "# NS-2 node movement file generated by QOMET\n");

  // write node initial position and display parameters
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    {
      node = &(scenario->nodes[node_i]);

      // should convert to ll before output, but coordinates
      // would become too small!!!!!!!!!!!!!!11

      // if coordinate system is not cartesian, we convert
      // x & y to latitude & longitude before storing
      /*
         if(cartesian_coord_syst == FALSE)
         {
         }
       */

      // write data for current node
      fprintf (motion_file,
	       "$node_(%d) set X_ %.9f\n$node_(%d) set Y_ %.9f\n$node_(%d) \
set Z_ %.9f\n", node->id, node->position.c[0], node->id, node->position.c[1], node->id, node->position.c[2]);

      /*
         // restore coordinates
         if(cartesian_coord_syst == FALSE)
         {
         }
       */
    }

  return SUCCESS;
}

// write motion file information in NS-2 format;
// return SUCCESS on succes, ERROR on error
int
io_write_ns2_motion_info_to_file (struct scenario_class *scenario,
				  FILE * motion_file, float time)
{
  int node_i;
  struct node_class *node;

  // write current positions of nodes
  for (node_i = 0; node_i < scenario->node_number; node_i++)
    {
      node = &(scenario->nodes[node_i]);
      fprintf (motion_file, "$ns_ at %.3f \"$node_(%d) setdest %.9f %.9f \
%.3f\"\n", time, node->id, node->position.c[0], node->position.c[1], DEFAULT_NS2_SPEED);
    }

  return SUCCESS;
}

// write objects to file;
// return SUCCESS on succes, ERROR on error
int
io_write_objects (struct scenario_class *scenario, int cartesian_coord_syst,
		  FILE * object_file)
{
  int object_i, vertex_i;
  struct object_class *object;
  struct coordinate_class point_blh;

  fprintf (object_file, "%% type height vertex_count vertices \
(as coordinate pairs) ...\n");

  for (object_i = 0; object_i < scenario->object_number; object_i++)
    {
      object = &(scenario->objects[object_i]);
      fprintf (object_file, "%d %.2f %d", object->type, object->height,
	       object->vertex_number);
      for (vertex_i = 0; vertex_i < object->vertex_number; vertex_i++)
	{
	  // cartesian coordinates are printed as such, latitude-longitude ones
	  // are printed after conversion back to such coordinates
	  // (internally all coordinates are store as cartesian)
	  if (cartesian_coord_syst == TRUE)
	    fprintf (object_file, " %.6f %.6f",
		     object->vertices[vertex_i].c[0],
		     object->vertices[vertex_i].c[1]);
	  else
	    {
	      en2ll (&(object->vertices[vertex_i]), &point_blh);
	      fprintf (object_file, " %.6f %.6f", point_blh.c[0],
		       point_blh.c[1]);
	    }
	}

      fprintf (object_file, "\n");
    }

  return SUCCESS;
}

// generate the settings file based on scenario properties
int
io_write_settings_file (struct scenario_class *scenario, FILE * settings_file)
{
  int node_i, interface_i;

  for (node_i = 0; node_i < scenario->node_number; node_i++)
    for (interface_i = 0;
	 interface_i < scenario->nodes[node_i].interface_number;
	 interface_i++)
      {
	// shortcut to interface structure
	struct interface_class *interface =
	  &((scenario->nodes[node_i]).interfaces[interface_i]);
	if (fprintf (settings_file, "%s %s %d %s\n",
		     (scenario->nodes[node_i]).name,
		     interface->name, interface->id,
		     interface->ip_address) < 0)
	  return ERROR;
      }

  return SUCCESS;
}

// read the scenario settings (ids and corresponding IP adresses)
// from a file, and store the adresses in the arrays p (binary) and 
// p_char (string) at the corresponding index;
// return the number of addresses (=interfaces) successfully read, 
// or ERROR on error
int
io_read_settings_file (char *settings_filename,
		       in_addr_t * p, char *p_char, int p_size)
{
  static char buf[BUFSIZ];
  int i = 0;
  int line_nr = 0;
  FILE *fd;

  int node_id;
  char node_ip[IP_ADDR_SIZE];

  // open settings file
  if ((fd = fopen (settings_filename, "r")) == NULL)
    {
      WARNING ("Cannot open settings file '%s' for reading",
	       settings_filename);
      return ERROR;
    }

  // parse file
  while (fgets (buf, BUFSIZ, fd) != NULL)
    {
      line_nr++;

      // check we didn't exceed maximum size
      if (i >= p_size)
	{
	  WARNING ("Maximum number of IP addresses (%d) exceeded", p_size);
	  fclose (fd);
	  return ERROR;
	}
      else
	{
	  int scaned_items;
	  char node_name[MAX_STRING];
	  char interface_name[MAX_STRING];

	  // parse each line for node id and IP (assume MAX_STRING is 256)
	  scaned_items =
	    sscanf (buf, "%" MAX_STRING_STR "s %" MAX_STRING_STR "s %d %16s",
		    node_name, interface_name, &node_id, node_ip);
	  if (scaned_items < 4)
	    {
	      WARNING ("Skipped invalid line #%d in settings file '%s'",
		       line_nr, settings_filename);
	      continue;
	    }
	  if (node_id < 0 || node_id >= p_size)
	    {
	      WARNING
		("Id %d is not within the permitted range [%d, %d]",
		 node_id, 0, p_size);
	      fclose (fd);
	      return ERROR;
	    }
	  if ((p[node_id] = inet_addr (node_ip)) != INADDR_NONE)
	    {
	      DEBUG ("Valid IP setting: id=%d ip(char)=%s", node_id, node_ip);
	      // TODO: replace with strncpy?!
	      snprintf (p_char + node_id * IP_ADDR_SIZE, IP_ADDR_SIZE,
			"%s", node_ip);
	      i++;
	    }
	}
    }

  fclose (fd);

  return i;
}

// read the MAC-level scenario settings (ids and corresponding MAC
// adresses) from a file, and store the addresses in the array
// 'mac_addresses' (binary form) at the corresponding index;
// 'array_size' represents the number of entries allocated in the
// 'mac_addresses' array;
// return the number of addresses (=interfaces) successfully read, 
// or ERROR on error
int
io_read_settings_file_mac (char *settings_filename,
			   in_addr_t * p, char *p_char,
			   unsigned char mac_addresses[][ETH_SIZE],
			   char mac_char_addresses[][MAC_ADDR_SIZE],
			   int array_size)
{
  static char buf[BUFSIZ];
  int i = 0;
  int line_nr = 0;
  FILE *fd;

  int node_id;
  char node_mac[MAC_ADDR_SIZE];
  char node_ip[IP_ADDR_SIZE];

  // open settings file
  if ((fd = fopen (settings_filename, "r")) == NULL)
    {
      WARNING ("Cannot open MAC settings file '%s' for reading",
	       settings_filename);
      return ERROR;
    }

  // parse file
  while (fgets (buf, BUFSIZ, fd) != NULL)
    {
      line_nr++;

      // check we didn't exceed maximum size
      if (i >= array_size)
	{
	  WARNING ("Exceeded the size of the MAC address array (%d)",
		   array_size);
	  fclose (fd);
	  return ERROR;
	}
      else
	{
	  int scaned_items;
	  char node_name[MAX_STRING];
	  char interface_name[MAX_STRING];
	  unsigned int mac_address[ETH_SIZE];

	  // parse each line for node name, interface name, node id and 
	  // MAC address (assume MAX_STRING is 256)
	  scaned_items =
	    sscanf (buf,
		    "%" MAX_STRING_STR "s %" MAX_STRING_STR "s %d %15s %17s",
		    node_name, interface_name, &node_id, node_ip, node_mac);
	  if (scaned_items < 4)
	    {
	      WARNING ("Skipped invalid line #%d in settings file '%s'",
		       line_nr, settings_filename);
	      continue;
	    }
	  if (node_id < 0 || node_id >= array_size)
	    {
	      WARNING
		("Id %d is not within the permitted range [%d, %d]",
		 node_id, 0, array_size);
	      fclose (fd);
	      return ERROR;
	    }

	  if ((p[node_id] = inet_addr (node_ip)) != INADDR_NONE)
	    {
	      DEBUG ("Valid IP setting: id=%d ip(char)=%s", node_id, node_ip);
	      snprintf (p_char + node_id * IP_ADDR_SIZE, IP_ADDR_SIZE,
			"%s", node_ip);
	    }
	  else
	    {
	      WARNING ("IP address '%s' is not correctly formatted", node_ip);
	      return ERROR;
	    }

	  if (sscanf (node_mac, "%x:%x:%x:%x:%x:%x", &(mac_address[0]),
		      &(mac_address[1]), &(mac_address[2]), &(mac_address[3]),
		      &(mac_address[4]), &(mac_address[5])) < 6)
	    {
	      WARNING ("MAC address '%s' is not correctly formatted",
		       node_mac);
	      fclose (fd);
	      return ERROR;
	    }
	  else
	    {
	      int j;

	      for (j = 0; j < ETH_SIZE; j++)
		if (mac_address[j] < 256)
		  {
		    //printf("mac_address[%d]=0x%02x\n", j, mac_address[j]);
		    mac_addresses[node_id][j] =
		      (unsigned char) mac_address[j];
		  }
		else
		  {
		    WARNING
		      ("MAC address '%s' is not correctly formatted (error at byte #%d='%d')",
		       node_mac, j, mac_address[j]);
		    fclose (fd);
		    return ERROR;
		  }

	      strncpy (mac_char_addresses[node_id], node_mac,
		       MAC_ADDR_SIZE - 1);

	      DEBUG
		("Valid MAC setting: id=%d MAC='%02X:%02X:%02X:%02X:%02X:%02X'",
		 node_id, mac_addresses[node_id][0],
		 mac_addresses[node_id][1], mac_addresses[node_id][2],
		 mac_addresses[node_id][3], mac_addresses[node_id][4],
		 mac_addresses[node_id][5]);
	      i++;
	    }
	}
    }

  fclose (fd);

  return i;
}


////////////////////////////////////////////////
// Binary I/O functions
////////////////////////////////////////////////

// print binary header
void
io_binary_print_header (struct binary_header_class *binary_header)
{
  // print signature (only first 3 characters)
  printf ("Header signature: %c%c%c\n",
	  binary_header->signature[0], binary_header->signature[1],
	  binary_header->signature[2]);
  printf ("Generated by QOMET v%d.%d.%d (revision %d)\n",
	  binary_header->major_version, binary_header->minor_version,
	  binary_header->subminor_version, binary_header->svn_revision);
  printf ("Number of interfaces in file: %d\n",
	  binary_header->interface_number);
  printf ("Number of time records in file: %d\n",
	  binary_header->time_record_number);
}

// print binary time record
void
io_binary_print_time_record (struct binary_time_record_class
			     *binary_time_record)
{
  printf ("- Time: %.2f s (%d records)\n", binary_time_record->time,
	  binary_time_record->record_number);
}

// print binary record
void
io_binary_print_record (struct binary_record_class *binary_record)
{
  /*
     printf ("-- Record: from_node=%d to_node=%d FER=%.4f num_retr=%.4f \
     op_rate=%.2f bandwidth=%.2f loss_rate=%.4f delay=%.4f\n", binary_record->from_node, 
     binary_record->to_node, binary_record->frame_error_rate, 
     binary_record->num_retransmissions, binary_record->operating_rate, 
     binary_record->bandwidth, binary_record->loss_rate, binary_record->delay);
   */
  printf ("-- Record: from_id=%d to_id=%d FER=%.4f num_retr=%.4f \
standard=%d op_rate=%.2f bandwidth=%.2f loss_rate=%.4f delay=%.4f\n", binary_record->from_id, binary_record->to_id, binary_record->frame_error_rate, binary_record->num_retransmissions, binary_record->standard, binary_record->operating_rate, binary_record->bandwidth, binary_record->loss_rate, binary_record->delay);
}

// copy binary record
void
io_binary_copy_record (struct binary_record_class *binary_record_dst,
		       struct binary_record_class *binary_record_src)
{
  binary_record_dst->from_id = binary_record_src->from_id;
  binary_record_dst->to_id = binary_record_src->to_id;
  binary_record_dst->frame_error_rate = binary_record_src->frame_error_rate;
  binary_record_dst->num_retransmissions =
    binary_record_src->num_retransmissions;
  binary_record_dst->standard = binary_record_src->standard;
  binary_record_dst->operating_rate = binary_record_src->operating_rate;
  binary_record_dst->bandwidth = binary_record_src->bandwidth;
  binary_record_dst->loss_rate = binary_record_src->loss_rate;
  binary_record_dst->delay = binary_record_src->delay;
}

// build binary record
void
io_binary_build_record (struct binary_record_class *binary_record,
			struct connection_class *connection,
			struct scenario_class *scenario)
{
  binary_record->from_id = connection->from_id;
  binary_record->to_id = connection->to_id;
  binary_record->frame_error_rate = connection->frame_error_rate;
  binary_record->num_retransmissions = connection->num_retransmissions;
  binary_record->standard = connection->standard;
  binary_record->operating_rate = connection_get_operating_rate (connection);
  binary_record->bandwidth = connection->bandwidth;
  binary_record->loss_rate = connection->loss_rate;
  binary_record->delay = connection->delay;
}

// compare with binary record;
// return TRUE if data is same with the one in the record,
// FALSE otherwise
int
io_binary_compare_record (struct binary_record_class *binary_record,
			  struct connection_class *connection,
			  struct scenario_class *scenario)
{
  // check from and to node ids
  if ((binary_record->from_id == connection->from_id)
      && (binary_record->to_id == connection->to_id)
      // check frame_error_rate
      && (fabs (binary_record->frame_error_rate
		- connection->frame_error_rate) < EPSILON)
      // check num_retr
      && (fabs (binary_record->num_retransmissions
		- connection->num_retransmissions) < EPSILON)
      && (binary_record->standard == connection->standard)
      // check op_rate
      && (fabs (binary_record->operating_rate
		- connection_get_operating_rate (connection)) < EPSILON)
      // check bandwidth (after conversion to Mbps)
      && (fabs (binary_record->bandwidth / 1e6
		- connection->bandwidth / 1e6) < EPSILON)
      // check loss rate
      && (fabs (binary_record->loss_rate - connection->loss_rate) < EPSILON)
      // check delay
      && (fabs (binary_record->delay - connection->delay) < EPSILON))
    // jitter field not defined yet
    //&& (fabs (binary_record->jitter - connection->jitter) < EPSILON))
    {
      return TRUE;
    }
  else
    {
#ifdef MESSAGE_DEBUG
      printf ("Compare is FALSE for from_id=%d to_id=%d\n",
	      binary_record->from_id, binary_record->to_id);
      printf
	("Differences: FER=%f num_retr=%f op_rate=%f bandwidth=%f loss_rate=%f delay=%f\n",
	 fabs (binary_record->frame_error_rate -
	       connection->frame_error_rate),
	 fabs (binary_record->num_retransmissions -
	       connection->num_retransmissions),
	 binary_record->operating_rate -
	 connection_get_operating_rate (connection),
	 fabs (binary_record->bandwidth / 1e6 - connection->bandwidth / 1e6),
	 fabs (binary_record->loss_rate - connection->loss_rate),
	 fabs (binary_record->delay - connection->delay));
#endif
      return FALSE;
    }

}

// read header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_binary_read_header_from_file (struct binary_header_class *binary_header,
				 FILE * binary_input_file)
{
  // read header from file
  if (fread (binary_header, sizeof (struct binary_header_class),
	     1, binary_input_file) != 1)
    {
      WARNING ("Error reading binary header from file");
      perror ("fread");
      return ERROR;
    }

  // check signature (DEFINE SOMEWHERE IN HEADER FILE...)
  if (!(binary_header->signature[0] == 'Q' &&
	binary_header->signature[1] == 'M' &&
	binary_header->signature[2] == 'T' &&
	binary_header->signature[3] == '\0'))
    {
      WARNING ("Incorrect signature in binary file");
      return ERROR;
    }

  return SUCCESS;
}

// write header of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_binary_write_header_to_file (int interface_number,
				long int time_record_number,
				int major_version, int minor_version,
				int subminor_version, int svn_revision,
				FILE * binary_file)
{
  struct binary_header_class binary_header;

  binary_header.signature[0] = 'Q';
  binary_header.signature[1] = 'M';
  binary_header.signature[2] = 'T';
  binary_header.signature[3] = '\0';

  binary_header.major_version = major_version;
  binary_header.minor_version = minor_version;
  binary_header.subminor_version = subminor_version;
  binary_header.svn_revision = svn_revision;

  /*
     binary_header.reserved[0] = 0xde;
     binary_header.reserved[1] = 0xad;
     binary_header.reserved[2] = 0xbe;
     binary_header.reserved[3] = 0xef;
   */
  binary_header.interface_number = interface_number;
  binary_header.time_record_number = time_record_number;

#ifdef MESSAGE_DEBUG
  io_binary_print_header (&binary_header);
#endif

  // write header to file
  if (fwrite (&binary_header, sizeof (struct binary_header_class),
	      1, binary_file) != 1)
    {
      WARNING ("Error writing binary output header to file");
      perror ("fwrite");
      return ERROR;
    }

  return SUCCESS;
}

// read a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_binary_read_time_record_from_file (struct binary_time_record_class
				      *binary_time_record,
				      FILE * binary_input_file)
{
  // read time record from file
  if (fread (binary_time_record, sizeof (struct binary_time_record_class),
	     1, binary_input_file) != 1)
    {
      WARNING ("Error reading binary time record from file");
      perror ("fread");
      return ERROR;
    }

  return SUCCESS;
}

/*
// write a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_write_binary_time_record_to_file (float time, int record_number,
				     FILE * binary_file)
{
  binary_time_record_class binary_time_record;

  binary_time_record.time = time;
  binary_time_record.record_number = record_number;

  // write time record to file
  if (fwrite (&binary_time_record,
	      sizeof (binary_time_record_class), 1, binary_file) != 1)
    {
      WARNING ("Error writing binary output time record to file");
      return ERROR;
    }

  return SUCCESS;
}
*/

// directly write a time record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_binary_write_time_record_to_file2 (struct binary_time_record_class
				      *binary_time_record, FILE * binary_file)
{
  // write time record to file
  if (fwrite (binary_time_record,
	      sizeof (struct binary_time_record_class), 1, binary_file) != 1)
    {
      WARNING ("Error writing binary output time record to file");
      perror ("fwrite");
      return ERROR;
    }

  return SUCCESS;
}

// read a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_binary_read_record_from_file (struct binary_record_class *binary_record,
				 FILE * binary_input_file)
{
  // record from file
  if (fread (binary_record, sizeof (struct binary_record_class),
	     1, binary_input_file) != 1)
    {
      WARNING ("Error reading binary record from file");
      perror ("fread");
      return ERROR;
    }

  return SUCCESS;
}

// read 'number_records' records from a QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_binary_read_records_from_file (struct binary_record_class *binary_records,
				  int number_records,
				  FILE * binary_input_file)
{
  // records from file
  //printf("Reading %d records from file\n", number_records);
  //fflush(stdout);

  if (fread (binary_records, sizeof (struct binary_record_class),
	     number_records, binary_input_file) != number_records)
    {
      WARNING ("Error reading binary records from file");
      perror ("fread");
      return ERROR;
    }

  return SUCCESS;
}

/*
// write a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_write_binary_record_to_file (struct connection_class * connection,
				struct scenario_class * scenario, FILE * binary_file)
{
  binary_record_class binary_record;

  binary_record.from_node = (scenario->nodes[connection->from_node_index]).id;
  binary_record.to_node = (scenario->nodes[connection->to_node_index]).id;
  binary_record.num_retransmissions = connection->num_retransmissions;
  binary_record.operating_rate = connection->operating_rate;
  binary_record.bandwidth = connection->bandwidth;
  binary_record.loss_rate = connection->loss_rate;
  binary_record.delay = connection->delay;

  //////////////////////////////////////////////////////////////////
  // NOTE: WHEN MORE FIELDS WILL BE ADDED, TAKE CARE TO CONVERT
  // X & Y COORDINATES TO LAT & LONG IF CARTESIAN SYSTEM IS NOT USED
  //////////////////////////////////////////////////////////////////

  // write record to file
  if (fwrite (&binary_record, sizeof (binary_record_class),
	      1, binary_file) != 1)
    {
      WARNING ("Error writing binary output record to file");
      perror("fwrite");
      return ERROR;
    }

  return SUCCESS;
}
*/

// directly write a record of QOMET binary output file;
// return SUCCESS on succes, ERROR on error
int
io_binary_write_record_to_file2 (struct binary_record_class *binary_record,
				 FILE * binary_file)
{
  //////////////////////////////////////////////////////////////////
  // NOTE: WHEN MORE FIELDS WILL BE ADDED, TAKE CARE TO CONVERT
  // X & Y COORDINATES TO LAT & LONG IF CARTESIAN SYSTEM IS NOT USED
  //////////////////////////////////////////////////////////////////

  // write record to file
  if (fwrite (binary_record, sizeof (struct binary_record_class),
	      1, binary_file) != 1)
    {
      WARNING ("Error writing binary output record to file");
      perror ("fwrite");
      return ERROR;
    }

  return SUCCESS;
}
