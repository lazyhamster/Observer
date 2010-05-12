
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
 * Copyright © 2005 Martin Kozák
 *
 */


 
 
 #if !defined(_LIBWDMATCH_H)
	
	 #define _LIBWDMATCH_H 
 

 	// *************************************************************************** 
 	// *   Functions
 	// *************************************************************************** 

 	// wdmatch.c
#ifdef __cplusplus
extern "C" {
#endif
  int libwdmatch_match_i(const char* pattern, const char* string);
  int libwdmatch_match(const char* pattern, const char* string);
#ifdef __cplusplus
}
#endif

#endif

