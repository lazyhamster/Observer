/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  GMime
 *  Copyright (C) 2000-2012 Jeffrey Stedfast
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */


#ifndef __GMIME_VERSION_H__
#define __GMIME_VERSION_H__

/**
 * GMIME_MAJOR_VERSION:
 *
 * GMime's major version.
 **/
#define GMIME_MAJOR_VERSION (2)

/**
 * GMIME_MINOR_VERSION:
 *
 * GMime's minor version.
 **/
#define GMIME_MINOR_VERSION (6)

/**
 * GMIME_MICRO_VERSION:
 *
 * GMime's micro version.
 **/
#define GMIME_MICRO_VERSION (20)

/**
 * GMIME_BINARY_AGE:
 *
 * GMime's binary age.
 **/
#define GMIME_BINARY_AGE    (620)

/**
 * GMIME_INTERFACE_AGE:
 *
 * GMime's interface age.
 **/
#define GMIME_INTERFACE_AGE (0)


/**
 * GMIME_CHECK_VERSION:
 * @major: Minimum major version
 * @minor: Minimum minor version
 * @micro: Minimum micro version
 *
 * Check whether a GMime version equal to or greater than
 * @major.@minor.@micro is present.
 **/
#define	GMIME_CHECK_VERSION(major,minor,micro)	\
    (GMIME_MAJOR_VERSION > (major) || \
     (GMIME_MAJOR_VERSION == (major) && GMIME_MINOR_VERSION > (minor)) || \
     (GMIME_MAJOR_VERSION == (major) && GMIME_MINOR_VERSION == (minor) && \
      GMIME_MICRO_VERSION >= (micro)))

#endif /* __GMIME_VERSION_H__ */
