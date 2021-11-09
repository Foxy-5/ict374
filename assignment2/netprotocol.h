/**
 * file:        netprotocol.h
 * Author:      Seow Wei Cheng (33753618)
 * Date:        9/11/2021 (version 1)
 * Purpose:     This is the network protocol description file
 *              This file defines the op code that is to be used
 *              for both client and server
 *  
 */

#define PUT_1 'P'
#define PUT_2 'D'
#define PUT_READY '0'
#define PUT_CLASH_ERROR '1'
#define PUT_CREATE_ERROR '2'
#define PUT_OTHER_ERROR '3'

#define GET_1 'G'
#define GET_2 'D'
#define GET_READY '0'
#define GET_NOT_FOUND '1'
#define GET_OTHER_ERROR '2'

#define PWD_CODE 'W'
#define PWD_READY '0'
#define PWD_ERROR '1'

#define DIR_CODE 'D'
#define DIR_READY '0'
#define DIR_ERROR '1'