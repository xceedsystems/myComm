#ifndef MCI0410_H
    #define MCI0410_H
/****************************************************************************/
/*  Copyright (c) 2002, Xceed Systems.  All rights reserved.   */
/*                                                                          */
/*  File Name   :   MCI0410.H                                              */
/*  Purpose     :   Header file for MCI0410 Card                           */
/*  Date        :                                                */
/*  Revision    :                                                     */
/*  Programmer  :   ahkait@pacific,net,sg                                   */
/****************************************************************************/

/****************************************************************************/
/*      Typedef  Definitions                                                */
/****************************************************************************/





#define     TRUE                1
#define     FALSE               0

#define     HIGH                1
#define     LOW                 0


///////////////function port from dll

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


	int SetBoardType(char *name);
	int GetBoardType(char *name);
	int SetPciIndex(int ind);
	int SetIsaAdrs(unsigned long adr);
	int SetIsaIrq(int irq);
	int SetPciAdrsIrq(int index);

	int BoardInit();
	void SetInterrupt(void(WINAPI *isr)(void));
	void ResetInterrupt(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );

	void SetEsLog(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetSpeedMode(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetAlmLog(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetNearLog(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetZLog(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetOverLog(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetInposLog(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetPulseLog(UINT32 log,int Axis,UINT8* Dest,int OffsetAxis );
	void SetForDir((UINT32 cw,int Axis,UINT8* Dest,int OffsetAxis );
	void SetClockType(UINT32 gate,int Axis,UINT8* Dest,int OffsetAxis );
	void SetMotorType(UINT32 sv,int Axis,UINT8* Dest,int OffsetAxis );
	void SetEncoderMul(UINT32 mul,int Axis,UINT8* Dest,int OffsetAxis );
	void SetHomeRetMode(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );
	void SetZCount(UINT32 cnt,int Axis,UINT8* Dest,int OffsetAxis );

	void SetCo(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );
	void SetSrv(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );
	void SetClr(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );

	void SetEs(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );
	void SetSs(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );
	void SetPause(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );
	void SetMainAxis(UINT32 on,int Axis,UINT8* Dest,int OffsetAxis );

	/* clr,srv,co  ,  z,in,near,inpos,rev,for,alm,es lsb */
	unsigned int GetIoStatus(int axis);

	unsigned char GetLsiStatus(int axis,int no);
	void SetPreg(int axis,long pls);
	void SetDreg(int axis,long pls);
	void SetAreg(int axis,long pls);
	void SetMreg(int axis,long spd);
	void SetSreg(int axis,long spd);
	void SetGreg(int axis,long spd);
	void SetCreg(int axis,long pls);
	void SetNreg(int axis,unsigned char n);
	void SetEncoder(int axis,long enc);
	long GetEncoder(int axis);
	void SetPosition(int axis,long pos);
	long GetPosition(int axis);
	void ErrorMessage(int err);
//	int CommandRun(char *buf);

	int MoveCheck();
	int MoveIndex(struct movpara *mv);
	int MoveInterporation(struct movpara *mv);
	int MoveNear(struct movpara *mv, int axis, int dir);
	int MoveHome(struct movpara *mv);
	int MoveScan(struct movpara *mv);
	int MoveHomeSq();


#endif
