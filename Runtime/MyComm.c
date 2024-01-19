/************************************************************

                MCI0410.c


This file implements all the module entry points: 

int rtIdentify( P_IDENTITY_BLOCK* ppIdentityBlock );
int rtLoad(   UINT32 ScanRate, UINT32* rDirectCalls );
int rtOpen(   LPDRIVER_INST lpNet, P_ERR_PARAM lpErrors);
int rtReload( LPDRIVER_INST lpNet, P_ERR_PARAM lpErrors);
int rtOnLine( LPDRIVER_INST lpNet, P_ERR_PARAM lpErrors);
int rtInput(  LPDRIVER_INST lpNet);
int rtOutput( LPDRIVER_INST lpNet);
int rtSpecial(LPDRIVER_INST lpNet, LPSPECIAL_INST lpData);
int rtOffLine(LPDRIVER_INST lpNet, P_ERR_PARAM  lpErrors);
int rtClose(  LPDRIVER_INST lpNet, P_ERR_PARAM  lpErrors);
int rtUnload( );

**************************************************************/


#include "stdafx.h"


/*********************************************************/
/*                                                       */
/*                MCI0410 Sample Program                */
/*                                                       */
/*********************************************************/ 
                 
#include <rt.h>

#include <stdlib.h>     // _MAX_PATH
#include <string.h>     // strcat()

#include "vlcport.h"
#include "CSFlat.h"     // FCoreSup
#include "DCFlat.h"     // FDriverCore
#include "driver.h"

#include "version.h"
#include "auxrut.h"
#include "MyComm.h"   // DUAL_PORT
#include "task.h"
#include "card.h"
#include "pcistuff.h"

int SpawnLoadIni(LPDRIVER_INST pNet)
{
    int   rc=SUCCESS;
    char  Fname[ _MAX_PATH ];
    char  Pathname[ _MAX_PATH ];

    char  Para1[10];

    char* argv[7];
	Fname[0]=0;

return rc;

// no path
rc = GetPathInfo( ptVcm, pcDir, Pathname, sizeof(Pathname) );

	strcat( Fname, pNet->ConfigFile );
	sprintf(Para1,"%d\0",(pNet->PciIndex -1));
	strcat( Pathname, "LoadInit.exe" );

    argv[0] =Pathname;
	argv[1] =Para1;		// card number
	argv[2] =Fname;		// file name
	argv[3] ="1";
    argv[4] = NULL;



    rc = spawnv( P_WAIT, argv[0], argv );

	pNet->ShareMemPool = rc;	// passing of variable

	if (rc < 1 ) rc= FAILURE;
	else
	rc=SUCCESS;

    return rc;
}



int rtIdentify( P_IDENTITY_BLOCK* ppIdentityBlock )
{
    static IDENTITY_BLOCK IdentityBlock; 
    IdentityBlock.DriverId   = DriverMCI0410;
    IdentityBlock.DriverVers = MCI0410VERS;
    IdentityBlock.pName      = PRODUCT_NAME ", " PRODUCT_VERSION;
    *ppIdentityBlock = &IdentityBlock;
    return 0;
}

int rtLoad( UINT32 ScanRate, UINT32* rDirectCalls )
{
    // Executing the LOAD PROJECT command

    #if defined( _DEBUG )
        SetDebuggingFlag( 1 );  // Disable the VLC watchdog, so we can step through our code. 
    #endif  // _DEBUG


    // Use direct calls for very fast applications.  
    // With the appropriate bit set, Input(), Output() and/or Special()
    //  can be directly called from the engine thread, 
    //  saving the delay introduced by a task switch. 
    // Note:  Functions exectuted in the engine thread cannot call 
    //  some C stdlib APIs, like sprintf(), malloc(), ...
    
    // *rDirectCalls = ( DIRECT_INPUT | DIRECT_OUTPUT | DIRECT_SPECIAL );



    EROP( "rtLoad() ScanRate=%d, rDirectCalls=%x", ScanRate, *rDirectCalls, 0, 0 );

    return 0;
}

int rtOpen( LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
    // Executing the LOAD PROJECT command

    int rc = SUCCESS;
    LPDEVICE_INST pDevice;

    if( pNet->Sentinel != RT3_SENTINEL )
        rc = IDS_VLCRTERR_ALIGNMENT;

    if( rc == SUCCESS )
    {
        UINT32* pSentinel = BuildUiotPointer( pNet->ofsSentinel );
        if( *pSentinel != RT3_SENTINEL )
            rc = IDS_VLCRTERR_ALIGNMENT;
    }

    EROP( "rtOpen() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0 );


if( !pNet->bSimulate )
{

        if( rc == SUCCESS )
			rc = InitPCI( pNet, pErrors );  // Load the physical address of the PCI card in pNet

		
        if( rc == SUCCESS )
        {
            pNet->pDpr = AllocateDpr( pNet->PhyAddr , DPR_TOTAL_SIZE ); // 64K
            if( pNet->pDpr == NULL )
                rc = IDS_VLCRTERR_CREATE_DESCRIPTOR;
        }

		
	if( rc == SUCCESS )
	{
		UINT32	OffsetAxis;

        pNet->pDeviceList = BuildUiotPointer( pNet->ofsDeviceList );

		for( pDevice = pNet->pDeviceList; pDevice->Type && ( rc == SUCCESS ) ; pDevice++ )

		{
			if( pDevice->Sentinel != RT3_SENTINEL )
				rc = IDS_VLCRTERR_ALIGNMENT;
			else
			{
                // Create UIOT pointers
				pDevice->pName = BuildUiotPointer( pDevice->ofsName );

					if (pDevice->Address == CR_AXIS) 
					OffsetAxis = 0x4000;
					else
					OffsetAxis=((pDevice->Address -1)* 0x1000)+0x100;

                if( pDevice->Input.bUsed )
				{
                    pDevice->Input.pDst  = BuildUiotPointer( pDevice->Input.ofsUiot );
                   pDevice->Input.pSrc  = (UINT8*)pNet->pDpr+OffsetAxis;

				}

                if( pDevice->Output.bUsed )
                { 
					pDevice->Output.pDst =  (UINT8*)pNet->pDpr+OffsetAxis;
					pDevice->Output.pSrc = BuildUiotPointer( pDevice->Output.ofsUiot );
				}
			}
		}
	}



	// Load default values into 1240
        if( rc == SUCCESS )
		rc=SpawnLoadIni(pNet);

	// load memory offset in io address




	}



        if( rc == SUCCESS )
            rc = CreateBackgroundTask(pNet);

	

    return rc;
}

int rtReload( LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
    // Executing the LOAD PROJECT command
    EROP( "rtReload() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0);
    if( !pNet->bSimulate )
    {
        InitLinkedList(&pNet->Pend);
        InitLinkedList(&pNet->Done);
    }

    // make sure pNet is in the same state as after rtOpen(). 
    return 0;
}

int rtOnLine( LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
    int	rc = SUCCESS;

	EROP( "rtOnLine() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0 );
    pNet->bFirstCycle = 1;
    pNet->bGoOffLine  = 0;

    if( !pNet->bSimulate )
    {
        /* Check all devices. If critical devices are offline,  rc = IDS_MCI0410_DEVICE_OFFLINE */

        rc = TestConfig( pNet, pErrors);
    }

	EROP( "rtOnLine(). exit.", 0, 0, 0, 0 );

    return rc;

}


int rtInput( LPDRIVER_INST pNet ) 
{

    int     rc       = SUCCESS;

	// EROP( "rtInput() pNet=%p", pNet, 0, 0, 0 );
    // This is the beginning of the VLC scan cycle
    if( !pNet->bSimulate )
    {
		// test get io status
		// Copy new input data from the hw to the driver input image in the UIOT. 
		LPDEVICE_INST pDevice = pNet->pDeviceList;
		for( ; pDevice->Type ; pDevice++ )
		{
			if( pDevice->Input.bUsed )
			{
			// move to uio
            CardCopy( pDevice->Input.pDst, pDevice->Input.pSrc, pDevice->Input.Size);
			}
		}

    VerifyDoneList(&pNet->Done);    // Flush the completed background functions
    }

	EROP( "rtInput(). exit", 0, 0, 0, 0 );

    return SUCCESS;
}


int rtOutput( LPDRIVER_INST pNet)
{
	    int	rc = SUCCESS;

   // This is the end of the VLC scan cycle
    if( !pNet->bSimulate )
    {
        // Copy new output data from the UIOT driver output image to the hw.
		LPDEVICE_INST pDevice = pNet->pDeviceList;
    
		for( ; pDevice->Type ; pDevice++ )
            if( pDevice->Output.bUsed )
		// Update output

//		rc= MCI0410WriteIO(pDevice, pDevice->Output.pSrc, pNet->PhyAddr );
	


        if( pNet->bFirstCycle )     // first Output() ?
        {
            //  Only now we have a valid output image in the DPR. 
            //    EnableOutputs(dp);  enable outputs (if our hardware lets us) 
            
            pNet->bFirstCycle = 0;
        }       
    }

 //   EROP( "rtOutput() pNet=%p", pNet, 0, 0, 0 );

    return SUCCESS;
}

int rtSpecial( LPDRIVER_INST pNet, LPSPECIAL_INST pData)
{
    // A trapeziodal block has been hit, function found in card.c

    UINT16  Result = 0;
    UINT16  Status = VLCFNCSTAT_OK;
    
	// get devicelist
	LPDEVICE_INST pDevice = pNet->pDeviceList;


    EROP( "rtSpecial() pNet=%p, pData=%p", pNet, pData, 0, 0 );

    if( !pNet->bSimulate )
    {
        int  FunctionId = pData->User.paramHeader.FunctionId;
        switch( FunctionId ) 
        {

			case DRVF_SET:
				MCI0410Set( pNet, pData );
				break;       

			case DRVF_SET_CR:
				MCI0410Set( pNet, pData );
				break;       
			case DRVF_DRIVE:
				MCI0410Function( pNet, pData );
				break;       

			case DRVF_VARIABLE:	// for testing, disable after production
				MCI0410Var( pNet, pData );

				break;

         	default:
                    Status = VLCFNCSTAT_WRONGPARAM;
                    break;
        }
    
        EROP("Special();  FunId= %d, Status= %d, pData= %p", FunctionId, Status, pData, 0);
    }
    else
    {
		UINT16* pResult = BuildUiotPointer( pData->User.paramHeader.ofsResult );
        if( pResult )   // some functions may not have the Result param implemented
		    *pResult = (UINT32) SUCCESS;

        Status = VLCFNCSTAT_SIMULATE;
    }

    if( pData->User.paramHeader.ofsStatus )   // some functions may not have the status param implemented
	{
		UINT16* pStatus = BuildUiotPointer( pData->User.paramHeader.ofsStatus );
		*pStatus = Status;
	}
    
    return SUCCESS;
}

int rtOffLine( LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
    // Executing the STOP PROJECT command
    int rc = SUCCESS;

    EROP( "rtOffLine() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0 );

    pNet->bGoOffLine = 1;
    if( !pNet->bSimulate )
    {

		//		SUDD_STOP:
//		outword((UINT16)(pNet->PhyAddr+PORT_WR0),((XYZU_AXIS << 8)+SUDD_STOP));


        rc = WaitForAllFunctionCompletion(pNet);  /* wait for the backgroung task to calm down */
        
        if( rc == SUCCESS )
        {
            /*
            DUAL_PORT far *  dp  = (DUAL_PORT far *)pNet->pDpr;
            if( pNet->StopState == 1 )
                rc = stop scanning;
    
            DisableOutputs(dp, &pNet->trans_count);
            DisableWD(dp); 
            */
        }
        
    }    

    EROP("rtOffLine(). exit  rc= %d", rc, 0, 0, 0);

    return rc;
}

/*   if Open() fails, Close() is not automatically called for this instance.
     if lpErrors == NULL, do not report any error, keep the Open()'s error code and params.  
 */ 
int rtClose( LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
    int rc = SUCCESS;


    // Executing the UNLOAD PROJECT command
    if( !pNet->bSimulate )
    {
        EROP("rtClose(). start. pNet= %p", pNet, 0, 0, 0);

		//		SUDD_STOP:
//		outword((UINT16)(pNet->PhyAddr+PORT_WR0),((XYZU_AXIS<<8)+SUDD_STOP));
//		outword((UINT16)pNet->PhyAddr,0x8000); // Reset motion card


        /*
        {
            DUAL_PORT far* const dp = (DUAL_PORT far *)pNet->pDpr;     / * pointer to the dualport * /
            Reset the board;
        }
        */
        
        //DeleteInterruptTask( pNet );
        DeleteBackgroundTask( pNet );
    
		if( pNet->pDpr )
        {
            FreeDpr( pNet->pDpr );
            pNet->pDpr = NULL;
        }

    }

    EROP( "rtClose() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0 );
    return SUCCESS;
}

int rtUnload()
{
    // Executing the UNLOAD PROJECT command
    EROP( "rtUnload()", 0,0,0,0 );
    return 0;
}




