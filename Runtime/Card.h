/***************************************************************

                Card.h             

   This file contains the interface to the manufacturer code

****************************************************************/


#ifndef  __CARD_H__
#define  __CARD_H__

int Init( void* const dp, P_ERR_PARAM const lpErrors);
int TestConfig( LPDRIVER_INST const pNet, P_ERR_PARAM const lpErrors);
int	DoCollect( LPDRIVER_INST pNet, LPSPECIAL_INST pData);
int MCI0410ReadIO( LPDEVICE_INST const pDevice, VOID *Dest, UINT32 PortAddr);
int MCI0410WriteIO( LPDEVICE_INST const pDevice, VOID *Src, UINT32 PortAddr);


void MCI0410Set(  const LPDRIVER_INST pNet, SPECIAL_INST* const pData );
void MCI0410Function(  const LPDRIVER_INST pNet, SPECIAL_INST* const pData );
void MCI0410Interpol(  const LPDRIVER_INST pNet, SPECIAL_INST* const pData );
void MCI0410Drive(  const LPDRIVER_INST pNet, SPECIAL_INST* const pData );
void MCI0410Test(  const LPDRIVER_INST pNet, SPECIAL_INST* const pData );
void MCI0410Var(  const LPDRIVER_INST pNet, SPECIAL_INST* const pData );


void LoadMCI0410Default(UINT32 PortAddr);

	int ResetInterrupt(int log,int Axis,UINT8* Dest,int OffsetAxis );

	int SetEsLog(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetSpeedMode(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetAlmLog(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetNearLog(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetZLog(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetOverLog(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetInposLog(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetPulseLog(int log,int Axis,UINT8* Dest,int OffsetAxis );
	int SetForDir(int cw,int Axis,UINT8* Dest,int OffsetAxis );
	int SetClockType(int gate,int Axis,UINT8* Dest,int OffsetAxis );
	int SetMotorType(int sv,int Axis,UINT8* Dest,int OffsetAxis );
	int SetEncoderMul(int mul,int Axis,UINT8* Dest,int OffsetAxis );
	int SetHomeRetMode(int on,int Axis,UINT8* Dest,int OffsetAxis );
	int SetZCount(int cnt,int Axis,UINT8* Dest,int OffsetAxis );

	int SetCo(int on,int Axis,UINT8* Dest,int OffsetAxis );
	int SetSrv(int on,int Axis,UINT8* Dest,int OffsetAxis );
	int SetClr(int on,int Axis,UINT8* Dest,int OffsetAxis );

	int SetEs(int on,int Axis,UINT8* Dest,int OffsetAxis );
	int SetSs(int on,int Axis,UINT8* Dest,int OffsetAxis );
	int SetPause(int on,int Axis,UINT8* Dest,int OffsetAxis );
	int SetMainAxis(int on,int Axis,UINT8* Dest,int OffsetAxis );

	int MoveInterporation(int on,int Axis,UINT8* Dest,int OffsetAxis );
	
	void SetPosition(int axis,long pos);


struct syspara{
	int EsLog;		/* 0=NormalOpen,1=NormalClose */
	int SpdMode[4];	/* 0=slow,1=normal,2=fast */
	int AlmLog[4];	/* 0=NormalOpen,1=NormalClose */
	int NearLog[4];
	int ZLog[4];
	int OverLog[4];
	int InpLog[4];
	int PlsLog[4];
	int ForDir[4];	/* 0=ccw,1=cw */
	int ClkType[4];	/* 0=2clk,1=1clk */
	int MotType[4];	/* 0=stepping,1=sarvo */
	int HomeSeq[4];	/* bit1=for/rev,bit0=2/1sensor */
	int HomeMode[4];
	int ZCnt[4];	/* 0-15	*/
	int EncdMul[4];	/* 1,2,4,by */
	int Encoder[4];	/* Encoder installed 0=No,1=Yes */
	int Deviation[4];	/* Position deviation */
	int SType[4];	/* 0=linear,1=Scurve */
	int SCRes[4];	/* 0=16,1=32,2=64,3=128 */
	int MaxSpd[4];	/* 0=65,535 (Stepper),1~25 = 68,265 (Servo) ~ 1,638,375 (Servo) */
	float PlsUnit[4];
};

struct movpara{
	int  Axis;		/* move axis bit0=A,bit1=B,bit2=C,bit3=D */
	int  Dir[4];	/* move direction 0=for,1=rev */
	long Pls[4];	/* move pulse */
	long Max[4];	/* max speed */
	long Self[4];	/* self speed */
	long Ramp[4];	/* acc/dec */
	int Deviation[4];	/* Position deviation */ /// enable for now
};


#endif      /* __CARD_H__ */

