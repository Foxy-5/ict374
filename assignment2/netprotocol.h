/**
 * file:        netprotocol.h
 * Author:      Seow Wei Cheng (33753618) and Jin Min Seok (33884206)
 * Date:        13/11/2021 (version 2)
 * Purpose:     This is the network protocol description file
 *              This file defines the op code that is to be used
 *              for both client and server
 *  
 */

#define PUT_CODE1 'P'
#define PUT_CODE2 'R'
#define PUT_READY '0'
#define PUT_CLASH_ERROR '1'
#define PUT_DONE '0'
#define PUT_FAIL '1'

#define GET_CODE1 'G'
#define GET_CODE2 'R'
#define GET_READY '0'
#define GET_NOT_FOUND '1'

#define PWD_CODE 'W'
#define PWD_READY '0'
#define PWD_ERROR '1'

#define DIR_CODE 'D'
#define DIR_READY '0'
#define DIR_ERROR '1'

#define CD_CODE 'C'
#define CD_READY '0'
#define CD_ERROR '1'
