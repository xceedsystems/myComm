/****************************************************************************

                            Driver.h

    PCI3TIER Driver specfic UIOT structures and adjunct to the network node

*****************************************************************************/



#ifndef __DRIVER_H__
#define __DRIVER_H__

/// Card info
#define VENDOR_ID	0x1453
#define DEVICE_ID	0x0004

// "PCI3TIER" id num. Make sure this id is unique across all VLC C-modules and drivers.
#define DriverMCI0410       0x14530004L

/*
    Version 01.0002
    Bump this version # every time DRIVER_INST, DEVICE_INST or
    DEVICE_IO structs have changed. 
    This will force old projects to be recompiled before execution. 
*/
#define MCI0410VERS         0x0002000aL



//  Network config struct id
#define NETCONFIG_ID        DriverMCI0410


/* 
    Network config struct version
    Bump this version # every time NETCONFIG got new fields. 
    NetPass1() will force drivers in old projects to be reconfigured. 
    If old fields and NETCONFIG size are preserved, 
    configuration in old fields will be kept. 
*/
#define NETCONFIG_VERS      0x00020000L



// Device  config struct id
#define DEVCONFIG_ID        0x12345678L    



/*
    Device  config struct version
    Bump this version # every time DEVCONFIG got new fields. 
    NetPass1() will force devices in old projects to be reconfigured. 
    If old fields and DEVCONFIG size are preserved, 
    configuration in old fields will be kept. 
*/
#define DEVCONFIG_VERS      0x00020000L     


// load this value in DRIVER_INST for rt checking
#define RT3_SENTINEL        0x55667788L


// max 4 networks can be controlled by 1 PC
#define  MAX_DRV_INSTANCES          8       



#include  "errors.h"

#ifndef APSTUDIO_READONLY_SYMBOLS


#ifndef VLCPORT_H__
#include "vlcport.h"
#pragma warning( disable: 4244 )
#endif

#ifndef DATASEG_H__
#include "dataseg.h"
#endif


/*****************************************************************************************

    This file keeps all necessary definitions needed for our pci3tier driver.
    This is a driver build for didactical purposes. It can be used as a starting point 
    when we need to create a new real driver.
    The PCI3TIER driver assumes we have an IO network. 
    The network is controlled by a card and can have up to MAX_DEVICES devices. 
    The card is accesible by a dual port ram and a port address.
    The DRP is DPR_TOTAL_SIZE bytes large and contains a control space, an input space and
    an output space.
    To keep things simple, our devices have all the same size: 8 bytes. 
    They are mapped in the DPR IO spaces based on their network address: device 0's input
    area can be found at the beginning of the DPR input space, device 1's input area is 
    8 bytes farther...  The same mechanism works for devices' output points. 
    In order to see input tags changing, we have to use an external application 
    that writes in the DPR input space.  We can also use the POKE special function to write there. 
    When generating the driver we can change DPR_OUTPUT_OFF to match DPR_INPUT_OFF. 
    Input space will then overlap the output space, so in the VLC project all input tags 
    will be animated by their correspondent output tags.    

    Configuring the driver:
    1.  Choose a DPR address.
    2.  Choose a port address (didactic purpose only: will not be used)
    3.  Choose an interrupt level (didactic purpose only: the interrupt routine does nothing)
    4.  Skip HW tests. We may want to control the thoroughness of initial hw tests.
    5.  Simulate:  if on, there will be no attempt to touch the hardware.
    6.  Watchdog:  if on, the card's watchdog must be cyclicly kicked at run time.
    7.  Cyclic Input Read:  if on,  we update UIOT input image every Input() call.
                            if off, we have to rely on some hardware features telling us if 
                                    any input changed.
    
    Configuring devices:
    1.  Choose a link address (0 ... 127). This determines allocation in the DPR IO space
    2.  Critical:  if on, this device must be active at Online()    
    

    There are 5 different driver models we are studying:
    Model1:     No special functions at all.    (Simulate)
    Model2:     Only sync s.f. No background task.  (PID, Utility, ...)
    Model3:     Sequential async s.f. processing:  Pend & Done lists. (ex. ABKTX, MTL, ...)
                    DRIVER_INST needs  MarkTime.
    Model4:     Simoultaneous async s.f. processing: Pend, Run, Done lists (ex. DHPLUS, ...)
                    The hw supports commands with reply. 
                    New commands can be launched while others are waiting for their replies.
    Model5:     Paralel sequential s.f. processing. 
                    The hw supports a fixed # of channels that can accept commands.
                    Commands executed on different channels can run in paralel.
                    Commands executed on a channel are performed on a FIFO basis.
                    Pend[], Done lists    (DATALOG)
                    DRIVER_INST needs  MarkTime[].
    

    Here is an example for model 3:    
    
*****************************************************************************************/



//************   1 byte alignment typedef structures !!!   ************

#pragma BYTE_ALIGN(_SPECIAL_INST)
typedef  struct _SPECIAL_INST*  LPSPECIAL_INST_;
typedef                 UINT8*  LPUINT8;

typedef  selector   SEMAPHORE;

typedef  UINT32     UIOTREF2CHAR;
typedef  UINT32     UIOTREF2SINT8;
typedef  UINT32     UIOTREF2UINT8;
typedef  UINT32     UIOTREF2SINT16;
typedef  UINT32     UIOTREF2UINT16;
typedef  UINT32     UIOTREF2SINT32;
typedef  UINT32     UIOTREF2UINT32;
typedef  UINT32     UIOTREF2DOUBLE;
typedef  UINT32     UIOTREF2VOID;


#pragma BYTE_ALIGN(_LINKED_LIST)
typedef struct _LINKED_LIST 
{
    LPSPECIAL_INST_     pHead;      // Pointer to the first element in the linked list
    LPSPECIAL_INST_     pTail;      // Pointer to the last element in the linked list
    SEMAPHORE           Semaphore;  // Semaphore that locks the SPECIAL_INST list
    UINT16              uCounter;   // How many items are enqueued here
} LINKED_LIST, *LPLINKED_LIST; 
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_PTBUFFER)
typedef struct _PTBUFFER 
{
    UIOTREF2VOID    Offset;  
    UINT32          Size;           // Use PTBUFFER type for PT_BUFFERs
} PTBUFFER, * LPPTBUFFER;           // Its size is 8 bytes
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_TASK)
typedef struct _TASK 
{
    UINT16      hTask;          // background/interrupt task handle
    SEMAPHORE   Semaphore;      // Where the background task waits
    void*       pStack;         // Pointer to the stack allocated to the task
    UINT16      bBusy;          // True if Special I/O Task is working on packet, used during shutdown
    UINT16      Error;          // error code for the task's init sequence
    void*       IrqThunk;       // pointer to the interrupt routine
    UINT16      level;          // irmx encoded IRQ
    UINT16      align;
} TASK, * LPTASK; 
#pragma BYTE_NORMAL()





#pragma BYTE_ALIGN(_DEVICE_IO)  // 1 byte alignment
typedef struct _DEVICE_IO       // Specifies the UIOT offset and the size for each device
{
    void*        pSrc;          // DPR  offset/pointer for input devices || UIOT offset/pointer for output devices
    void*        pDst;          // UIOT offset/pointer for input devices || DPR  offset/pointer for output devices
    UIOTREF2VOID ofsUiot;       // ofsDst for input devices or ofsSrc for output devices
    UINT16       Size;          // device input or output size.  Never 0 !!!
    UINT16       bUsed;         // If no I/O tags defined in the UIOT, skip it
} DEVICE_IO, *LPDEVICE_IO;            
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_DEVICE_INST)         // 1 byte alignment
typedef struct _DEVICE_INST
{
    UIOTREF2VOID ofsName;       // UIOT offset to the device name generated at compile time
    UINT16       Address;       // device's network address
    UINT16       Type;          // DEVICE_1W_INPUT, ... Never 0 !!!
    UINT16       Gain;	
	UINT16       Bipolar;
    UINT16       bPresent;      // if 1 --> device was online when load and go
    char*        pName;         // Usable UIOT pointer to the device name generated at runtime based on ofsName.
	DEVICE_IO	 Input;			// how to handle the input image
	DEVICE_IO	 Output;		// how to handle the output image
	UINT32       Sentinel;
} DEVICE_INST, *LPDEVICE_INST;
#pragma BYTE_NORMAL()




#pragma BYTE_ALIGN(_DRIVER_INST) 
typedef struct _DRIVER_INST 
{
    NETWORK Net;

        // Compile-time Static variables.  This structure maps to UIOT

    UIOTREF2VOID    ofsDeviceList;      // Where the DEVICE_INST list starts.
    UINT32          nDevices;           // how many Devices are in the list
	UINT16			PciIndex;			// 1...8  which board
	UINT16			PciCount;			// 0...8  how many boards expected in the system. 0--> skip test.
	UINT32          PhyAddr;            // BAR0 read from the PCI card
    UINT16          NumChannels;        // 
    UINT16          bSimulate;          // =0 --> interface card must be present
    
        // Run-time Dynamic Variables
    LPDEVICE_INST   pDeviceList;        // Where the DEVICE_INST list starts.

	void* 			pDpr;               // virtual pointer built based on BAR0
    UINT16          bFirstCycle;        // Set by OnLine(), reset by Output(). Read by Input() and Output()
    UINT16          bGoOffLine;         // Tell all the bkg functions to shutdown

    LINKED_LIST     Pend;               // Pointer to the linked list of pending functions
    LINKED_LIST     Done;               // Pointer to the linked list of done  functions

    TASK            BackgroundTask;     // controls for the background task
    TASK            InterruptTask;      // controls for the interrupt task
	UIOTREF2UINT8   ofsConfigFile;      // Where the config file can be found in the UIOT
    UINT32          szConfigFile;       // How many bytes in the config file
	long			ShareMemPool; 
    char            ConfigFile[64];   // 28  File name for the configuration file


    UIOTREF2UINT32  ofsSentinel;        // 0x55667788 - display this value using Soft Scope to check corrct map
    UINT32          Sentinel;           // 0x55667788 - display this value using Soft Scope to check corrct map

} DRIVER_INST, *LPDRIVER_INST;    
#pragma BYTE_NORMAL() 


#pragma BYTE_ALIGN( _SPECIAL_INST_HEADER )      // Must be first block in all paremeter blocks
typedef struct _SPECIAL_INST_HEADER
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                    // off, sz, ob.sz
    UINT16          FunctionId;     //  0    2   2L   PT_CONST  --> UINT16, _SIZE 2L
    UINT16          align;          //  2    2
    UIOTREF2UINT16  ofsStatus;      //  4    4   2L   PT_REF    --> tag's offset in the UIOT
    UIOTREF2UINT16  ofsResult;      //  8    4   2L   PT_REF    --> tag's offset in the UIOT
} SPECIAL_INST_HEADER;              //      12 == sizeof( SPECIAL_INST_HEADER )
#pragma BYTE_NORMAL()
/*
    Note: beacuse all functions have an Id field and a return status, we can standardize them 
    at offsets 0 and 4. This is especially helpful when using customized parameter structures 
    to better match function particularities and to save memory. 
*/






#pragma BYTE_ALIGN( _SPECIAL_INST_SETGET ) 
typedef struct _SPECIAL_INST_SETGET
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off, sz, ob.sz
    SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
    UINT16              Address;        // 12    2   2L   PT_VALUE, PT_DEVICE --> UINT32
	UINT16				Spare0;			// 14	 2
    UINT16				Function;		// 16    2   2L   PT_REF    --> tag's offset in the UIOT
	UINT16				Spare1;			// 18	 2
	UINT32				Spare2;			// 20	 4

    UINT32				SetValue;			// 24    8   8L   PT_BUFFER --> tag's offset in the UIOT
	UINT32				Spare3;
	UIOTREF2SINT32      GetValue;
	

} SPECIAL_INST_SETGET;                 //     152 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()




#pragma BYTE_ALIGN( _SPECIAL_INST_SET_KDATA ) 
typedef struct _SPECIAL_INST_SET_KDATA
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off, sz, ob.sz
    SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
    UINT16              Address;        // 12    2   2L   PT_VALUE, PT_DEVICE --> UINT32
	UINT16				Spare0;			// 14	 2
    UINT16				Function;		// 16    2   2L   PT_REF    --> tag's offset in the UIOT
	UINT16				Spare1;			// 18	 2
	UINT32				Spare2;			// 20	 4

	SINT16				ValueInt;			// 24	2 	/* move direction 0=for,1=rev */
	UINT16				Spare3;				// 18	 2
	SINT32				ValueLng;			// 28	4	/* move pulse */
	double				ValueFlt;			// 32	8

} SPECIAL_INST_SET_KDATA;                 //     152 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()



#pragma BYTE_ALIGN(_SPECIAL_INST_PORT)  // we may have substitutes for SPECIAL_INST_PARAM
typedef struct _SPECIAL_INST_PORT
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off, sz, ob.sz
    SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
    UINT16              Address;        // 12    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
    UINT16              Length;         // 14    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
    UIOTREF2UINT16      ofsInValue;     // 16    4   2L   PT_REF    --> tag's offset in the UIOT 
    UINT16              OutValue;       // 20    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
    UINT16              align;          // 22    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
} SPECIAL_INST_PORT;                    //      24 == sizeof( SPECIAL_INST_PORT )
#pragma BYTE_NORMAL()

typedef union _SPECIAL_INST_PARAM
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off, sz
    SPECIAL_INST_HEADER  paramHeader;   //  0   12
    SPECIAL_INST_PORT    paramPort;     //  0   24
	SPECIAL_INST_SETGET  paramSetGet;
	SPECIAL_INST_SET_KDATA paraSetKdata;
} SPECIAL_INST_PARAM;                   //      48 == sizeof(SPECIAL_INST_PARAM)


typedef struct _SPECIAL_INST
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off,  sz
    SPECIAL_INST_PARAM  User;           //   0   48
    SPECIAL_INST_PARAM  Work;           //  48   48

        // generic, same for all drivers having asyncronous special functions
    UINT32                MarkTime;     //  96    4  when this s.f. must be complete
    SINT16                Status;       // 100    2
    UINT16                Busy;         // 102    2    
    struct _SPECIAL_INST* pNext;        // 104    4

} SPECIAL_INST, *LPSPECIAL_INST;        //      108 == sizeof( SPECIAL_INST )

/*
Note1: This struct is declared 1 byte aligned on top of file. The struct description is 
       evaluated by the "Runtime" sub-project only.  
       The 'Gui' subproject evaluates the SPECIAL_INST parameter block as presented 
       by the FNC_... definitions. 

Note2: For a very simple function module,  SPECIAL_INST is sufficient.  
       Parameter fields can be described directly in SPECIAL_INST.  
       SPECIAL_INST_PARAM, SPECIAL_INST_PORT, SPECIAL_INST_COMMAND and SPECIAL_INST_HEADER 
       are optional.  They have been defined here only to show a more complex example. 

Note3: In order to save memory SPECIAL_INST can be used only for asynchronous special functions. 
       SPECIAL_INST_COMMAND, SPECIAL_INST_PORT, or even SPECIAL_INST_HEADER 
       will do the job for synchronous special functions. 
       Make sure the correct param block size is declared NET(DEV)_FUNC_TYPE paragraph (p#2).

Note4: Because asynchronous functions are executed concurenlty with the flowchart code, 
       it is safer to provide a copy of the parameter block, to be used by the background thread. 
       This is why we have introduced the 'User' and 'Work' areas. 
       'User' is the area marked by the compiler to be filled in every time a function 
       is called. When the function is posted for execution, 'User' is copied into 'Work' 
       and 'Work' is what the background sees.
       Make sure the fields in 'User' and 'Header' match the FNC_... definitions. 
       It is a good idea to have them both mapped at offset 0.

Note5: The Runtime Special() entry point offers a pointer to the associated SPECIAL_INST. 
       Depending on the FunctionId, the right parameter layout will be selected. 
       This can be implemented in 3 ways: 
       a. Define 1 layout only large enough to encompass all parameters needed by any function. 
       b. Define 1 layout for every function, and cast to the right one based on the FunctionId. 
       c. Define 1 layout for every function, store them into a union and select the right 
          union branch based on the FunctionId. 
       Our current implementation is a mixture of a. and c. and should be optimal 
       for consumed memory and code complexity. 
*/


#ifdef WINVER          // This is for MSVC compiler


#ifndef DRVRUTIL_H__
#include "drvrutil.h"   // SS_ZERO
#endif

// What we put into the database for network config

#pragma BYTE_ALIGN(_NETCONFIG)     // 1 byte alignment
typedef struct _NETCONFIG
{
    UINT32           NetcfgId;          //  0  NETCONFIG_ID
    UINT16           NetcfgVersMinor;   //  4  LOW(  NETCONFIG_VERS )
    UINT16           NetcfgVersMajor;   //  6  HIGH( NETCONFIG_VERS )
    UINT32           PciIndex;          //  8  1...8 max 8 boards in the system
    UINT16           PciCount;          // 12  0...8 if 0--> skip the count test
    UINT16           IrqLevel;          // 14  0 ... 15
    UINT16           BaudRate;          // 16  BAUDRATE_125, BAUDRATE_250, ...
    UINT16			 NumChannels;       // 18  8 or 16 channels

    UINT16           bSimulate;         // 20  =0 --> interface card must be present
    UINT16           bWatchdog;         // 22  =1 --> kick the watchdog
    UINT16           DprHWTests;        // 24  HWTEST_RW, HWTEST_OFF
    UINT16           InputRead;         // 26  INPUT_READ_CYCLIC, INPUT_READ_COS,
    UINT8            ConfigFile[64];   // 28  File name for the configuration file    
    UINT8            Reserved[128-92];  // 28  add new fields without changing NETCONFIG size
} NETCONFIG;                            // 128  == NET_CONFIG_SIZE == sizeof(NETCONFIG)
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_DEVCONFIG)  // 1 byte alignment
typedef struct _DEVCONFIG
{                               // Byte Offset
    UINT32  NetcfgId;           //  0 NETCONFIG_ID
    UINT32  DevcfgId;           //  4 DEVCONFIG_ID
    UINT16  DevcfgVersMinor;    //  8 LOW(  DEVCONFIG_VERS )
    UINT16  DevcfgVersMajor;    // 10 HIGH( DEVCONFIG_VERS )
    UINT16  Address;            // 12 device's address on the link
    UINT16  Reserved0;          // 14 =1 --> this device must be present on the link
	UINT16  Gain;				// 16 Desired gain for input
	UINT16	Bipolar;			// 18 TRUE if this is a bipolar channel
    UINT8   Reserved[128-20];	// 20 add new fields without changing NETCONFIG size
} DEVCONFIG;                    // 128 == DEVCONFIG_SIZE == sizeof(DEVCONFIG)
#pragma BYTE_NORMAL()
/*
    Note: The reserved fields will be used for future developpment. 
    They ensure compatibility with projects generated by older versions of this driver.
*/


#endif      // WINVER


#endif      // ! APSTUDIO_READONLY_SYMBOLS

/* 
    Defines for .rcd file 
    Arithmetic expressions are allowed to define RC and RCD constants, 
    when  ONLY using + and -.  
    It is a good idea to have them encapsulated in ( ).
    Never use * and /.  The RC compiler silently ignores them.
*/


// SPECIAL_INST offsets & sizes
#define FNC_HD_FUNCTIONID           0L 
#define FNC_HD_FUNCTIONID_SIZE          2L      // PT_CONST  --> size 2L    
#define FNC_HD_STATUS               4L 
#define FNC_HD_STATUS_SIZE              2L      // PT_REF --> size of the object pointed to
#define FNC_HD_RESULT               8L 
#define FNC_HD_RESULT_SIZE              2L      // PT_REF --> size of the object pointed to


#define FNC_PO_ADDRESS              12L    
#define FNC_PO_ADDRESS_SIZE             2L      // PT_VALUE --> 2L
#define FNC_PO_LENGTH               14L    
#define FNC_PO_LENGTH_SIZE              2L      // PT_VALUE --> 2L
#define FNC_PO_IN_VALUE             16L 
#define FNC_PO_IN_VALUE_SIZE            2L      // PT_REF --> size of the object pointed to
#define FNC_PO_OUT_VALUE            20L    
#define FNC_PO_OUT_VALUE_SIZE           2L      // PT_VALUE --> 2L


#define FNC_CM_AXIS_NO              12L    
#define FNC_CM_AXIS_NO_SIZE          2L      // PT_VALUE, PT_DEVICE  --> size 4L    
#define FNC_CM_FUN_SEL             16L    
#define FNC_CM_FUN_SEL_SIZE          2L      // PT_REF --> size of the object pointed to

#define FNC_CM_SET             24L    
#define FNC_CM_SET_SIZE         4L      // PT_VALUE  --> size 4L    
#define FNC_CM_GET             32L 
#define FNC_CM_GET_SIZE         4L      // PT_BUFFER --> size 8L


#define FNC_CM_INT             24L    
#define FNC_CM_INT_SIZE         2L      // PT_VALUE  --> size 4L    
#define FNC_CM_LONG            28L    
#define FNC_CM_LONG_SIZE        4L      // PT_VALUE  --> size 4L    
#define FNC_CM_FLT             32L    
#define FNC_CM_FLT_SIZE         8L      // PT_VALUE  --> size 4L    


#define FNC_HD_SPECIAL_INST_SIZE        12
#define FNC_SG_SPECIAL_INST_SIZE		48
#define FNC_PO_SPECIAL_INST_SIZE        24
#define FNC_SPECIAL_INST_SIZE           84


// NETCONFIG offsets & sizes
#define NET_ID                      0 
#define NET_ID_SIZE                     32 
#define NET_VERS                    4 
#define NET_VERS_SIZE                   32 
#define NET_PCI_INDEX               8 
#define NET_PCI_INDEX_SIZE              32 
#define NET_PCI_COUNT               12
#define NET_PCI_COUNT_SIZE              16 
#define NET_NUMCHANNELS             18 
#define NET_NUMCHANNELS_SIZE            16 
#define NET_SIMULATE                20 
#define NET_SIMULATE_SIZE               16 
#define NET_WATCHDOG                22 
#define NET_WATCHDOG_SIZE               16 
#define NET_HWTEST                  24 
#define NET_HWTEST_SIZE                 16 
#define NET_CONFIG_FILE             28 
#define NET_CONFIG_FILE_SIZE			1024
#define NETCONFIG_SIZE              128 

// DEVICECONFIG offsets & sizes
#define DEV_DRVID                   0
#define DEV_DRVID_SIZE                  32
#define DEV_ID                      4
#define DEV_ID_SIZE                     32
#define DEV_VERS                    8
#define DEV_VERS_SIZE                   32
#define DEV_ADDRESS                 12 
#define DEV_ADDRESS_SIZE                16 
#define DEV_GAIN                    16 
#define DEV_GAIN_SIZE                   16 
#define DEV_BIPOLAR                 18 
#define DEV_BIPOLAR_SIZE                16 
#define DEVCONFIG_SIZE              128 


// This is for the demo only.  Replace it with the real settings. 
// Dual port ram layout
//#define  DPR_CONTROL_OFF        0
//#define  DPR_CONTROL_SIZE       2048
//#define  DPR_INPUT_OFF          2048    // where the input image can be found in the dpr
//#define  DPR_INPUT_SIZE         1024    // 1kbyte =  MAX_DEVICES * 8bytes input devices
//#define  DPR_OUTPUT_OFF         3072    // where the output image can be found in the dpr
//#define  DPR_OUTPUT_OFF       2048    // for didactic purposes use 2048 --> outputs will be looped back in inputs
//#define  DPR_OUTPUT_SIZE        1024    // 1kbyte =  MAX_DEVICES * 8bytes input devices
#define  DPR_TOTAL_SIZE         64*1024 // 4 kbytes
#define  MAX_DEVICES            9      // max 16 devices allowed by our didactical network

#define  DEVICE_CARD_AXIS        10

#define  DEVICE_CARD_AXIS_A      11
#define  DEVICE_CARD_AXIS_B      12
#define  DEVICE_CARD_AXIS_C      13
#define  DEVICE_CARD_AXIS_D      14
#define  DEVICE_CARD_CR			 19


#define  DRVF_SET_CR            2100    
#define  DRVF_SET               2101
#define  DRVF_INTERPOL			2102    
#define  DRVF_DRIVE				2103
#define  DRVF_TESTONLY			2110
#define  DRVF_VARIABLE			2111


#define  DEVICE_FUNC                2010    // special device functions ids
#define  DEVF_GET_DEVSTAT           2200    // functions at device level

#define  LIST_SETAXIS_CR			3001
#define  LIST_SETAXIS				3002
#define  LIST_WRITE_CMD				3010
#define  LIST_WRITE_CMD_CR			3020
#define  LIST_DATA_READ				3011
#define  LIST_DRVCMD				3012
#define  LIST_DRVCMD_CR				3022
#define  LIST_VARIABLE				3023
#define  LIST_INTERPOL				3013
#define  LIST_INTERPOL_AXIS			3014

#define	 DO_COLLECT					4000	// does nothing but may for background task

// #define  MAX_LENGTH                 400

///// card thing byte type
   
#define	COMMAND_1				0x1100
#define	COMMAND_2				0x1101
#define	COMMAND_3				0x1102
#define	COMMAND_4				0x1106
#define	COMMAND_5				0x1107
#define	COMMAND_EC1				0x1164
#define	COMMAND_EC2				0x1166
#define	N_REG					0x113C

///// card thing word type
#define	S_REG					0x1224
#define	M_REG					0x1226
#define	G1_REG					0x122C
#define	G2_REG					0x122E
///// card thing 12bit type
#define	P_REG					0x130C
#define	D_REG					0x1314
#define	A_REG					0x131C
#define	C_REG					0x1334
#define	O_REG					0x1344
///// card thing DWORD type
#define	ENC_REG					0x144C
#define	COMMAND_EP0				0x146C
#define	COMMAND_EP1				0x1474
#define	COMMAND_EP2				0x147C
/////////// offset 4000 type
#define	CR_A					0x4100
#define	CR_B					0x4101
#define	CR_C					0x4102
#define	CR_D					0x4103
#define	CR_STOP					0x4104
#define	CR_COMMON				0x4105
#define	CR_INPIT				0x4106
#define	CR_INT					0x4107

///////////function
#define	ES_LOG				0x8000
#define SET_SP_MODE			0x8001
#define RESET_INT			0x8002
#define SET_ALM_LOG			0x8003		
#define SET_NER_LOG			0x8004
#define SET_Z_LOG			0x8005
#define SET_OVER_LOG		0x8006
#define SET_INP_LOG			0x8007
#define SET_PULSE_LOG		0x8008
#define SET_FOR_DIR			0x8009
#define SET_CLK_TYPE		0x800A
#define SET_MOT_TYPE		0x800B
#define SET_ENC_MUL			0x800C
#define SET_HOM_RET			0x800D
#define SET_Z_CNT			0x800E
#define SET_CO				0x800F
#define SET_SRV				0x8010
#define SET_CLR				0x8011
#define SET_ES				0x8012
#define SET_SS				0x8013
#define SET_PULSE			0x8014
#define SET_MAIN_AXIS		0x8015


////internal variable
#define	U_K_DATA			0x9000
#define	U_DIR				0x9001
#define	U_PULSE				0x9002
#define	U_MAX				0x9003
#define	U_SELF				0x9004
#define	U_RAMP				0x9005
#define	U_STYPE				0x9006
#define	U_HOME_MODE			0x9007
#define	U_HOME_TYPE			0x9008
#define	U_INTERPOL			0x9009
//#define	U_HOME_MODE			0x9007



////// axis configuration
#define IPO_AXIS				0x00	

#define A_AXIS                  0x01		//X
#define B_AXIS                  0x02		//Y
#define C_AXIS                  0x03		//Z
#define D_AXIS                  0x04		//U
#define CR_AXIS                 0x09		//U
#define ABCD_AXIS               0x0F

/*
#define AB_AXIS                 0x03
#define AC_AXIS                 0x05
#define AD_AXIS                 0x09
#define BC_AXIS                 0x06
#define BD_AXIS                 0x0A
#define CD_AXIS                 0x0C
#define ABC_AXIS                0x07
#define ABD_AXIS                0x0B
#define ACD_AXIS                0x0D
#define BCD_AXIS                0x0E
#define ABCD_AXIS               0x0F
*/



#endif       // __DRIVER_H__ 




