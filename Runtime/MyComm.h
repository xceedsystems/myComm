/********************************************************************

            Pci1240.h             

   This file contains defintions for all of the common structures 
   and type definitions necessary to describe our hardware

**********************************************************************/



#ifndef  __MYCOMM_H__
#define  __MYCOMM_H__



/*============================================================================

 STATUS_BYTE - 

=============================================================================*/



/*=============================================================================

    TO DO:  define here the structure of the dual port


    DP_HEADER   - defines the structure of the dual port memory header.

=============================================================================*/

#pragma BYTE_ALIGN(_AXIS_MAP)  /* 1 byte alignement  */
typedef struct _AXIS_MAP
{
	 UINT8 Header[0x100]; /*offset*/
     UINT8 Command_1;	/* :0000 */
     UINT8 Command_2;	/* :0001 */
     UINT8 Command_3;	/* :0002 */
     UINT8 Status_1;	/* :0003 */
     UINT8 Status_2;	/* :0004 */
     UINT8 Status_3;	/* :0005 */
	 UINT8 Command_4;	/* :0006 */
     UINT8 Command_5;	/* :0007 */
	 UINT8  Spare_1[4];	/* :0008~B */
	 UINT32 P_Register;	/* :000C */
	 UINT8  Spare_2[4];	/* :0010 */
	 UINT32 D_Register;	/* :0014 */
	 UINT8  Spare_3[4];	/* :0017 */
	 UINT32 A_Register;	/* :001C */
	 UINT8  Spare_4[4];	/* :0020 */
	 UINT16 S_Register;	/* :0024 */
	 UINT16 M_Register;	/* :0026 */
	 UINT8  Spare_5[4];	/* :0028 */
	 UINT32 G_Register;	/* :002C */
	 UINT8  Spare_6[4];	/* :0030 */
	 UINT32 C_Register;	/* :0034 */
	 UINT8  Spare_7[4];	/* :0038 */
	 UINT8 N_Register;	/* :003c */
	 UINT8 Spare_8[7];	/* :003d~f */
	 UINT32 O_Register;	/* :0044 */
	 UINT8  Spare_9[4];	/* :0048 */
	 UINT32 Encoder;	/* :004c */
	 UINT8  Spare_10[4];	/* :0050 */
	 UINT32 Pulse;		/* :0054 */
	 UINT8  Spare_11[4];	/* :0058 */
	 UINT16 Freuency;	/* :005C */
	 UINT8 Spare_12[6];	/* :005e~63 */
	 UINT8 EC_CMD_1;	/* :0064 */
	 UINT8 EC_STAT;		/* :0065 */
	 UINT8 EC_CMD_2;	/* :0066 */
	 UINT8 Spare13[5];	/* :0067 */
	 UINT32 EP0_Register;	/* :006c */
	 UINT8  Spare14[4];		/* :0070 */
	 UINT32 EP1_Register;	/* :0074 */
	 UINT8  Spare15[8];		/* :0078 */
	 UINT32 EP2_Register;	/* :007c */
	 UINT8	Spare16[0x1000 - 0x180]; /* rest are blank*/

////4000~4007 NOT MAP HERE
	 

} AXIS_MAP;
#pragma BYTE_NORMAL()


// needs to allocate 64K space



#pragma BYTE_ALIGN(_DUAL_PORT) 
typedef struct _DUAL_PORT
{
//AXIS_MAP AxisA;
//AXIS_MAP AxisB;
//AXIS_MAP AxisC;
//AXIS_MAP AxisD;
UINT8	ByteDpr[DPR_TOTAL_SIZE];

} DUAL_PORT;
#pragma BYTE_NORMAL()


#endif            /* __PCI1240_H__ */

