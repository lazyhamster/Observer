
/*********************************************************************
 **   Written for Wildcard Matching Functions Library
 *********************************************************************
 *
 * This software is OSI Certified Open Source Software. OSI Certified 
 * is a certification mark of the Open Source Initiative. Gifted to all
 * peoples with good will. Distributed under the MIT License.
 *
 * For questions, help or comments please contact
 * Martin Kozák (martin.kozak@openoffice.cz).
 *  
 * Thanks to user '' on http://www../ which has written this code.
 *
 * Copyright © 2004 - 2005 Martin Kozák
 *
 */



 // *************************************************************************** 
 // *   Headers
 // *************************************************************************** 

	#include "libwdmatch.h"
	#include <ctype.h>


 // *************************************************************************** 
 // *   Functions
 // *************************************************************************** 

	// Case-sensitive pattern match
	
	int libwdmatch_match(const char* pattern, const char* string)
	{
		switch (pattern[0])
		{
			case '\0':
				return !string[0];
	
			case '*':
				return libwdmatch_match(pattern+1, string) || string[0] && libwdmatch_match(pattern, string+1);

			case '?':
				return string[0] && libwdmatch_match(pattern+1, string+1);

			default:
				return (pattern[0] == string[0]) && libwdmatch_match(pattern+1, string+1);
		}
	}


	// Case-insensitive pattern match

	int libwdmatch_match_i(const char* pattern, const char* string)
	{
		switch (pattern[0])
		{
			case '\0':
				return !string[0];
	
			case '*' :
				return libwdmatch_match_i(pattern+1, string) || string[0] && libwdmatch_match_i(pattern, string+1);

			case '?' :
				return string[0] && libwdmatch_match_i(pattern+1, string+1);
			
			default :	
				return (toupper(pattern[0]) == toupper(string[0])) && libwdmatch_match_i(pattern+1, string+1);
		}
	}
