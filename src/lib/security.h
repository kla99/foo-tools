/*
 * foo-tools, a collection of utilities for glftpd users.
 * Copyright (C) 2003  Tanesha FTPD Project, www.tanesha.net
 *
 * This file is part of foo-tools.
 *
 * foo-tools is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * foo-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with foo-tools; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * Some lib to do smth about security  /sorend
 *
 * $Id: security.h,v 1.2 2003/01/22 14:31:29 sorend Exp $
 */

/*
 * The network interface which the network ip must be on.
 */
#define NETINTERFACE "eth0"

/*
 * Fills ip with the local ip of a box.
 *
 * Returns 1 if local ip was found, and 0 on error. 
 */
int get_local_ip(char *ip);

/*
 * Matches some ip against list of allowed ips.
 *
 * Returns 1 if the ip is allowed, and 0 if not.
 */
int is_secure_ip(char *ip);

