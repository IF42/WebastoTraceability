/*=============================================================================|
|  PROJECT SNAP7                                                         1.4.0 |
|==============================================================================|
|  Copyright (C) 2013, 2014 Davide Nardella                                    |
|  All rights reserved.                                                        |
|==============================================================================|
|  SNAP7 is free software: you can redistribute it and/or modify               |
|  it under the terms of the Lesser GNU General Public License as published by |
|  the Free Software Foundation, either version 3 of the License, or           |
|  (at your option) any later version.                                         |
|                                                                              |
|  It means that you can distribute your commercial software linked with       |
|  SNAP7 without the requirement to distribute the source code of your         |
|  application and without the requirement that your application be itself     |
|  distributed under LGPL.                                                     |
|                                                                              |
|  SNAP7 is distributed in the hope that it will be useful,                    |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of              |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               |
|  Lesser GNU General Public License for more details.                         |
|                                                                              |
|  You should have received a copy of the GNU General Public License and a     |
|  copy of Lesser GNU General Public License along with Snap7.                 |
|  If not, see  http://www.gnu.org/licenses/                                   |
|==============================================================================|
|                                                                              |
|  C/C++ Snap 7 classes/Imports definitions                                    |
|                                                                              |
|=============================================================================*/
#ifndef snap7_h
#define snap7_h
//---------------------------------------------------------------------------
// Platform detection
//---------------------------------------------------------------------------
#if defined (_WIN32)||defined(_WIN64)||defined(__WIN32__)||defined(__WINDOWS__)
# define OS_WINDOWS
#endif

// Visual Studio needs this to use the correct time_t size
#if defined (_WIN32) && !defined(_WIN64)
# define _USE_32BIT_TIME_T 
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)
# define PLATFORM_UNIX
#endif

#if defined(__SVR4) || defined(__svr4__)
# define OS_SOLARIS
#endif

#if BSD>=0
# define OS_BSD
#endif

#if defined(__APPLE__)
# define OS_OSX
#endif

#if defined(PLATFORM_UNIX) || defined(OS_OSX)
# include <unistd.h>
# if defined(_POSIX_VERSION)
#   define POSIX
# endif
#endif

//---------------------------------------------------------------------------
// C++ Library
//---------------------------------------------------------------------------
#ifdef __cplusplus
#include <string>
#include <time.h>

// Visual C++ not C99 compliant (VS2008--)
#ifdef _MSC_VER
# if _MSC_VER >= 1600
#  include <stdint.h>  // VS2010++ have it 
# else
   typedef signed __int8     int8_t;
   typedef signed __int16    int16_t;
   typedef signed __int32    int32_t;
   typedef signed __int64    int64_t;
   typedef unsigned __int8   uint8_t;
   typedef unsigned __int16  uint16_t;
   typedef unsigned __int32  uint32_t;
   typedef unsigned __int64  uint64_t;
   #ifdef _WIN64
     typedef unsigned __int64  uintptr_t;
   #else
     typedef unsigned __int32  uintptr_t;
   #endif
# endif
#else
# include <stdint.h>
#endif

extern "C" {
#endif
//---------------------------------------------------------------------------
// C exact length types
//---------------------------------------------------------------------------
#ifndef __cplusplus

#ifdef OS_BSD
#  include <stdint.h>
#  include <time.h>
#endif

#ifdef OS_OSX
#  include <stdint.h>  
#  include <time.h>
#endif

#ifdef OS_SOLARIS
#  include <stdint.h>  
#  include <time.h>
#endif

#if defined(_UINTPTR_T_DEFINED)
#  include <stdint.h>
#  include <time.h>
#endif

#if !defined(_UINTPTR_T_DEFINED) && !defined(OS_SOLARIS) && !defined(OS_BSD) && !defined(OS_OSX)
  typedef unsigned char   uint8_t;  //  8 bit unsigned integer
  typedef unsigned short  uint16_t; // 16 bit unsigned integer
  typedef unsigned int    uint32_t; // 32 bit unsigned integer
  typedef unsigned long   uintptr_t;// 64 bit unsigned integer
#endif

#endif

#ifdef OS_WINDOWS
# define S7API __stdcall
#else
# define S7API
#endif

#pragma pack(1)
//******************************************************************************
//                                   COMMON
//******************************************************************************
// Exact length types regardless of platform/processor
typedef uint8_t    byte;
typedef uint16_t   word;
typedef uint32_t   longword;
typedef byte       *pbyte;
typedef word       *pword;
typedef uintptr_t  S7Object; // multi platform/processor object reference
                             // DON'T CONFUSE IT WITH AN OLE OBJECT, IT'S SIMPLY
                             // AN INTEGER VALUE (32 OR 64 BIT) USED AS HANDLE.

#ifndef __cplusplus
typedef struct
{
  int   tm_sec;
  int   tm_min;
  int   tm_hour;
  int   tm_mday;
  int   tm_mon;
  int   tm_year;
  int   tm_wday;
  int   tm_yday;
  int   tm_isdst;
}tm;

#ifndef bool
typedef int bool;
#define false 0;
#define true  1;
#endif
#endif

#define errLibInvalidParam  = -1;
#define errLibInvalidObject = -2;

// CPU status
#define S7CpuStatusUnknown  0x00
#define S7CpuStatusRun      0x08
#define S7CpuStatusStop     0x04

// ISO Errors
#define errIsoConnect            = 0x00010000; // Connection error
#define errIsoDisconnect         = 0x00020000; // Disconnect error
#define errIsoInvalidPDU         = 0x00030000; // Bad format
#define errIsoInvalidDataSize    = 0x00040000; // Bad Datasize passed to send/recv buffer is invalid
#define errIsoNullPointer    	= 0x00050000; // Null passed as pointer
#define errIsoShortPacket    	= 0x00060000; // A short packet received
#define errIsoTooManyFragments   = 0x00070000; // Too many packets without EoT flag
#define errIsoPduOverflow    	= 0x00080000; // The sum of fragments data exceded maximum packet size
#define errIsoSendPacket         = 0x00090000; // An error occurred during send
#define errIsoRecvPacket         = 0x000A0000; // An error occurred during recv
#define errIsoInvalidParams    	= 0x000B0000; // Invalid TSAP params
#define errIsoResvd_1            = 0x000C0000; // Unassigned
#define errIsoResvd_2            = 0x000D0000; // Unassigned
#define errIsoResvd_3            = 0x000E0000; // Unassigned
#define errIsoResvd_4            = 0x000F0000; // Unassigned

// Tag Struct
typedef struct{
	int Area;
	int DBNumber;
	int Start;
	int Size;
	int WordLen;
}TS7Tag, *PS7Tag;

//------------------------------------------------------------------------------
//                                  PARAMS LIST            
//------------------------------------------------------------------------------
#define p_u16_LocalPort  	    = 1;
#define p_u16_RemotePort 	    = 2;
#define p_i32_PingTimeout	    = 3;
#define p_i32_SendTimeout     = 4;
#define p_i32_RecvTimeout     = 5;
#define p_i32_WorkInterval    = 6;
#define p_u16_SrcRef          = 7;
#define p_u16_DstRef          = 8;
#define p_u16_SrcTSap         = 9;
#define p_i32_PDURequest      = 10;
#define p_i32_MaxClients      = 11;
#define p_i32_BSendTimeout    = 12;
#define p_i32_BRecvTimeout    = 13;
#define p_u32_RecoveryTime    = 14;
#define p_u32_KeepAliveTime   = 15;

// Client/Partner Job status 
#define JobComplete           = 0;
#define JobPending            = 1;

//******************************************************************************
//                                   CLIENT
//******************************************************************************

// Error codes
#define errNegotiatingPDU            = 0x00100000;
#define errCliInvalidParams          = 0x00200000;
#define errCliJobPending             = 0x00300000;
#define errCliTooManyItems           = 0x00400000;
#define errCliInvalidWordLen         = 0x00500000;
#define errCliPartialDataWritten     = 0x00600000;
#define errCliSizeOverPDU            = 0x00700000;
#define errCliInvalidPlcAnswer       = 0x00800000;
#define errCliAddressOutOfRange      = 0x00900000;
#define errCliInvalidTransportSize   = 0x00A00000;
#define errCliWriteDataSizeMismatch  = 0x00B00000;
#define errCliItemNotAvailable       = 0x00C00000;
#define errCliInvalidValue           = 0x00D00000;
#define errCliCannotStartPLC         = 0x00E00000;
#define errCliAlreadyRun             = 0x00F00000;
#define errCliCannotStopPLC          = 0x01000000;
#define errCliCannotCopyRamToRom     = 0x01100000;
#define errCliCannotCompress         = 0x01200000;
#define errCliAlreadyStop            = 0x01300000;
#define errCliFunNotAvailable        = 0x01400000;
#define errCliUploadSequenceFailed   = 0x01500000;
#define errCliInvalidDataSizeRecvd   = 0x01600000;
#define errCliInvalidBlockType       = 0x01700000;
#define errCliInvalidBlockNumber     = 0x01800000;
#define errCliInvalidBlockSize       = 0x01900000;
#define errCliDownloadSequenceFailed = 0x01A00000;
#define errCliInsertRefused          = 0x01B00000;
#define errCliDeleteRefused          = 0x01C00000;
#define errCliNeedPassword           = 0x01D00000;
#define errCliInvalidPassword        = 0x01E00000;
#define errCliNoPasswordToSetOrClear = 0x01F00000;
#define errCliJobTimeout             = 0x02000000;
#define errCliPartialDataRead        = 0x02100000;
#define errCliBufferTooSmall         = 0x02200000;
#define errCliFunctionRefused        = 0x02300000;
#define errCliDestroying             = 0x02400000;
#define errCliInvalidParamNumber     = 0x02500000;
#define errCliCannotChangeParam      = 0x02600000;

#define MaxVars     = 20; // Max vars that can be transferred with MultiRead/MultiWrite

// Client Connection Type
#define CONNTYPE_PG                      = 0x0001;  // Connect to the PLC as a PG
#define CONNTYPE_OP                      = 0x0002;  // Connect to the PLC as an OP
#define CONNTYPE_BASIC                   = 0x0003;  // Basic connection

// Area ID
#define S7AreaPE   =	0x81;
#define S7AreaPA   =	0x82;
#define S7AreaMK   =	0x83;
#define S7AreaDB   =	0x84;
#define S7AreaCT   =	0x1C;
#define S7AreaTM   =	0x1D;

// Word Length
#define S7WLBit     = 0x01;
#define S7WLByte    = 0x02;
#define S7WLWord    = 0x04;
#define S7WLDWord   = 0x06;
#define S7WLReal    = 0x08;
#define S7WLCounter = 0x1C;
#define S7WLTimer   = 0x1D;

// Block type
#define Block_OB   = 0x38;
#define Block_DB   = 0x41;
#define Block_SDB  = 0x42;
#define Block_FC   = 0x43;
#define Block_SFC  = 0x44;
#define Block_FB   = 0x45;
#define Block_SFB  = 0x46;

// Sub Block Type
#define SubBlk_OB  = 0x08;
#define SubBlk_DB  = 0x0A;
#define SubBlk_SDB = 0x0B;
#define SubBlk_FC  = 0x0C;
#define SubBlk_SFC = 0x0D;
#define SubBlk_FB  = 0x0E;
#define SubBlk_SFB = 0x0F;

// Block languages
#define BlockLangAWL       = 0x01;
#define BlockLangKOP       = 0x02;
#define BlockLangFUP       = 0x03;
#define BlockLangSCL       = 0x04;
#define BlockLangDB        = 0x05;
#define BlockLangGRAPH     = 0x06;

// Read/Write Multivars
typedef struct{
   int   Area;
   int   WordLen;
   int   Result;
   int   DBNumber;
   int   Start;
   int   Amount;
   void  *pdata;
} TS7DataItem, *PS7DataItem;

//typedef int TS7ResultItems[MaxVars];
//typedef TS7ResultItems *PS7ResultItems;

// List Blocks
typedef struct {
   int OBCount;
   int FBCount;
   int FCCount;
   int SFBCount;
   int SFCCount;
   int DBCount;
   int SDBCount;
} TS7BlocksList, *PS7BlocksList;

// Blocks info
typedef struct {
   int BlkType;    // Block Type (OB, DB) 
   int BlkNumber;  // Block number
   int BlkLang;    // Block Language
   int BlkFlags;   // Block flags
   int MC7Size;    // The real size in bytes
   int LoadSize;   // Load memory size
   int LocalData;  // Local data
   int SBBLength;  // SBB Length
   int CheckSum;   // Checksum
   int Version;    // Block version
   // Chars info
   char CodeDate[11]; // Code date
   char IntfDate[11]; // Interface date 
   char Author[9];    // Author
   char Family[9];    // Family
   char Header[9];    // Header
} TS7BlockInfo, *PS7BlockInfo ;

typedef word TS7BlocksOfType[0x2000];
typedef TS7BlocksOfType *PS7BlocksOfType;

// Order code
typedef struct {
   char Code[21];
   byte V1;
   byte V2;
   byte V3;
} TS7OrderCode, *PS7OrderCode;

// CPU Info
typedef struct {
   char ModuleTypeName[33];
   char SerialNumber[25];
   char ASName[25];
   char Copyright[27];
   char ModuleName[25];
} TS7CpuInfo, *PS7CpuInfo;

// CP Info
typedef struct {
   int MaxPduLengt;
   int MaxConnections;
   int MaxMpiRate;
   int MaxBusRate;
} TS7CpInfo, *PS7CpInfo;

// See §33.1 of "System Software for S7-300/400 System and Standard Functions"
// and see SFC51 description too
typedef struct {
   word LENTHDR;
   word N_DR;
} SZL_HEADER, *PSZL_HEADER;

typedef struct {
   SZL_HEADER Header;
   byte Data[0x4000-4];
} TS7SZL, *PS7SZL;

// SZL List of available SZL IDs : same as SZL but List items are big-endian adjusted
typedef struct {
   SZL_HEADER Header;
   word List[0x2000-2];
} TS7SZLList, *PS7SZLList;

// See §33.19 of "System Software for S7-300/400 System and Standard Functions"
typedef struct {
   word  sch_schal;
   word  sch_par;
   word  sch_rel;
   word  bart_sch;
   word  anl_sch;
} TS7Protection, *PS7Protection;

// Client completion callback
typedef void (S7API *pfn_CliCompletion) (void *usrPtr, int opCode, int opResult);
//------------------------------------------------------------------------------
//  Import prototypes
//------------------------------------------------------------------------------
S7Object S7API Cli_Create();
void S7API Cli_Destroy(S7Object *Client);
int S7API Cli_ConnectTo(S7Object Client, const char *Address, int Rack, int Slot);
int S7API Cli_SetConnectionParams(S7Object Client, const char *Address, word LocalTSAP, word RemoteTSAP);
int S7API Cli_SetConnectionType(S7Object Client, word ConnectionType);
int S7API Cli_Connect(S7Object Client);
int S7API Cli_Disconnect(S7Object Client);
int S7API Cli_GetParam(S7Object Client, int ParamNumber, void *pValue);
int S7API Cli_SetParam(S7Object Client, int ParamNumber, void *pValue);
int S7API Cli_SetAsCallback(S7Object Client, pfn_CliCompletion pCompletion, void *usrPtr);
// Data I/O main functions
int S7API Cli_ReadArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
int S7API Cli_WriteArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
int S7API Cli_ReadMultiVars(S7Object Client, PS7DataItem Item, int ItemsCount);
int S7API Cli_WriteMultiVars(S7Object Client, PS7DataItem Item, int ItemsCount);
// Data I/O Lean functions
int S7API Cli_DBRead(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
int S7API Cli_DBWrite(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
int S7API Cli_MBRead(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_MBWrite(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_EBRead(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_EBWrite(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_ABRead(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_ABWrite(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_TMRead(S7Object Client, int Start, int Amount, void *pUsrData);
int S7API Cli_TMWrite(S7Object Client, int Start, int Amount, void *pUsrData);
int S7API Cli_CTRead(S7Object Client, int Start, int Amount, void *pUsrData);
int S7API Cli_CTWrite(S7Object Client, int Start, int Amount, void *pUsrData);
// Directory functions
int S7API Cli_ListBlocks(S7Object Client, TS7BlocksList *pUsrData);
int S7API Cli_GetAgBlockInfo(S7Object Client, int BlockType, int BlockNum, TS7BlockInfo *pUsrData);
int S7API Cli_GetPgBlockInfo(S7Object Client, void *pBlock, TS7BlockInfo *pUsrData, int Size);
int S7API Cli_ListBlocksOfType(S7Object Client, int BlockType, TS7BlocksOfType *pUsrData, int *ItemsCount);
// Blocks functions
int S7API Cli_Upload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int *Size);
int S7API Cli_FullUpload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int *Size);
int S7API Cli_Download(S7Object Client, int BlockNum, void *pUsrData, int Size);
int S7API Cli_Delete(S7Object Client, int BlockType, int BlockNum);
int S7API Cli_DBGet(S7Object Client, int DBNumber, void *pUsrData, int *Size);
int S7API Cli_DBFill(S7Object Client, int DBNumber, int FillChar);
// Date/Time functions
int S7API Cli_GetPlcDateTime(S7Object Client, tm *DateTime);
int S7API Cli_SetPlcDateTime(S7Object Client, tm *DateTime);
int S7API Cli_SetPlcSystemDateTime(S7Object Client);
// System Info functions
int S7API Cli_GetOrderCode(S7Object Client, TS7OrderCode *pUsrData);
int S7API Cli_GetCpuInfo(S7Object Client, TS7CpuInfo *pUsrData);
int S7API Cli_GetCpInfo(S7Object Client, TS7CpInfo *pUsrData);
int S7API Cli_ReadSZL(S7Object Client, int ID, int Index, TS7SZL *pUsrData, int *Size);
int S7API Cli_ReadSZLList(S7Object Client, TS7SZLList *pUsrData, int *ItemsCount);
// Control functions
int S7API Cli_PlcHotStart(S7Object Client);
int S7API Cli_PlcColdStart(S7Object Client);
int S7API Cli_PlcStop(S7Object Client);
int S7API Cli_CopyRamToRom(S7Object Client, int Timeout);
int S7API Cli_Compress(S7Object Client, int Timeout);
int S7API Cli_GetPlcStatus(S7Object Client, int *Status);
// Security functions
int S7API Cli_GetProtection(S7Object Client, TS7Protection *pUsrData);
int S7API Cli_SetSessionPassword(S7Object Client, char *Password);
int S7API Cli_ClearSessionPassword(S7Object Client);
// Low level
int S7API Cli_IsoExchangeBuffer(S7Object Client, void *pUsrData, int *Size);
// Misc
int S7API Cli_GetExecTime(S7Object Client, int *Time);
int S7API Cli_GetLastError(S7Object Client, int *LastError);
int S7API Cli_GetPduLength(S7Object Client, int *Requested, int *Negotiated);
int S7API Cli_ErrorText(int Error, char *Text, int TextLen);
// 1.1.0
int S7API Cli_GetConnected(S7Object Client, int *Connected);
//------------------------------------------------------------------------------
//  Async functions
//------------------------------------------------------------------------------
int S7API Cli_AsReadArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
int S7API Cli_AsWriteArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
int S7API Cli_AsDBRead(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
int S7API Cli_AsDBWrite(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
int S7API Cli_AsMBRead(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_AsMBWrite(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_AsEBRead(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_AsEBWrite(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_AsABRead(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_AsABWrite(S7Object Client, int Start, int Size, void *pUsrData);
int S7API Cli_AsTMRead(S7Object Client, int Start, int Amount, void *pUsrData);
int S7API Cli_AsTMWrite(S7Object Client, int Start, int Amount, void *pUsrData);
int S7API Cli_AsCTRead(S7Object Client, int Start, int Amount, void *pUsrData);
int S7API Cli_AsCTWrite(S7Object Client, int Start, int Amount, void *pUsrData);
int S7API Cli_AsListBlocksOfType(S7Object Client, int BlockType, TS7BlocksOfType *pUsrData, int *ItemsCount);
int S7API Cli_AsReadSZL(S7Object Client, int ID, int Index, TS7SZL *pUsrData, int *Size);
int S7API Cli_AsReadSZLList(S7Object Client, TS7SZLList *pUsrData, int *ItemsCount);
int S7API Cli_AsUpload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int *Size);
int S7API Cli_AsFullUpload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int *Size);
int S7API Cli_AsDownload(S7Object Client, int BlockNum, void *pUsrData, int Size);
int S7API Cli_AsCopyRamToRom(S7Object Client, int Timeout);
int S7API Cli_AsCompress(S7Object Client, int Timeout);
int S7API Cli_AsDBGet(S7Object Client, int DBNumber, void *pUsrData, int *Size);
int S7API Cli_AsDBFill(S7Object Client, int DBNumber, int FillChar);
int S7API Cli_CheckAsCompletion(S7Object Client, int *opResult);
int S7API Cli_WaitAsCompletion(S7Object Client, int Timeout);

//******************************************************************************
//                                   SERVER
//******************************************************************************
#define OperationRead  = 0;
#define OperationWrite = 1;

#define mkEvent = 0;
#define mkLog   = 1;

// Server Area ID  (use with Register/unregister - Lock/unlock Area)
#define srvAreaPE = 0;
#define srvAreaPA = 1;
#define srvAreaMK = 2;
#define srvAreaCT = 3;
#define srvAreaTM = 4;
#define srvAreaDB = 5;

// Errors
#define errSrvCannotStart        = 0x00100000; // Server cannot start
#define errSrvDBNullPointer      = 0x00200000; // Passed null as PData
#define errSrvAreaAlreadyExists  = 0x00300000; // Area Re-registration
#define errSrvUnknownArea        = 0x00400000; // Unknown area
#define errSrvInvalidParams      = 0x00500000; // Invalid param(s) supplied
#define errSrvTooManyDB          = 0x00600000; // Cannot register DB
#define errSrvInvalidParamNumber = 0x00700000; // Invalid param (srv_get/set_param)
#define errSrvCannotChangeParam  = 0x00800000; // Cannot change because running

// TCP Server Event codes
#define evcServerStarted       = 0x00000001;
#define evcServerStopped       = 0x00000002;
#define evcListenerCannotStart = 0x00000004;
#define evcClientAdded         = 0x00000008;
#define evcClientRejected      = 0x00000010;
#define evcClientNoRoom        = 0x00000020;
#define evcClientException     = 0x00000040;
#define evcClientDisconnected  = 0x00000080;
#define evcClientTerminated    = 0x00000100;
#define evcClientsDropped      = 0x00000200;
#define evcReserved_00000400   = 0x00000400; // actually unused
#define evcReserved_00000800   = 0x00000800; // actually unused
#define evcReserved_00001000   = 0x00001000; // actually unused
#define evcReserved_00002000   = 0x00002000; // actually unused
#define evcReserved_00004000   = 0x00004000; // actually unused
#define evcReserved_00008000   = 0x00008000; // actually unused
// S7 Server Event Code
#define evcPDUincoming  	      = 0x00010000;
#define evcDataRead            = 0x00020000;
#define evcDataWrite    	      = 0x00040000;
#define evcNegotiatePDU        = 0x00080000;
#define evcReadSZL             = 0x00100000;
#define evcClock               = 0x00200000;
#define evcUpload              = 0x00400000;
#define evcDownload            = 0x00800000;
#define evcDirectory           = 0x01000000;
#define evcSecurity            = 0x02000000;
#define evcControl             = 0x04000000;
#define evcReserved_08000000   = 0x08000000; // actually unused
#define evcReserved_10000000   = 0x10000000; // actually unused
#define evcReserved_20000000   = 0x20000000; // actually unused
#define evcReserved_40000000   = 0x40000000; // actually unused
#define evcReserved_80000000   = 0x80000000; // actually unused
// Masks to enable/disable all events
#define evcAll                 = 0xFFFFFFFF;
#define evcNone                = 0x00000000;
// Event SubCodes
#define evsUnknown                 = 0x0000;
#define evsStartUpload             = 0x0001;
#define evsStartDownload           = 0x0001;
#define evsGetBlockList            = 0x0001;
#define evsStartListBoT            = 0x0002;
#define evsListBoT                 = 0x0003;
#define evsGetBlockInfo            = 0x0004;
#define evsGetClock                = 0x0001;
#define evsSetClock                = 0x0002;
#define evsSetPassword             = 0x0001;
#define evsClrPassword             = 0x0002;
// Event Params : functions group
#define grProgrammer               = 0x0041;
#define grCyclicData               = 0x0042;
#define grBlocksInfo               = 0x0043;
#define grSZL                      = 0x0044;
#define grPassword                 = 0x0045;
#define grBSend                    = 0x0046;
#define grClock                    = 0x0047;
#define grSecurity                 = 0x0045;
// Event Params : control codes
#define CodeControlUnknown         = 0x0000;
#define CodeControlColdStart       = 0x0001;
#define CodeControlWarmStart       = 0x0002;
#define CodeControlStop            = 0x0003;
#define CodeControlCompress        = 0x0004;
#define CodeControlCpyRamRom       = 0x0005;
#define CodeControlInsDel          = 0x0006;
// Event Result
#define evrNoError                 = 0x0000;
#define evrFragmentRejected        = 0x0001;
#define evrMalformedPDU            = 0x0002;
#define evrSparseBytes             = 0x0003;
#define evrCannotHandlePDU         = 0x0004;
#define evrNotImplemented          = 0x0005;
#define evrErrException            = 0x0006;
#define evrErrAreaNotFound         = 0x0007;
#define evrErrOutOfRange           = 0x0008;
#define evrErrOverPDU              = 0x0009;
#define evrErrTransportSize        = 0x000A;
#define evrInvalidGroupUData       = 0x000B;
#define evrInvalidSZL              = 0x000C;
#define evrDataSizeMismatch        = 0x000D;
#define evrCannotUpload            = 0x000E;
#define evrCannotDownload          = 0x000F;
#define evrUploadInvalidID         = 0x0010;
#define evrResNotFound             = 0x0011;

typedef struct{
	time_t EvtTime;    // Timestamp
	int EvtSender;     // Sender
	longword EvtCode;  // Event code
	word EvtRetCode;   // Event result
	word EvtParam1;    // Param 1 (if available)
	word EvtParam2;    // Param 2 (if available)
	word EvtParam3;    // Param 3 (if available)
	word EvtParam4;    // Param 4 (if available)
}TSrvEvent, *PSrvEvent;

// Server Events callback
typedef void (S7API *pfn_SrvCallBack)(void *usrPtr, PSrvEvent PEvent, int Size);
// Server Read/Write callback
typedef int(S7API *pfn_RWAreaCallBack)(void *usrPtr, int Sender, int Operation, PS7Tag PTag, void *pUsrData);

S7Object S7API Srv_Create();
void S7API Srv_Destroy(S7Object *Server);
int S7API Srv_GetParam(S7Object Server, int ParamNumber, void *pValue);
int S7API Srv_SetParam(S7Object Server, int ParamNumber, void *pValue);
int S7API Srv_StartTo(S7Object Server, const char *Address);
int S7API Srv_Start(S7Object Server);
int S7API Srv_Stop(S7Object Server);
int S7API Srv_RegisterArea(S7Object Server, int AreaCode, word Index, void *pUsrData, int Size);
int S7API Srv_UnregisterArea(S7Object Server, int AreaCode, word Index);
int S7API Srv_LockArea(S7Object Server, int AreaCode, word Index);
int S7API Srv_UnlockArea(S7Object Server, int AreaCode, word Index);
int S7API Srv_GetStatus(S7Object Server, int *ServerStatus, int *CpuStatus, int *ClientsCount);
int S7API Srv_SetCpuStatus(S7Object Server, int CpuStatus);
int S7API Srv_ClearEvents(S7Object Server);
int S7API Srv_PickEvent(S7Object Server, TSrvEvent *pEvent, int *EvtReady);
int S7API Srv_GetMask(S7Object Server, int MaskKind, longword *Mask);
int S7API Srv_SetMask(S7Object Server, int MaskKind, longword Mask);
int S7API Srv_SetEventsCallback(S7Object Server, pfn_SrvCallBack pCallback, void *usrPtr);
int S7API Srv_SetReadEventsCallback(S7Object Server, pfn_SrvCallBack pCallback, void *usrPtr);
int S7API Srv_SetRWAreaCallback(S7Object Server, pfn_RWAreaCallBack pCallback, void *usrPtr);
int S7API Srv_EventText(TSrvEvent *Event, char *Text, int TextLen);
int S7API Srv_ErrorText(int Error, char *Text, int TextLen);

//******************************************************************************
//                                   PARTNER
//******************************************************************************

// Status
#define par_stopped         = 0;   // stopped
#define par_connecting      = 1;   // running and active connecting
#define par_waiting         = 2;   // running and waiting for a connection
#define par_linked          = 3;   // running and connected : linked
#define par_sending         = 4;   // sending data
#define par_receiving       = 5;   // receiving data
#define par_binderror       = 6;   // error starting passive server

// Errors
#define errParAddressInUse       = 0x00200000;
#define errParNoRoom             = 0x00300000;
#define errServerNoRoom          = 0x00400000;
#define errParInvalidParams      = 0x00500000;
#define errParNotLinked          = 0x00600000;
#define errParBusy               = 0x00700000;
#define errParFrameTimeout       = 0x00800000;
#define errParInvalidPDU         = 0x00900000;
#define errParSendTimeout        = 0x00A00000;
#define errParRecvTimeout        = 0x00B00000;
#define errParSendRefused        = 0x00C00000;
#define errParNegotiatingPDU     = 0x00D00000;
#define errParSendingBlock       = 0x00E00000;
#define errParRecvingBlock       = 0x00F00000;
#define errParBindError          = 0x01000000;
#define errParDestroying         = 0x01100000;
#define errParInvalidParamNumber = 0x01200000; // Invalid param (par_get/set_param)
#define errParCannotChangeParam  = 0x01300000; // Cannot change because running
#define errParBufferTooSmall     = 0x01400000; // Raised by LabVIEW wrapper

// Brecv Data incoming Callback
typedef void (S7API *pfn_ParRecvCallBack)(void * usrPtr, int opResult, longword R_ID, void *pData, int Size);
// BSend Completion Callback
typedef void (S7API *pfn_ParSendCompletion)(void * usrPtr, int opResult);

S7Object S7API Par_Create(int Active);
void S7API Par_Destroy(S7Object *Partner);
int S7API Par_GetParam(S7Object Partner, int ParamNumber, void *pValue);
int S7API Par_SetParam(S7Object Partner, int ParamNumber, void *pValue);
int S7API Par_StartTo(S7Object Partner, const char *LocalAddress, const char *RemoteAddress,
    word LocTsap, word RemTsap);
int S7API Par_Start(S7Object Partner);
int S7API Par_Stop(S7Object Partner);
// BSend
int S7API Par_BSend(S7Object Partner, longword R_ID, void *pUsrData, int Size);
int S7API Par_AsBSend(S7Object Partner, longword R_ID, void *pUsrData, int Size);
int S7API Par_CheckAsBSendCompletion(S7Object Partner, int *opResult);
int S7API Par_WaitAsBSendCompletion(S7Object Partner, longword Timeout);
int S7API Par_SetSendCallback(S7Object Partner, pfn_ParSendCompletion pCompletion, void *usrPtr);
// BRecv
int S7API Par_BRecv(S7Object Partner, longword *R_ID, void *pData, int *Size, longword Timeout);
int S7API Par_CheckAsBRecvCompletion(S7Object Partner, int *opResult, longword *R_ID,
	void *pData, int *Size);
int S7API Par_SetRecvCallback(S7Object Partner, pfn_ParRecvCallBack pCompletion, void *usrPtr);
// Stat
int S7API Par_GetTimes(S7Object Partner, longword *SendTime, longword *RecvTime);
int S7API Par_GetStats(S7Object Partner, longword *BytesSent, longword *BytesRecv,
	longword *SendErrors, longword *RecvErrors);
int S7API Par_GetLastError(S7Object Partner, int *LastError);
int S7API Par_GetStatus(S7Object Partner, int *Status);
int S7API Par_ErrorText(int Error, char *Text, int TextLen);


#pragma pack()
#ifdef __cplusplus
 }
#endif // __cplusplus

#ifdef __cplusplus

//******************************************************************************
//                           CLIENT CLASS DEFINITION
//******************************************************************************
class TS7Client
{
private:
    S7Object Client;
public:
	TS7Client();
	~TS7Client();
    // Control functions
    int Connect();
    int ConnectTo(const char *RemAddress, int Rack, int Slot);
    int SetConnectionParams(const char *RemAddress, word LocalTSAP, word RemoteTSAP);
    int SetConnectionType(word ConnectionType);
    int Disconnect();
    int GetParam(int ParamNumber, void *pValue);
    int SetParam(int ParamNumber, void *pValue);
    // Data I/O Main functions
    int ReadArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
    int WriteArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
    int ReadMultiVars(PS7DataItem Item, int ItemsCount);
    int WriteMultiVars(PS7DataItem Item, int ItemsCount);
    // Data I/O Lean functions
    int DBRead(int DBNumber, int Start, int Size, void *pUsrData);
    int DBWrite(int DBNumber, int Start, int Size, void *pUsrData);
    int MBRead(int Start, int Size, void *pUsrData);
    int MBWrite(int Start, int Size, void *pUsrData);
    int EBRead(int Start, int Size, void *pUsrData);
    int EBWrite(int Start, int Size, void *pUsrData);
    int ABRead(int Start, int Size, void *pUsrData);
    int ABWrite(int Start, int Size, void *pUsrData);
    int TMRead(int Start, int Amount, void *pUsrData);
    int TMWrite(int Start, int Amount, void *pUsrData);
    int CTRead(int Start, int Amount, void *pUsrData);
    int CTWrite(int Start, int Amount, void *pUsrData);
    // Directory functions
    int ListBlocks(PS7BlocksList pUsrData);
    int GetAgBlockInfo(int BlockType, int BlockNum, PS7BlockInfo pUsrData);
    int GetPgBlockInfo(void *pBlock, PS7BlockInfo pUsrData, int Size);
    int ListBlocksOfType(int BlockType, TS7BlocksOfType *pUsrData, int *ItemsCount);
    // Blocks functions
    int Upload(int BlockType, int BlockNum, void *pUsrData, int *Size);
    int FullUpload(int BlockType, int BlockNum, void *pUsrData, int *Size);
    int Download(int BlockNum, void *pUsrData, int Size);
    int Delete(int BlockType, int BlockNum);
    int DBGet(int DBNumber, void *pUsrData, int *Size);
    int DBFill(int DBNumber, int FillChar);
    // Date/Time functions
    int GetPlcDateTime(tm *DateTime);
    int SetPlcDateTime(tm *DateTime);
    int SetPlcSystemDateTime();
    // System Info functions
    int GetOrderCode(PS7OrderCode pUsrData);
    int GetCpuInfo(PS7CpuInfo pUsrData);
    int GetCpInfo(PS7CpInfo pUsrData);
	int ReadSZL(int ID, int Index, PS7SZL pUsrData, int *Size);
	int ReadSZLList(PS7SZLList pUsrData, int *ItemsCount);
	// Control functions
	int PlcHotStart();
	int PlcColdStart();
	int PlcStop();
	int CopyRamToRom(int Timeout);
	int Compress(int Timeout);
	// Security functions
	int GetProtection(PS7Protection pUsrData);
	int SetSessionPassword(char *Password);
	int ClearSessionPassword();
	// Properties
	int ExecTime();
	int LastError();
	int PDURequested();
	int PDULength();
	int PlcStatus();
	bool Connected();
	// Async functions
	int SetAsCallback(pfn_CliCompletion pCompletion, void *usrPtr);
	bool CheckAsCompletion(int *opResult);
	int WaitAsCompletion(longword Timeout);
	int AsReadArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
	int AsWriteArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
	int AsListBlocksOfType(int BlockType, PS7BlocksOfType pUsrData, int *ItemsCount);
	int AsReadSZL(int ID, int Index, PS7SZL pUsrData, int *Size);
	int AsReadSZLList(PS7SZLList pUsrData, int *ItemsCount);
	int AsUpload(int BlockType, int BlockNum, void *pUsrData, int *Size);
	int AsFullUpload(int BlockType, int BlockNum, void *pUsrData, int *Size);
	int AsDownload(int BlockNum, void *pUsrData,  int Size);
	int AsCopyRamToRom(int Timeout);
	int AsCompress(int Timeout);
	int AsDBRead(int DBNumber, int Start, int Size, void *pUsrData);
	int AsDBWrite(int DBNumber, int Start, int Size, void *pUsrData);
	int AsMBRead(int Start, int Size, void *pUsrData);
	int AsMBWrite(int Start, int Size, void *pUsrData);
	int AsEBRead(int Start, int Size, void *pUsrData);
	int AsEBWrite(int Start, int Size, void *pUsrData);
	int AsABRead(int Start, int Size, void *pUsrData);
	int AsABWrite(int Start, int Size, void *pUsrData);
    int AsTMRead(int Start, int Amount, void *pUsrData);
    int AsTMWrite(int Start, int Amount, void *pUsrData);
    int AsCTRead(int Start, int Amount, void *pUsrData);
	int AsCTWrite(int Start, int Amount, void *pUsrData);
    int AsDBGet(int DBNumber, void *pUsrData, int *Size);
	int AsDBFill(int DBNumber, int FillChar);
};
typedef TS7Client *PS7Client;
//******************************************************************************
//                           SERVER CLASS DEFINITION
//******************************************************************************
class TS7Server
{
private:
    S7Object Server;
public:
    TS7Server();
    ~TS7Server();
    // Control
    int Start();
    int StartTo(const char *Address);
    int Stop();
    int GetParam(int ParamNumber, void *pValue);
    int SetParam(int ParamNumber, void *pValue);
    // Events
    int SetEventsCallback(pfn_SrvCallBack PCallBack, void *UsrPtr);
    int SetReadEventsCallback(pfn_SrvCallBack PCallBack, void *UsrPtr);
    int SetRWAreaCallback(pfn_RWAreaCallBack PCallBack, void *UsrPtr);
    bool PickEvent(TSrvEvent *pEvent);
    void ClearEvents();
    longword GetEventsMask();
    longword GetLogMask();
    void SetEventsMask(longword Mask);
    void SetLogMask(longword Mask);
    // Resources
    int RegisterArea(int AreaCode, word Index, void *pUsrData, word Size);
    int UnregisterArea(int AreaCode, word Index);
    int LockArea(int AreaCode, word Index);
    int UnlockArea(int AreaCode, word Index);
    // Properties
    int ServerStatus();
    int GetCpuStatus();
    int SetCpuStatus(int Status);
    int ClientsCount();
};
typedef TS7Server *PS7Server;

//******************************************************************************
//                          PARTNER CLASS DEFINITION
//******************************************************************************
class TS7Partner
{
private:
	S7Object Partner; // Partner Handle
public:
	TS7Partner(bool Active);
	~TS7Partner();
	// Control
	int GetParam(int ParamNumber, void *pValue);
	int SetParam(int ParamNumber, void *pValue);
	int Start();
	int StartTo(const char *LocalAddress,
				const char *RemoteAddress,
				int LocalTSAP,
				int RemoteTSAP);
	int Stop();
	// Data I/O functions : BSend
	int BSend(longword R_ID, void *pUsrData, int Size);
	int AsBSend(longword R_ID, void *pUsrData, int Size);
	bool CheckAsBSendCompletion(int *opResult);
	int WaitAsBSendCompletion(longword Timeout);
	int SetSendCallback(pfn_ParSendCompletion pCompletion, void *usrPtr);
	// Data I/O functions : BRecv
	int BRecv(longword *R_ID, void *pUsrData, int *Size, longword Timeout);
	bool CheckAsBRecvCompletion(int *opResult, longword *R_ID, void *pUsrData, int *Size);
	int SetRecvCallback(pfn_ParRecvCallBack pCallback, void *usrPtr);
	// Properties
	int Status();
	int LastError();
	int GetTimes(longword *SendTime, longword *RecvTime);
	int GetStats(longword *BytesSent,
				 longword *BytesRecv,
				 longword *ErrSend,
				 longword *ErrRecv);
	bool Linked();
};
typedef TS7Partner *PS7Partner;
//******************************************************************************
//                               TEXT ROUTINES
// Only for C++, for pure C use xxx_ErrorText() which uses *char
//******************************************************************************
#define TextLen 1024

// String type
// Here we define generic TextString (by default mapped onto std::string).
// So you can change it if needed (Unicodestring, Ansistring etc...)

typedef std::string TextString;

TextString CliErrorText(int Error);
TextString SrvErrorText(int Error);
TextString ParErrorText(int Error);
TextString SrvEventText(TSrvEvent *Event);


#endif // __cplusplus
#endif // snap7_h
