//=================================================================
//
//        strcat2.c
//
//        Testcase for C library strcat()
//
//=================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//=================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):     ctarpy, jlarmour
// Contributors:  
// Date:          2000-04-30
// Description:   Contains testcode for C library strcat() function
//
//
//####DESCRIPTIONEND####

// INCLUDES

#include <pkgconf/isoinfra.h>
#include <string.h>
#include <cyg/infra/testcase.h>

// CONSTANTS

#define NUM_ROBUSTNESS_RUNS 300

// FUNCTIONS

// Functions to avoid having to use libc strings

static int my_strcmp(const char *s1, const char *s2)
{
    for ( ; *s1 == *s2 ; s1++,s2++ )
    {
        if ( *s1 == '\0' )
            break;
    } // for

    return (*s1 - *s2);
} // my_strcmp()


static char *my_strcpy(char *s1, const char *s2)
{
    while (*s2 != '\0') {
        *(s1++) = *(s2++);
    }
    *s1 = '\0';

    return s1; 
} // my_strcpy()


#if CYGINT_ISO_MAIN_STARTUP
int main( int argc, char *argv[] )
#else
void cyg_user_start(void)
#endif
{
    char x[300];
    char y[300];
    char *ret;
    int ctr;
    int fail;

    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for C library "
                  "strcat() function");
    CYG_TEST_INFO("This testcase tests robustness, and may take some time");

    fail = 0;
    for (ctr = 0; ctr < NUM_ROBUSTNESS_RUNS; ctr++)
    {
        my_strcpy(x, "in the land of Mordor, ");
        my_strcpy(y, "where the shadows lie.");
        ret = strcat(x, y);
        if ((my_strcmp(x, "in the land of Mordor, where the shadows lie.")!=0))
        {
             fail = 1;
             break;
        } // if
        if (ret != x)
        {
            fail = 1;
            break;
        } // if
    }
    CYG_TEST_PASS_FAIL( (fail == 0), "Robustness test passed" );

//    CYG_TEST_NA("Testing is not applicable to this configuration");

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for C library "
                    "strcat() function");

} // main()


// EOF strcat2.c
