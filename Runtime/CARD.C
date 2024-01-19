/********************************************************************

                                Card.c

    Interface specific code. 
    This file only should touch the hardware.

*********************************************************************/


#include "stdafx.h"

#include <rt.h>
#include <string.h>     // strlen()
#include <stdio.h>      // sprintf()

#include "vlcport.h"
#include "dcflat.h"     // EROP()
#include "driver.h"     /* SEMAPHORE */
#include "errors.h"     /* IDS_RT_DP_RW_TEST                     */
#include "auxrut.h"     /* StartTimeout(), IsTimeout(), EROP     */
#include "MyComm.h"   /* DUAL_PORT                             */
#include "card.h"       /* Init()                                */

static char				m_BoardName[10];

static int				m_BoardType=102;	/* board type  */
static unsigned long	m_ofs_a=0x1000;		/* axis offset */
static unsigned long	m_ofs_b=0x01;		/* byte offset */

static	int				m_IrqNo;
static	unsigned long	m_Address;
static  int				m_PciIndex;

static	int				m_BoardInitOk;
static	int				m_IncFlag;
static	int				m_ErrFlag;

static	struct			syspara m_Sys;
static	struct			movpara m_Move;

static	long			m_Position[4];
static	int				m_MoveFlag[4];
static	int				m_Nreg[4];
static	float			m_Kdata[4];
static	int				m_Tm[4];
static	int				m_Sq[4];


void DataInit()
{
	int axis =0;
	m_IrqNo=0;
	m_Address=0;
	m_PciIndex=0;

	m_BoardInitOk=0;
	m_IncFlag=0;	// 0=abs
	m_ErrFlag=0;	// 0=abs
	m_Move.Axis=0;

	m_Sys.EsLog=0;	// 0=A,1=B
	for(axis=0;axis<4;axis++){
		m_Sys.SpdMode[axis]=0;	// 0=slow,1=normal,2=fast
		m_Sys.AlmLog[axis]=0;	// 0=A,1=B
		m_Sys.NearLog[axis]=0;
		m_Sys.ZLog[axis]=0;
		m_Sys.OverLog[axis]=0;
		m_Sys.InpLog[axis]=0;
		m_Sys.PlsLog[axis]=0;
		m_Sys.ForDir[axis]=0;	// 0=ccw,1=cw
		m_Sys.ClkType[axis]=0;	// 0=2clk,1=1clk
		m_Sys.MotType[axis]=0;	// 0=stepping,1=sarvo
		m_Sys.HomeSeq[axis]=0;
		m_Sys.HomeMode[axis]=0;
		m_Sys.ZCnt[axis]=1;
		m_Sys.EncdMul[axis]=0;
		m_Sys.Encoder[axis]=0;
		m_Sys.Deviation[axis]=0;
		m_Sys.SType[axis]=0;
		m_Sys.SCRes[axis]=0;
		m_Sys.MaxSpd[axis]=0;
		m_Sys.PlsUnit[axis]=1.0;

		//m_Move.Deviation[axis]=0;
		m_Move.Max[axis]=1000;
		m_Move.Self[axis]=100;
		m_Move.Ramp[axis]=100;
		m_Move.Dir[axis]=0;
		m_Move.Pls[axis]=1;
		m_Position[axis]=0;
		m_MoveFlag[axis]=0;	//Set to Idle
		m_Nreg[axis]=25;
		m_Kdata[axis]=1.0;
		m_Tm[axis]=0;
		m_Sq[axis]=0;
	}
}


/*   not use yet
int FileLoad(char *fname)
{
FILE *fp;
char txt[30];
	fp=fopen(fname,"r");
	if(fp==NULL) return(-1);	
	fscanf(fp,"%s\n",txt);	
	fscanf(fp,"%s\n",txt);
	strcpy(m_BoardName,txt);
	fscanf(fp,"%d:%s\n",&m_BoardType,txt);
	fscanf(fp,"%d:%s\n",&m_IrqNo,txt);
	fscanf(fp,"%lx:%s\n",&m_Address,txt);
	fscanf(fp,"%d:%s\n",&m_PciIndex,txt);
	fscanf(fp,"%d:%s\n",&m_Sys.EsLog,txt);
	for(int axis=0;axis<4;axis++){
		fscanf(fp,"%d:%s\n",&m_Sys.SpdMode[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.AlmLog[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.NearLog[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.ZLog[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.OverLog[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.InpLog[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.PlsLog[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.ForDir[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.ClkType[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.MotType[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.HomeSeq[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.ZCnt[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.EncdMul[axis],txt);
		//S----- Add by CKTan-----
		fscanf(fp,"%d:%s\n",&m_Sys.HomeMode[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.Encoder[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.Deviation[axis],txt);
		//E----- Add by CKTan-----
		fscanf(fp,"%f:%s\n",&m_Sys.PlsUnit[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.SType[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.SCRes[axis],txt);
		fscanf(fp,"%d:%s\n",&m_Sys.MaxSpd[axis],txt);
	}
	fclose(fp);
	return(0); // ok
} */

/******************* Card specific  Functions  *******************************/


/******************* Initialization  *******************************/


static int TestAndFill(UINT8* pc, const int Size, const int test, const int fill)   /* test == 1  --> no test */
{
    int i  = 0;
    for(; i < Size;  *pc++ = fill, i++)
    {
        int c = *pc & 255;
        if(test != 1  &&  test != c)
        {
            EROP("Ram Error.  Address %p, is 0x%02x, and should be 0x%02x", pc, c, test, 0);
            return IDS_MCI0410_HW_TEST;
        }
    }
    return SUCCESS;
}


int  Init( LPDRIVER_INST pNet, P_ERR_PARAM const lpErrors)
{
    int rc = SUCCESS;

    return rc;
}



/****************************************************************************************
    IN:     pName   --> pointer to the device user name
            Address --> device's network address
            pBuf    --> pointer to the destination buffer
            szBuf   --> size of the destination buffer

    OUT:    *pBuf   <-- "Address xx (usr_name)".  
    Note:   The device user name may be truncated!
*/
static void LoadDeviceName( char* pName, UINT16 Address, char* pBuf, size_t szBuf )
{
    if( szBuf && (pBuf != NULL) )
    {
        char* format = "Address %d";

        *pBuf = '\0';

        if( szBuf > (strlen(format)+3) )    /* Address may need more digits */
        {
            size_t  len;

            sprintf(pBuf, format, Address & 0xffff);

            len = strlen( pBuf ); 

            if( pName && ((szBuf - len) > 10) )     /* if we still have 10 free bytes  */
            {
                strcat(pBuf, " (");
                len += 2;
                strncat( pBuf, pName, szBuf-len-2 );
                *(pBuf + szBuf - 2) = '\0';
                strcat( pBuf, ")" );
            }
        }
    }
}



int  TestConfig( LPDRIVER_INST const pNet, P_ERR_PARAM const lpErrors )
{
    int rc = SUCCESS;

    return rc;
}


/********************* runtime specific card access functions ********************/


int	DoCollect( LPDRIVER_INST pNet, LPSPECIAL_INST pData)
{
    int     rc       = SUCCESS;
//	int		channel;
//	UINT16	*ChanBuff[16];
//	UINT16	NumSamples = pData->Work.paramCommand.NumSamps;
	UINT16* pResult = BuildUiotPointer( pData->Work.paramHeader.ofsResult );
   
//	for (channel = 0; (channel < 16) && (rc == SUCCESS); channel++)
//	{
//		LPPTBUFFER pRBuffer = &pData->Work.paramCommand.Buffers[channel];

//		printf("Channel %d  Length %d  Offset %d\n", 
//			channel, pRBuffer->Size, pRBuffer->Offset);

//		ChanBuff[channel] = BuildUiotPointer( pRBuffer->Offset );
//		if( pRBuffer->Size > NumSamples)
//		{
//			rc = IDS_MCI0410_READ_SIZE;
//		}
//	}

	// At this time, ChanBuf[i] is a pointer to the buffer area for channel i.
	// Insert your code here.
	*pResult = rc;
	return  (rc);
}



//// read what is in the dpr into static variable

void LoadMCI0410Default(UINT32 PortAddr)
{
    int	rc = SUCCESS;

}

void MCI0410Set( const LPDRIVER_INST pNet, SPECIAL_INST* const pData )
{
    int rc = SUCCESS ;

	SPECIAL_INST_SETGET* pUser = &pData->User.paramSetGet;
    UINT16* pResult = BuildUiotPointer( pUser->Header.ofsResult );
    UINT8* Dest = (UINT8*)pNet->pDpr; 
//	UINT8* Destt;
	int Axis = pUser->Address;  
	int OffsetAxis=((Axis -1)* 0x1000)+0x100;

	UINT8  SetValueByte= (UINT8)(pUser->SetValue & 0xFF);
	UINT16 SetValueInt= (UINT16)(pUser->SetValue & 0xFFFF);
	UINT32 SetValueWord= (UINT32)(pUser->SetValue );
	UINT32 SetValueWord3= (UINT32)(pUser->SetValue & 0x00FFFFFF);

	UINT32 CmdOffset = pUser->Function &0xFF;
	UINT32 CmdValueType = pUser->Function &0x0F00;

	if (Axis == CR_AXIS) 
	OffsetAxis = 0x4000;


 switch(CmdValueType ) 
    {

	case	0x0100:	//Byte type
//	*((UINT8*)&Dest[OffsetAxis+CmdOffset]) = SetValueByte;
	*((UINT8*)&Dest[OffsetAxis+CmdOffset] )= SetValueByte;
//	Destt=*((UINT8*)&Dest[+OffsetAxis+CmdOffset]) ;

	break;

	case	0x0200:	//Word type
	*((UINT16*)&Dest[OffsetAxis+CmdOffset]) = SetValueInt;
		
	break;

	case	0x0300: // 12 bit type
	*((UINT32*)&Dest[OffsetAxis+CmdOffset]) = SetValueWord3;
		
	break;

	case	0x0400: // Dword type
	*((UINT32*)&Dest[OffsetAxis+CmdOffset]) = SetValueWord;
		
	break;

   	default:
    rc = -1;		// not such function
    break;


	}

	*pResult=rc;	

}



void MCI0410Var( const LPDRIVER_INST pNet, SPECIAL_INST* const pData )
{
    int rc = SUCCESS ;

	SPECIAL_INST_SET_KDATA* pUser = &pData->User.paraSetKdata ;
    UINT16* pResult = BuildUiotPointer( pUser->Header.ofsResult );

	int Axis = pUser->Address - 1;  

	int ValueInt	 = pUser->ValueInt ;
	long ValueLong	 = pUser->ValueLng ;
	float ValueFloat = pUser->ValueFlt ;


 switch(pUser->Function) 
    {
	case	U_K_DATA:	m_Kdata[Axis]=ValueFloat;
	break;

	case	U_DIR:		m_Move.Dir[Axis] = ValueInt;	
	break;

	case	U_PULSE	:	m_Move.Pls[Axis] = ValueLong;	
	break;

	case	U_MAX:		m_Move.Max[Axis] = ValueLong;	
	break;

	case	U_SELF: 	m_Move.Self[Axis]  = ValueLong;	
	break;

	case	U_RAMP: 	m_Move.Ramp[Axis]  = ValueLong;	
	break;

	case	U_STYPE: 	m_Sys.SType[Axis]  = ValueInt;	
	break;

	case	U_HOME_MODE: 	m_Sys.HomeMode[Axis]  = ValueInt;	
	break;

	case	U_HOME_TYPE: 	m_Sys.HomeSeq[Axis]  = ValueInt;	
	break;

	case	U_INTERPOL: 	m_Move.Axis = ValueInt;	
	break;


   	default:
    rc = -1;		// not such function
    break;

	}

	*pResult=rc;	

}




void MCI0410Function( const LPDRIVER_INST pNet, SPECIAL_INST* const pData )
{
    int rc = SUCCESS ;

	SPECIAL_INST_SETGET* pUser = &pData->User.paramSetGet;
    UINT16* pResult = BuildUiotPointer( pUser->Header.ofsResult );
    UINT8* Dest = (UINT8*)pNet->pDpr; 

	int Axis = pUser->Address;  
	int OffsetAxis=((Axis -1)* 0x1000)+0x100;

	if (Axis == CR_AXIS) 
	OffsetAxis = 0x4000;


 switch(pUser->Function ) 
    {

	case	ES_LOG:			rc=SetEsLog(pUser->SetValue,(Axis-1),Dest,OffsetAxis );
	break;
	case	RESET_INT:		rc=ResetInterrupt(pUser->SetValue,(Axis-1),Dest,OffsetAxis );
	break;
	case	SET_SP_MODE:	rc=SetSpeedMode(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_ALM_LOG:	rc=SetAlmLog(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_NER_LOG:	rc=SetNearLog(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_Z_LOG:		rc=SetZLog(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_OVER_LOG:	rc=SetOverLog(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_INP_LOG:	rc=SetInposLog(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_PULSE_LOG:	rc=SetPulseLog(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_FOR_DIR:	rc=SetForDir(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_CLK_TYPE:	rc=SetClockType(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_MOT_TYPE:	rc=SetMotorType(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_ENC_MUL:	rc=SetEncoderMul(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_HOM_RET:	rc=SetHomeRetMode(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_Z_CNT:		rc=SetZCount(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_CO:			rc=SetCo(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_SRV:		rc=SetSrv(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_CLR:		rc=SetClr(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_ES:			rc=SetEs(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_SS:			rc=SetSs(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_PULSE:		rc=SetPause(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;
	case	SET_MAIN_AXIS:	rc=SetMainAxis(pUser->SetValue,(Axis-1),Dest, OffsetAxis );
	break;

   	default:
    rc = -1;		// not such function
    break;


	}

	*pResult=rc;	

}

/// bit = 0 if log =0

int SetEsLog(int log,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x0005]) ;

	if (Axis!=CR_AXIS-1) return (-2);

		if(log==0) c &= 0xf7;			
		else	   c |= 0x08;
	*((UINT8*)&Dest[OffsetAxis+0x0005]) = c;

	return  SUCCESS;

}



int ResetInterrupt(int log,int Axis,UINT8* Dest,int OffsetAxis )

{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x0005]) ;

	if (Axis!=CR_AXIS-1) return (-2);

	c &= 0xef;

	*((UINT8*)&Dest[OffsetAxis+0x0005]) = c;

	return  SUCCESS;

}



int SetSpeedMode(int mode,int Axis,UINT8* Dest,int OffsetAxis )
{

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);


	m_Sys.SpdMode[Axis]=mode;
	if(mode==0)		{ m_Nreg[Axis]=25; m_Kdata[Axis]=1.0;	}
	else if(mode==1){ m_Nreg[Axis]=2; m_Kdata[Axis]=12.5;	}
	else			{ m_Nreg[Axis]=1; m_Kdata[Axis]=25.0;	}
//	SetNreg(axis,m_Nreg[axis]);

	*((UINT8*)&Dest[OffsetAxis]) = m_Nreg[Axis];
	return  SUCCESS;

}


int SetAlmLog(int log,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x06]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);


	m_Sys.AlmLog[Axis]=log;

	if(log==0)	c &= 0xfb;	/* a */
	else		c |= 0x04;	/* b */
	
	*((UINT8*)&Dest[OffsetAxis+0x06]) =c;
		return  SUCCESS;

}

int SetNearLog(int log,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x06]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	m_Sys.NearLog[Axis]=log;

		if(log==0)	c &= 0xfe;	/* a */
		else		c |= 0x01;	/* b */
	
	*((UINT8*)&Dest[OffsetAxis+0x06]) =c;
		return  SUCCESS;

}

int SetZLog(int log,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+Axis]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	m_Sys.ZLog[Axis]=log;
	
	if(log==0)	c &= 0xdf;	/* a */
	else		c |= 0x20;	/* b */
	*((UINT8*)&Dest[OffsetAxis+Axis]) =c;
		return  SUCCESS;

}



int SetOverLog(int log,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x06]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);
	
	m_Sys.OverLog[Axis]=log;

		if(log==0)	c &= 0xdf;	/* a */
		else		c |= 0x20;	/* b */
	
	*((UINT8*)&Dest[OffsetAxis+0x06]) =c;
		return  SUCCESS;

}

int SetInposLog(int log,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x06]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	m_Sys.InpLog[Axis]=log;

	
	if(log==0)	c &= 0xbf;	/* a */
	else		c |= 0x40;	/* b */
	
	*((UINT8*)&Dest[OffsetAxis+0x06]) =c;
		return  SUCCESS;

}

int SetPulseLog(int log,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x06]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	m_Sys.PlsLog[Axis]=log;

	if(log==0)	c &= 0x7f;	/* a */
	else		c |= 0x80;	/* b */
	
	*((UINT8*)&Dest[OffsetAxis+0x06]) =c;
		return  SUCCESS;

}

int SetForDir(int cw,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x07]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	
	m_Sys.ForDir[Axis]=cw;

	if(cw==0)	c &= 0xfe;	/* ccw */
	else		c |= 0x01;	/* cw */
	*((UINT8*)&Dest[OffsetAxis+0x07]) =c;
		return  SUCCESS;

}

int SetClockType(int clk,int Axis,UINT8* Dest,int OffsetAxis )
{
	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x07]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);


	if(clk==0)	c &= 0xfd;	/* 2clk */
	else		c |= 0x02;	/* 1clk */
	*((UINT8*)&Dest[OffsetAxis+0x07]) =c;

		return  SUCCESS;

}


int SetMotorType(int srv,int Axis,UINT8* Dest,int OffsetAxis )
{


	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x07]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	
	m_Sys.MotType[Axis]=srv;
	
	if(srv==0)	c &= 0xfb;	/* stepping */
	else		c |= 0x04;	/* srevo */
	*((UINT8*)&Dest[OffsetAxis+0x07]) =c;

	return  SUCCESS;

}

int SetEncoderMul(int mul,int Axis,UINT8* Dest,int OffsetAxis )
{
	//mul 0=1x(F),1=2x(F),2=4x(F),3=2Clk(F),4=1x(R),5=2x(R),6=4x(R),7=2Clk(R)
	/*	0000	0=1x(F)
		0001	1=2x(F)
		0010	2=4x(F)
		0011	3=2Clk(F)
		0100	4=1x(R)
		0101	5=2x(R)
		0110	6=4x(R)
		0111	7=2Clk(R)
	*/
	//m_Sys.Encoder[axis]
	/*
	{	m_nEC2Bit01[i]	= 3;	//Encoder clock (3-Bi-clock)
		m_nEC2Bit23[i]	= 3 - m_nSCW[i];	//Encoder input 2-PCW/DCCW, 3-DCCW/PCW
	}
	//E----- Encoder Setting -----

	mpgdata[axis].m_nEccmd[0]	= 0x00;		//@EC command 1
	mpgdata[axis].m_nEccmd[1]	= (BYTE)((BYTE)m_nEC2Bit01[axis] + (BYTE)(m_nEC2Bit23[axis]<<2));

	*/

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x66]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	m_Sys.EncdMul[Axis]=mul;
	

	if(m_Sys.Encoder[Axis]==1)
	{
//		c &= 0xfc;
//		c += mul;
		c &= 0x00;
		c = (UINT8) mul;
	} else
	{
		c &= 0x00;
		c = (UINT8)(3 + ((3 - m_Sys.ForDir[Axis]) << 2));
	}
	*((UINT8*)&Dest[OffsetAxis+0x66]) =c;
	return  SUCCESS;

}

int SetHomeRetMode(int on,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+Axis]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

		if(on) c |= 0x10;
		else   c &= 0xef;
	*((UINT8*)&Dest[OffsetAxis+Axis]) =c;

		return  SUCCESS;

}

int SetZCount(int cnt,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+Axis]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);
	
	m_Sys.ZCnt[Axis]=cnt;

	c = (c & 0xf0)+(cnt & 0x0f);
	*((UINT8*)&Dest[OffsetAxis+Axis]) =c;
	return  SUCCESS;
	
}

int SetCo(int on,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x01]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	
	
	if(on)	c |= 0x20;
	else	c &= ~(0x20);
			c |= 0x03;
	
		*((UINT8*)&Dest[OffsetAxis+0x01]) =c;
	return  SUCCESS;

}

int SetSrv(int on,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x01]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

	
		if(on)	c |= 0x10;
		else	c &= ~(0x10);
		c |= 0x03;

		*((UINT8*)&Dest[OffsetAxis+0x01]) =c;

		return  SUCCESS;

}

int SetClr(int on,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+Axis]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);


		if(on)	c |= 0x40;
		else	c &= 0xbf;
		*((UINT8*)&Dest[OffsetAxis+Axis]) =c;

	return  SUCCESS;

}

int SetEs(int on,int Axis,UINT8* Dest,int OffsetAxis )
{
	int cnt;
	UINT8 c;
	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);


	for(cnt=0;cnt<4;cnt++){
			if(Axis<4 && cnt!=Axis) continue;
			c  = *((UINT8*)&Dest[OffsetAxis+0x0004]) ;
			c |= (0x10<<cnt);
			*((UINT8*)&Dest[OffsetAxis+0x0004]) =c;
		}
		for(cnt=0;cnt<4;cnt++){
			if(Axis<4 && cnt!=Axis) continue;
			c  = *((UINT8*)&Dest[OffsetAxis+0x0004]) ;
			c &= ~(0x10<<cnt);
			*((UINT8*)&Dest[OffsetAxis+0x0004]) =c;

		}

			return  SUCCESS;

}

int SetSs(int on,int Axis,UINT8* Dest,int OffsetAxis )
{
	int cnt;
	UINT8 c;
	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

		for(cnt=0;cnt<4;cnt++){
			if(Axis<4 && cnt!=Axis) continue;
			c  = *((UINT8*)&Dest[OffsetAxis+0x0004]) ;
			c |= (0x01<<cnt);
			*((UINT8*)&Dest[OffsetAxis+0x0004]) =c;
		}
		for(cnt=0;cnt<4;cnt++){
			if(Axis<4 && cnt!=Axis) continue;
			c  = *((UINT8*)&Dest[OffsetAxis+0x0004]) ;
			c &= ~(0x01<<cnt);
			*((UINT8*)&Dest[OffsetAxis+0x0004]) =c;
		}
	return  SUCCESS;

}

int SetPause(int on,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c  = *((UINT8*)&Dest[OffsetAxis+0x0005]) ;

	if (Axis!=CR_AXIS-1) return (-2);

		if(on)	c |= 0x04;
		else	c &= ~0x04;
			*((UINT8*)&Dest[OffsetAxis+0x0005]) =c;
	return  SUCCESS;
	
}

int SetMainAxis(int on,int Axis,UINT8* Dest,int OffsetAxis )
{

	UINT8 c  = *((UINT8*)&Dest[OffsetAxis+0x0005]) ;

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);

 c &= 0xfc;
 c += Axis;
			*((UINT8*)&Dest[OffsetAxis+0x0005]) =c;

	return  SUCCESS;

}





int MoveHomeSet(int on,int Axis,UINT8* Dest,int OffsetAxis )
{
long lnum;
UINT8 cmd;

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);
	//test d0 for pulse out			
	if((c & 0x01)==1) return (-100 * Axis); //// Axis is moving 

	
	*((UINT32*)&Dest[OffsetAxis+0x0C]) = 0;	// set p reg 

	*((UINT16*)&Dest[OffsetAxis+0x24]) = (long)(m_Move.Self[Axis]/m_Kdata[Axis]); // sreg
	*((UINT16*)&Dest[OffsetAxis+0x26]) = (long)(m_Move.Max[Axis]/m_Kdata[Axis]);  //mreg


		lnum=(m_Move.Ramp[Axis]*1000)/(long)(2*m_Kdata[Axis]*m_Kdata[Axis]*m_Nreg[Axis]);
	*((UINT16*)&Dest[OffsetAxis+0x2C]) = (UINT16)lnum;	//g reg
	*((UINT16*)&Dest[OffsetAxis+0x2E]) = (UINT16)lnum;	// g reg
		
		//e0=1110,1:Auto Cal Dec point. 11:Slope Res 31Steps.
		//     00:Slope Monitor=Even Spd
		//       000:Memory Channel=0

		*((UINT8*)&Dest[OffsetAxis+0x02]) = 0xe0; //cmd 3

		cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2
		cmd |= 0x03;	//2:Clr Err o/p &Status1, 1:Clr INT o/p &Status1
		*((UINT8*)&Dest[OffsetAxis+0x01])=cmd;	//CMD2
		m_Sq[Axis]=1;


return SUCCESS;
}


int MoveNear(int dir,int Axis,UINT8* Dest,int OffsetAxis )
{
long lnum;
UINT8 cmd;

	UINT8 c	= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);
	//test d0 for pulse out			
	if((c & 0x01)==1) return (-100 * Axis); //// Axis is moving 

	
	*((UINT32*)&Dest[OffsetAxis+0x0C]) = 0;	// set p reg 

	*((UINT16*)&Dest[OffsetAxis+0x24]) = (long)(m_Move.Self[Axis]/m_Kdata[Axis]); // sreg
	*((UINT16*)&Dest[OffsetAxis+0x26]) = (long)(m_Move.Max[Axis]/m_Kdata[Axis]);  //mreg


	lnum=(m_Move.Ramp[Axis]*1000)/(long)(2*m_Kdata[Axis]*m_Kdata[Axis]*m_Nreg[Axis]);
	*((UINT16*)&Dest[OffsetAxis+0x2C]) = (UINT16)lnum;	//g reg
	*((UINT16*)&Dest[OffsetAxis+0x2E]) = (UINT16)lnum;	// g reg

//	if(dir)				//4=0100, 01:Trapezoid Ac/Dc mode. 00:Normal Ac/Dc.
//		cmd = 0x47;		//7=0111, 01:Dec & Stop when NEAR=1. 11:For Run
//	else cmd = 0x45;	//5=0101, 01:Dec & Stop when NEAR=1. 01:Rev Run

	if(dir)				//4=0100, 01:Trapezoid Ac/Dc mode. 00:Normal Ac/Dc.
		cmd = 0x6b;	//6:0110-01:Trapezoid Ac/Dc mode. 10:Pulse at startSpd. 0001:For RUN
	else cmd = 0x69;	//6:0110-01:Trapezoid Ac/Dc mode. 10:Pulse at startSpd. 0011:Rev RUN
	
	*((UINT8*)&Dest[OffsetAxis+0x00])=cmd;	//CMD1

	m_MoveFlag[Axis] = 4;

	return SUCCESS;

}


int MoveIndex(int on,int Axis,UINT8* Dest,int OffsetAxis )
{

long g,c, lnum;
UINT8 cmd;
int cnt;

	UINT8 cByte	= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

	if (Axis<0)  return (-2);
	if (Axis>3)  return (-2);
	//test d0 for pulse out			
	if((cByte & 0x01)==1) return (-100 * Axis); //// Axis is moving 

//		SetPreg(axis,p);	done externally set num of pulse
	*((UINT32*)&Dest[OffsetAxis+0x0C]) = m_Move.Pls[Axis] ;	// set p reg 

	*((UINT16*)&Dest[OffsetAxis+0x24]) = (long)(m_Move.Self[Axis]/m_Kdata[Axis]); // sreg
	*((UINT16*)&Dest[OffsetAxis+0x26]) = (long)(m_Move.Max[Axis]/m_Kdata[Axis]);  //mreg


	lnum=(m_Move.Ramp[Axis]*1000)/(long)(2*m_Kdata[Axis]*m_Kdata[Axis]*m_Nreg[Axis]);
	*((UINT16*)&Dest[OffsetAxis+0x2C]) = (UINT16)lnum;	//g reg
	*((UINT16*)&Dest[OffsetAxis+0x2E]) = (UINT16)lnum;	// g reg

	g=lnum;

	*((UINT8*)&Dest[OffsetAxis+0x02]) = 0xe0; //cmd 3
		
		cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2
		cmd |= 0x03;
		*((UINT8*)&Dest[OffsetAxis+0x01])=cmd ;

		if(m_Sys.SType[Axis]){
			c *= lnum*256*30;
			c /= (m_Move.Max[Axis]-m_Move.Self[Axis])/(long)(m_Kdata[Axis]);

//			SetCreg(Axis,c);
			*((UINT32*)&Dest[OffsetAxis+0x0034])=c;

			for(cnt=0;cnt<16;cnt++){
				if(cnt<4) c=(g*4)/8;
				else if(cnt<8) c=(lnum*(cnt+1))/8;
				else if(cnt<13) c=(lnum*cnt)/8;
				else c=(lnum*12)/8;

				*((UINT16*)&Dest[(OffsetAxis-0x100)+(cnt*2)])=c;

			}
			cmd=0x01;
		}
		else cmd=0x41;

		if(m_Move.Dir[Axis]==0) cmd |= 0x02;

		m_MoveFlag[Axis]=1;	//Set to Index move
		*((UINT8*)&Dest[OffsetAxis+0x00])=cmd;
	
	return SUCCESS;

}



int MoveHomeSq(int on,int Axis,UINT8* Dest,int OffsetAxis )
{
int flag;
UINT8 cmd,sts;

	flag=0x0f;

//	for(axis=0;axis<4;axis++){

	switch(m_Sq[Axis]){
			case 1:
				if(m_Sys.HomeSeq[Axis] & 0x02)	//4=0100, 01:Trapezoid Ac/Dc mode. 00:Normal Ac/Dc.
					 cmd = 0x47;	//7=0111, 01:Dec & Stop when NEAR=1. 11:For Run
				else cmd = 0x45;	//5=0101, 01:Dec & Stop when NEAR=1. 01:Rev Run

				*((UINT8*)&Dest[OffsetAxis+0x00])=cmd;	//CMD1
				m_MoveFlag[Axis]=4;	//Set to homing
				m_Sq[Axis]++;
				break;
			case 2:
				//sts=GetLsiStatus(axis,1);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

				if(sts & 0x01) break;	//EStop
				if(sts & 0x20){			//Servo
//					sts=GetLsiStatus(Axis,2);
					sts= *((UINT8*)&Dest[OffsetAxis+0x0004]) ;	/// read status 2

					if(sts & 0x03) m_Sq[Axis]=30;	/* err */
					else m_Sq[Axis]=20;			/* over run	*/
				}
				else{
					m_Sq[Axis]=3;
				}
//				cmd=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b));		//CMD2
				cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2
				cmd |= 0x03;	//2:Clr Err o/p &Status1, 1:Clr INT o/p &Status1
//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b),cmd);	//CMD2
				*((UINT8*)&Dest[OffsetAxis+0x01])=cmd ;		//CMD2
	
				break;
			case 3:
//				sts=GetLsiStatus(axis,1);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

				if(sts & 0x08)	//Rev Limit
						m_Sq[Axis]=5;		/* near on */
				else	m_Sq[Axis]=4;		/* near off */
				if(m_Sys.HomeSeq[Axis] & 0x02)
					 cmd = 0x61;	//6:0110-01:Trapezoid Ac/Dc mode. 10:Pulse at startSpd. 0001:Rev RUN
				else cmd = 0x63;	//6:0110-01:Trapezoid Ac/Dc mode. 10:Pulse at startSpd. 0011:For RUN
//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x100*m_ofs_b),cmd);	//CMD1
				*((UINT8*)&Dest[OffsetAxis+0x00])=cmd ;		//CMD1

				break;
			case 4:
//				sts=GetLsiStatus(axis,1);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

				if((sts & 0x01)==0){
					m_Sq[Axis]=30;
//					cmd=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b));		//CMD2
				cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2
					cmd |= 0x03;	//2:Clr Err o/p &Status1, 1:Clr INT o/p &Status1
//					_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b),cmd);	//CMD2
				*((UINT8*)&Dest[OffsetAxis+0x01])=cmd ;		//CMD2

				}
				else{
					if(sts & 0x08)
						m_Sq[Axis]++;
				}
				break;
			case 5:
//				sts=GetLsiStatus(axis,1);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

				if((sts & 0x01)==0){
					m_Sq[Axis]=30;
//					cmd=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b));		//CMD2
					cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2
					cmd |= 0x03;	//2:Clr Err o/p &Status1, 1:Clr INT o/p &Status1
//					_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b),cmd);	//CMD2
				*((UINT8*)&Dest[OffsetAxis+0x01])=cmd ;		//CMD2

				}
				else if((sts & 0x08)==0){
//					cmd=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x100*m_ofs_b));		//CMD1
					cmd=*((UINT8*)&Dest[OffsetAxis+0x00]) ;		//CMD1

					//f=1111-11:Pulse Rst o/p, 11:Pulse at MaxSpd
					//e=1110-11:Dec when NEAR=1=>EStop when Z=1, 10:ForDir
					cmd &= 0xfe;	
//					_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x100*m_ofs_b),cmd);	//CMD1
					*((UINT8*)&Dest[OffsetAxis+0x00])=cmd ;		//CMD1

					m_Sq[Axis]++;
				}
				break;
			case 6:
//				sts=GetLsiStatus(axis,1);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

				if((sts & 0x01)==0){
//					cmd=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b));		//CMD2
					cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2
					cmd |= 0x03;	//2:Clr Err o/p &Status1, 1:Clr INT o/p &Status1
//					_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b),cmd);	//CMD2
					*((UINT8*)&Dest[OffsetAxis+0x01])=cmd ;		//CMD2

					m_Sq[Axis]++;
				}
				break;
			case 7:
//				SetHomeRetMode(axis,1);
				SetHomeRetMode(1,Axis,Dest,0x4000 );

				if(m_Sys.HomeSeq[Axis] & 0x02)
					 cmd = 0x63;	//6:0110-01:Trapezoid Ac/Dc mode. 10:Pulse at startSpd. 0011:For RUN
				else cmd = 0x61;	//6:0110-01:Trapezoid Ac/Dc mode. 10:Pulse at startSpd. 0001:Rev RUN
				if(m_Sys.HomeSeq[Axis] & 0x01)
					 cmd |= 0x0c;	//11:Dec when NEAR=1=>EStop when Z=1
				else cmd |= 0x08;	//10:EStop when NEAR=1
//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x100*m_ofs_b),cmd);	//CMD1
				*((UINT8*)&Dest[OffsetAxis+0x00])=cmd ;		//CMD1

				m_Sq[Axis]++;
				break;
			case 8:
//				sts=GetLsiStatus(axis,1);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

				if(sts & 0x01) break;
				if(sts & 0x20){
					m_Sq[Axis]=30;
				}
				else{
					m_Sq[Axis]++;
				}
//				cmd=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b));		//CMD2
				cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2

				cmd |= 0x03;	//2:Clr Err o/p &Status1, 1:Clr INT o/p &Status1
//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b),cmd);	//CMD2
				*((UINT8*)&Dest[OffsetAxis+0x01])=cmd ;		//CMD2

				break;
			case 9:
				if(m_BoardType==101){
					m_Sq[Axis]++;
					break;
				}
//				SetHomeRetMode(Axis,0);
				SetHomeRetMode(1,Axis,Dest,0x4000 );

				SetPosition(Axis,0);	//Set Encoder into Zero
				m_MoveFlag[Axis]=0;	//Set to Idle
				m_Sq[Axis]=0;
				break;
			case 10:
//				SetClr(Axis,1);
				SetClr(1,Axis,Dest, 0x4000 );
				m_Tm[Axis]=0;
				m_Sq[Axis]++;
				break;
			case 11:
				RtSleep(1);
				m_Tm[Axis]++;
				if(m_Tm[Axis]>=20){
//					SetClr(Axis,0);
				SetClr(0,Axis,Dest, 0x4000 );

					SetPosition(Axis,0);
					m_MoveFlag[Axis]=0;		//Set to Idle
					m_Sq[Axis]=0;
				}
				break;
			case 20:
//				SetClr(Axis,1);
				SetClr(1,Axis,Dest, 0x4000 );

				m_Tm[Axis]=0;
				m_Sq[Axis]++;
				break;
			case 21:
				RtSleep(1);
				m_Tm[Axis]++;
				if(m_Tm[Axis]==20)	SetClr(0,Axis,Dest, 0x4000 );
				//SetClr(Axis,0);
				if(m_Tm[Axis]>=500) m_Sq[Axis]++;
				break;
			case 22:
				if(m_Sys.HomeSeq[Axis] & 0x02)	//4=0100, 01:Trapezoid Ac/Dc mode. 00:Normal Ac/Dc
					 cmd = 0x45;	//5:0101, 01:Dec & Stop when NEAR=1. 01:Rev Run
				else cmd = 0x47;	//5:0101, 01:Dec & Stop when NEAR=1. 11:For Run
//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x100*m_ofs_b),cmd);	//CMD1
				cmd=*((UINT8*)&Dest[OffsetAxis+0x00]) ;		//CMD1

				m_MoveFlag[Axis]=1;	//Set to Index move
				m_Sq[Axis]++;
				break;
			case 23:
//				sts=GetLsiStatus(axis,1);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0003]) ;	/// read status 1

				if(sts & 0x01) break;
				if(sts & 0x20){
					m_Sq[Axis]=30;	/* err */
				}
				else{
					if(sts & 0x08)
							m_Sq[Axis]=3;		/* near on */
					else	m_Sq[Axis]=7;		/* near off */
				}
//				cmd=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b));		//CMD2
				cmd=*((UINT8*)&Dest[OffsetAxis+0x01]) ;		//CMD2

				cmd |= 0x03;	//2:Clr Err o/p &Status1, 1:Clr INT o/p &Status1
//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b),cmd);	//CMD2
				*((UINT8*)&Dest[OffsetAxis+0x01])=cmd ;		//CMD2

				break;
			case 30:
				if(m_ErrFlag==0){
//					sts=GetLsiStatus(axis,2);
				sts= *((UINT8*)&Dest[OffsetAxis+0x0004]) ;	/// read status 2

					if(sts & 0x01)
						m_ErrFlag=101+(100*Axis);
					else if(sts & 0x02)
						m_ErrFlag=102+(100*Axis);
					else if(sts & 0x04)
						m_ErrFlag=103+(100*Axis);
					else if(sts & 0x08)
						m_ErrFlag=104+(100*Axis);
				}
				m_Sq[Axis]=9;
				break;
			default:
				flag &= ~(0x01<<Axis);
		}
//	} //for
	return(flag);
}

void SetPosition(int axis,long pos)
{
	m_Position[axis]=pos;
}



int MoveInterporation(int on,int Axis,UINT8* Dest,int OffsetAxis )
{
int axis,cnt,rev,maina,flag;
long p,g,c,mainp;
unsigned char cmd[4];
UINT8 sts;
int offset;

	mainp=0;
	maina=0;
	flag=m_Move.Axis;
	for(axis=0;axis<4;axis++){
		if((flag & (0x01<<axis))==0) continue;
		offset=(axis * 0x1000)+0x100;
		sts= *((UINT8*)&Dest[offset+0x0003]) ;	/// read status 1

//		if(m_MoveFlag[axis]){	//Check if busy
		if(sts & 0x01){
			flag &= ~(0x01<<axis);
			continue;
		}
		p=m_Move.Pls[axis];
		rev=m_Move.Dir[axis];
		if(p==0){
			flag &= ~(0x01<<axis);
			continue;
		}
		if(p>mainp){
			mainp=p;
			maina=axis;
		}

/*		SetPreg(axis,p);
		SetSreg(axis,(long)(m_Move.Self[axis]/m_Kdata[axis]));
		SetMreg(axis,(long)(m_Move.Max[axis]/m_Kdata[axis]));
		g=(long)((m_Move.Ramp[axis]*1000)/(2*m_Kdata[axis]*m_Kdata[axis]*m_Nreg[axis]));
		SetGreg(axis,g);

*/
	offset=(axis * 0x1000)+0x100;

	*((UINT32*)&Dest[offset+0x0C]) = p ;	// set p reg 
	*((UINT16*)&Dest[offset+0x24]) = (long)(m_Move.Self[Axis]/m_Kdata[Axis]); // sreg
	*((UINT16*)&Dest[offset+0x26]) = (long)(m_Move.Max[Axis]/m_Kdata[Axis]);  //mreg
	g=(m_Move.Ramp[Axis]*1000)/(long)(2*m_Kdata[Axis]*m_Kdata[Axis]*m_Nreg[Axis]);
	*((UINT16*)&Dest[offset+0x2C]) = (UINT16)g;	//g reg
	*((UINT16*)&Dest[offset+0x2E]) = (UINT16)g;	// g reg


//		_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x102*m_ofs_b),0xe0);
//		cmd[axis]=_MemReadChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b));

		*((UINT8*)&Dest[offset+0x02]) = 0xe0; //cmd 3
		
		cmd[axis]=*((UINT8*)&Dest[offset+0x01]) ;		//CMD2

		cmd[axis] |= 0x03;
//		_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x101*m_ofs_b),cmd[axis]);
		*((UINT8*)&Dest[offset+0x01])=cmd[axis] ;		//CMD2
		if(m_Sys.SType[axis]){
			c *= g*256*30;
			c = (long)(c/(m_Move.Max[axis]-m_Move.Self[axis])/m_Kdata[axis]);
//			SetCreg(axis,c);
			offset=(axis * 0x1000);
			*((UINT32*)&Dest[offset+0x0034])=c;

			for(cnt=0;cnt<16;cnt++){
				if(cnt<4) c=(g*4)/8;
				else if(cnt<8) c=(g*(cnt+1))/8;
				else if(cnt<13) c=(g*cnt)/8;
				else c=(g*12)/8;

//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(cnt*2*m_ofs_b),(unsigned char)c);
				offset=(axis * 0x1000);
				*((UINT16*)&Dest[(offset)+(cnt*2)])=(UINT8)c;

//				if(m_BoardType==101) _MemReadChar(m_Address); /* MCI-0400 */
//				c>>=8;
//				_MemWriteChar(m_Address+(axis*m_ofs_a)+(((cnt*2)+1)*m_ofs_b),(unsigned char)c);
//				if(m_BoardType==101) _MemReadChar(m_Address); /* MCI-0400 */
			}
			cmd[axis]=0x01;
		}
		else cmd[axis]=0x41;
		if(rev==0) cmd[axis] |= 0x02;
	}
	if(flag==0) return(flag);

//	SetMainAxis(maina);
	SetMainAxis(0,maina, Dest,0x4000 );


	for(axis=0;axis<4;axis++){
		if((flag & (0x01<<axis))==0) continue;
		if(axis!=maina){
			m_MoveFlag[axis]=3;	//Set to SecondaryAxis move
			offset=(axis * 0x1000)+0x100;
//			SetAreg(axis,mainp);
		*((UINT32*)&Dest[offset+0x1C])=mainp ;		//a reg

			cmd[axis] &= 0x0f;	
			cmd[axis] |= 0x80;	//8=1000,10:LinearInterpolationMode 00:Normal Ac/Dc
//			_MemWriteChar(m_Address+(axis*m_ofs_a)+(0x100*m_ofs_b),cmd[axis]);	//CMD1
			*((UINT8*)&Dest[offset+0x00])=cmd[axis] ;		//CMD1

		}
	}
	m_MoveFlag[maina]=2;	//Set to MainAxis move
//	_MemWriteChar(m_Address+(maina*m_ofs_a)+(0x100*m_ofs_b),cmd[maina]);	//CMD1

	offset=(maina * 0x1000)+0x100;
	*((UINT8*)&Dest[offset+0x00])=cmd[maina] ;		//CMD1

	return(flag);
}

