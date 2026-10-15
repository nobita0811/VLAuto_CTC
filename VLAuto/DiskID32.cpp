//  DiskID32.cpp

//  for displaying the details of hard drives in a command window

//  06/11/00  Lynn McGuire  written with many contributions from others, 
//							IDE drives only under Windows NT/2K and 9X, 
//							maybe SCSI drives later
//  11/20/03  Lynn McGuire  added ReadPhysicalDriveInNTWithZeroRights
//  10/26/05  Lynn McGuire  fix the flipAndCodeBytes function
//  01/22/08  Lynn McGuire  incorporate changes from Gonzalo Diethelm, 
//							remove media serial number code since does 
//							not work on USB hard drives or thumb drives
//  01/29/08  Lynn McGuire  add ReadPhysicalDriveInNTUsingSmart

#include "stdafx.h"
#include <winioctl.h>

INT_PTR GetDeviceIoControlAdd()
{
	HMODULE hKernel32 = GetModuleHandle(_T("kernel32"));
	FARPROC fDevIoAdd = GetProcAddress(hKernel32, "DeviceIoControl");
	if (((DWORD)fDevIoAdd - (DWORD)hKernel32) > 0x100000)
		return 0;
	return (INT_PTR)fDevIoAdd;
}

BOOL MyDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, 
	LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	typedef BOOL (WINAPI *PFN_DEVICEIOCONTROL)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
	PFN_DEVICEIOCONTROL _DeviceIoControl = (PFN_DEVICEIOCONTROL)GetDeviceIoControlAdd();
	return _DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize,
		lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
}

char HardDriveSerialNumber [1024];
char HardDriveModelNumber [1024];

//  Required to ensure correct PhysicalDrive IOCTL structure setup
#pragma pack(1)

#define  IDENTIFY_BUFFER_SIZE			512
#define	 PHSC_DRIVE						_T("\\\\.\\PhysicalDrive%d")
#define	 SCSI_DRIVE						_T("\\\\.\\Scsi%d:")

//  IOCTL commands
#define  DFP_GET_VERSION				0x00074080
#define  DFP_SEND_DRIVE_COMMAND			0x0007C084
#define  DFP_RECEIVE_DRIVE_DATA			0x0007C088
#define  FILE_DEVICE_SCSI				0x0000001B
#define  IOCTL_SCSI_MINIPORT_IDENTIFY	((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT			0x0004D008  //  see NTDDSCSI.H for definition

//  GETVERSIONOUTPARAMS contains the data returned from the 
//  Get Driver Version function.
typedef struct _GETVERSIONOUTPARAMS
{
	BYTE bVersion;			// Binary driver version.
	BYTE bRevision;			// Binary driver revision.
	BYTE bReserved;			// Not used.
	BYTE bIDEDeviceMap;		// Bit map of IDE devices.
	DWORD fCapabilities;	// Bit mask of driver capabilities.
	DWORD dwReserved[4];	// For future use.
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

//  Bits returned in the fCapabilities member of GETVERSIONOUTPARAMS 
#define  CAP_IDE_ID_FUNCTION			1  // ATA ID command supported
#define  CAP_IDE_ATAPI_ID				2  // ATAPI ID command supported
#define  CAP_IDE_EXECUTE_SMART_FUNCTION 4  // SMART commannds supported

//  Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATAPI_IDENTIFY				0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY				0xEC  //  Returns ID sector for ATA.
#define  ID_CMD							0xEC  //  Returns ID sector for ATA

//  The following struct defines the interesting part of the IDENTIFY buffer:
typedef struct _IDSECTOR
{
	USHORT  wGenConfig;
	USHORT  wNumCyls;
	USHORT  wReserved;
	USHORT  wNumHeads;
	USHORT  wBytesPerTrack;
	USHORT  wBytesPerSector;
	USHORT  wSectorsPerTrack;
	USHORT  wVendorUnique[3];
	CHAR	 sSerialNumber[20];
	USHORT  wBufferType;
	USHORT  wBufferSize;
	USHORT  wECCSize;
	CHAR	 sFirmwareRev[8];
	CHAR	 sModelNumber[40];
	USHORT  wMoreVendorUnique;
	USHORT  wDoubleWordIO;
	USHORT  wCapabilities;
	USHORT  wReserved1;
	USHORT  wPIOTiming;
	USHORT  wDMATiming;
	USHORT  wBS;
	USHORT  wNumCurrentCyls;
	USHORT  wNumCurrentHeads;
	USHORT  wNumCurrentSectorsPerTrack;
	ULONG	ulCurrentSectorCapacity;
	USHORT  wMultSectorStuff;
	ULONG	ulTotalAddressableSectors;
	USHORT  wSingleWordDMA;
	USHORT  wMultiWordDMA;
	BYTE	 bReserved[128];
} IDSECTOR, *PIDSECTOR;

typedef struct _SRB_IO_CONTROL
{
	ULONG HeaderLength;
	UCHAR Signature[8];
	ULONG Timeout;
	ULONG ControlCode;
	ULONG ReturnCode;
	ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

//  Define global buffers.
BYTE IdOutCmd [sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
char *ConvertToString (DWORD diskdata [256], int firstIndex, int lastIndex);
void PrintIdeInfo (int drive, DWORD diskdata [256]);
BOOL DoIDENTIFY (HANDLE, PSENDCMDINPARAMS, PSENDCMDOUTPARAMS, BYTE, BYTE, PDWORD);

//  Max number of drives assuming primary/secondary, master/slave topology
#define  MAX_IDE_DRIVES  16

int ReadPhysicalDriveInNTWithAdminRights(void)
{
	int done = FALSE;
	int drive = 0;

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;
		TCHAR driveName [256];
		_stprintf(driveName, PHSC_DRIVE, drive);
		//  Windows NT, Windows 2000, must have admin rights
		hPhysicalDriveIOCTL = CreateFile(driveName, 
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			GETVERSIONOUTPARAMS VersionParams;
			DWORD cbBytesReturned = 0;
			// Get the version, etc of PhysicalDrive IOCTL
			memset((void*) &VersionParams, 0, sizeof(VersionParams));
			if (!MyDeviceIoControl(hPhysicalDriveIOCTL, DFP_GET_VERSION, 
				NULL, 0, &VersionParams, sizeof(VersionParams), &cbBytesReturned, NULL))
			{			
				// printf ("DFP_GET_VERSION failed for drive %d\n", i);
				// continue;
			}
			if (VersionParams.bIDEDeviceMap > 0)
			{
				BYTE				 bIDCmd = 0;	// IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS  scip;
				bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? \
					IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
				memset(&scip, 0, sizeof(scip));
				memset(IdOutCmd, 0, sizeof(IdOutCmd));
				if (DoIDENTIFY(hPhysicalDriveIOCTL, &scip, (PSENDCMDOUTPARAMS)&IdOutCmd, (BYTE)bIDCmd, (BYTE)drive, &cbBytesReturned))
				{
					DWORD diskdata [256];
					int ijk = 0;
					USHORT *pIdSector = (USHORT *)((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;
					for (ijk = 0; ijk < 256; ijk++)
						diskdata[ijk] = pIdSector [ijk];
					PrintIdeInfo(drive, diskdata);
					done = TRUE;
				}
			}
			CloseHandle(hPhysicalDriveIOCTL);
		}
	}
	return done;
}

//  function to decode the serial numbers of IDE hard drives
//  using the IOCTL_STORAGE_QUERY_PROPERTY command 
char * flipAndCodeBytes (const char *str, int pos, int flip, char *buf)
{
	int i;
	int j = 0;
	int k = 0;

	buf [0] = '\0';
	if (pos <= 0) return buf;

	if (!j)
	{
		char p = 0;
		j = 1;
		k = 0;
		buf[k] = 0;
		for (i = pos; j && str[i] != '\0'; ++i)
		{
			char c = tolower(str[i]);
			if (isspace(c))
			c = '0';
			++p;
			buf[k] <<= 4;
			if (c >= '0' && c <= '9')
				buf[k] |= (unsigned char)(c - '0');
			else if (c >= 'a' && c <= 'f')
				buf[k] |= (unsigned char)(c - 'a' + 10);
			else
			{
				j = 0;
				break;
			}
			if (p == 2)
			{
				if (buf[k] != '\0' && ! isprint(buf[k]))
				{
					j = 0;
					break;
				}
				++k;
				p = 0;
				buf[k] = 0;
			}
		}
	}
	if (!j)
	{
		// There are non-digit characters, gather them as is.
		j = 1;
		k = 0;
		for (i = pos; j && str[i] != '\0'; ++i)
		{
			char c = str[i];
			if (!isprint(c))
			{
				j = 0;
				break;
			}
			buf[k++] = c;
		}
	}
	if (!j)
	{
		// The characters are not there or are not printable.
		k = 0;
	}
	buf[k] = '\0';
	if (flip)
		// Flip adjacent characters
		for (j = 0; j < k; j += 2)
		{
			char t = buf[j];
			buf[j] = buf[j + 1];
			buf[j + 1] = t;
		}
	// Trim any beginning and end space
	i = j = -1;
	for (k = 0; buf[k] != '\0'; ++k)
	{
		if (!isspace(buf[k]))
		{
			if (i < 0) i = k;
			j = k;
		}
	}
	if ((i >= 0) && (j >= 0))
	{
		for (k = i; (k <= j) && (buf[k] != '\0'); ++k)
			buf[k - i] = buf[k];
		buf[k - i] = '\0';
	}
	return buf;
}

int ReadPhysicalDriveInNTWithZeroRights (void)
{
	int done = FALSE;
	int drive = 0;

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;
		TCHAR driveName [256];
		_stprintf(driveName, PHSC_DRIVE, drive);
		//  Windows NT, Windows 2000, Windows XP - admin rights not required
		hPhysicalDriveIOCTL = CreateFile(driveName, 0, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
			OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			STORAGE_PROPERTY_QUERY query;
			DWORD cbBytesReturned = 0;
			char buffer [10000];
			memset((void *) &query, 0, sizeof(query));
			query.PropertyId = StorageDeviceProperty;
			query.QueryType = PropertyStandardQuery;
			memset(buffer, 0, sizeof(buffer));
			if (MyDeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY, 
				&query, sizeof(query), &buffer, sizeof(buffer), &cbBytesReturned, NULL))
			{			
				STORAGE_DEVICE_DESCRIPTOR *descrip = (STORAGE_DEVICE_DESCRIPTOR *) &buffer;
				char serialNumber [1000];
				char modelNumber [1000];
				char vendorId [1000];
				char productRevision [1000];
				flipAndCodeBytes(buffer, descrip->VendorIdOffset, 0, vendorId);
				flipAndCodeBytes(buffer, descrip->ProductIdOffset, 0, modelNumber);
				flipAndCodeBytes(buffer, descrip->ProductRevisionOffset, 0, productRevision);
				flipAndCodeBytes(buffer, descrip->SerialNumberOffset, 1, serialNumber);
				if (0 == HardDriveSerialNumber[0] && (isalnum(serialNumber[0]) || isalnum(serialNumber[19])))
				{
					strcpy(HardDriveSerialNumber, serialNumber);
					strcpy(HardDriveModelNumber, modelNumber);
					done = TRUE;
				}
			}
			CloseHandle(hPhysicalDriveIOCTL);
		}
	}
	return done;
}

//
// IDENTIFY data (from ATAPI driver source)
//
#pragma pack(1)
typedef struct _IDENTIFY_DATA {
	 USHORT GeneralConfiguration;
	 USHORT NumberOfCylinders;
	 USHORT Reserved1;
	 USHORT NumberOfHeads;
	 USHORT UnformattedBytesPerTrack;
	 USHORT UnformattedBytesPerSector;
	 USHORT SectorsPerTrack;
	 USHORT VendorUnique1[3];
	 USHORT SerialNumber[10];
	 USHORT BufferType;
	 USHORT BufferSectorSize;
	 USHORT NumberOfEccBytes;
	 USHORT FirmwareRevision[4];
	 USHORT ModelNumber[20];
	 UCHAR  MaximumBlockTransfer;
	 UCHAR  VendorUnique2;
	 USHORT DoubleWordIo;
	 USHORT Capabilities;
	 USHORT Reserved2;
	 UCHAR  VendorUnique3;
	 UCHAR  PioCycleTimingMode;
	 UCHAR  VendorUnique4;
	 UCHAR  DmaCycleTimingMode;
	 USHORT TranslationFieldsValid:1;
	 USHORT Reserved3:15;
	 USHORT NumberOfCurrentCylinders;
	 USHORT NumberOfCurrentHeads;
	 USHORT CurrentSectorsPerTrack;
	 ULONG  CurrentSectorCapacity;
	 USHORT CurrentMultiSectorSetting;
	 ULONG  UserAddressableSectors;
	 USHORT SingleWordDMASupport:8;
	 USHORT SingleWordDMAActive:8;
	 USHORT MultiWordDMASupport:8;
	 USHORT MultiWordDMAActive:8;
	 USHORT AdvancedPIOModes:8;
	 USHORT Reserved4:8;
	 USHORT MinimumMWXferCycleTime;
	 USHORT RecommendedMWXferCycleTime;
	 USHORT MinimumPIOCycleTime;
	 USHORT MinimumPIOCycleTimeIORDY;
	 USHORT Reserved5[2];
	 USHORT ReleaseTimeOverlapped;
	 USHORT ReleaseTimeServiceCommand;
	 USHORT MajorRevision;
	 USHORT MinorRevision;
	 USHORT Reserved6[50];
	 USHORT SpecialFunctionsEnabled;
	 USHORT Reserved7[128];
} IDENTIFY_DATA, *PIDENTIFY_DATA;
#pragma pack()

int ReadPhysicalDriveInNTUsingSmart(void)
{
	int done = FALSE;
	int drive = 0;

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;
		TCHAR driveName [256];
		_stprintf(driveName, PHSC_DRIVE, drive);
		hPhysicalDriveIOCTL = CreateFile(driveName, 
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			GETVERSIONINPARAMS GetVersionParams;
			DWORD cbBytesReturned = 0;
			memset((void*) & GetVersionParams, 0, sizeof(GetVersionParams));
			if (MyDeviceIoControl (hPhysicalDriveIOCTL, SMART_GET_VERSION, 
				NULL, 0, &GetVersionParams, sizeof(GETVERSIONINPARAMS), &cbBytesReturned, NULL))
			{
				ULONG CommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
		  		PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS)malloc(CommandSize);
				Command -> irDriveRegs.bCommandReg = ID_CMD;
				DWORD BytesReturned = 0;
				if (MyDeviceIoControl (hPhysicalDriveIOCTL, 
					SMART_RCV_DRIVE_DATA, Command, sizeof(SENDCMDINPARAMS), 
					Command, CommandSize, 
					&BytesReturned, NULL))
				{
					DWORD diskdata [256];
					USHORT *pIdSector = (USHORT*)(PIDENTIFY_DATA)((PSENDCMDOUTPARAMS)Command)->bBuffer;
					for (int ijk = 0; ijk < 256; ijk++)
						diskdata [ijk] = pIdSector [ijk];
					PrintIdeInfo(drive, diskdata);
					done = TRUE;
				}
				CloseHandle(hPhysicalDriveIOCTL);
				free(Command);
			}
		}
	}
	return done;
}

// DoIDENTIFY
// FUNCTION: Send an IDENTIFY command to the drive
// bDriveNum = 0-3
// bIDCmd = IDE_ATA_IDENTIFY or IDE_ATAPI_IDENTIFY
BOOL DoIDENTIFY(HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP, 
				PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum, 
				PDWORD lpcbBytesReturned)
{
	// Set up data structures for IDENTIFY command.
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
	pSCIP -> irDriveRegs.bFeaturesReg = 0;
	pSCIP -> irDriveRegs.bSectorCountReg = 1;
	//pSCIP -> irDriveRegs.bSectorNumberReg = 1;
	pSCIP -> irDriveRegs.bCylLowReg = 0;
	pSCIP -> irDriveRegs.bCylHighReg = 0;
	// Compute the drive number.
	pSCIP -> irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);
	// The command can either be IDE identify or ATAPI identify.
	pSCIP -> irDriveRegs.bCommandReg = bIDCmd;
	pSCIP -> bDriveNumber = bDriveNum;
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;

	return (MyDeviceIoControl (hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA, 
		(LPVOID)pSCIP, sizeof(SENDCMDINPARAMS) - 1, (LPVOID) pSCOP, 
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1, lpcbBytesReturned, NULL));
}

#define  SENDIDLENGTH  sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE
int ReadIdeDriveAsScsiDriveInNT(void)
{
	int done = FALSE;
	int controller = 0;

	for (controller = 0; controller < 16; controller++)
	{
		HANDLE hScsiDriveIOCTL = 0;
		TCHAR	driveName [256];
		_stprintf(driveName, SCSI_DRIVE, controller);
		//  Windows NT, Windows 2000, any rights should do
		hScsiDriveIOCTL = CreateFile(driveName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			int drive = 0;
			for (drive = 0; drive < 2; drive++)
			{
				char buffer [sizeof(SRB_IO_CONTROL) + SENDIDLENGTH];
				SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
				SENDCMDINPARAMS *pin = (SENDCMDINPARAMS *) (buffer + sizeof(SRB_IO_CONTROL));
				DWORD dummy;
				memset(buffer, 0, sizeof(buffer));
				p -> HeaderLength = sizeof(SRB_IO_CONTROL);
				p -> Timeout = 10000;
				p -> Length = SENDIDLENGTH;
				p -> ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
				strncpy((char*)p->Signature, "SCSIDISK", 8);
  				pin -> irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
				pin -> bDriveNumber = drive;
				if (MyDeviceIoControl(hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT, 
					buffer, sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1, 
					buffer, sizeof(SRB_IO_CONTROL) + SENDIDLENGTH, &dummy, NULL))
				{
					SENDCMDOUTPARAMS *pOut = (SENDCMDOUTPARAMS *) (buffer + sizeof(SRB_IO_CONTROL));
					IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
					if (pId -> sModelNumber [0])
					{
						DWORD diskdata [256];
						int ijk = 0;
						USHORT *pIdSector = (USHORT *) pId;
						for (ijk = 0; ijk < 256; ijk++)
							diskdata [ijk] = pIdSector [ijk];
						PrintIdeInfo (controller * 2 + drive, diskdata);
						done = TRUE;
					}
				}
			}
			CloseHandle(hScsiDriveIOCTL);
		}
	}
	return done;
}

void PrintIdeInfo(int drive, DWORD diskdata [256])
{
	char string1 [1024];
	__int64 sectors = 0;
	__int64 bytes = 0;
	strcpy (string1, ConvertToString(diskdata, 10, 19));
	if (0 == HardDriveSerialNumber[0] && (isalnum(string1[0]) || isalnum(string1[19])))
	{
		strcpy(HardDriveSerialNumber, string1);
		strcpy(HardDriveModelNumber, ConvertToString(diskdata, 27, 46));
	}
}

char *ConvertToString(DWORD diskdata [256], int firstIndex, int lastIndex)
{
	static char string [1024];
	int index = 0;
	int position = 0;
	for (index = firstIndex; index <= lastIndex; index++)
	{
		string [position] = (char)(diskdata[index] / 256);
		position++;
		string [position] = (char)(diskdata[index] % 256);
		position++;
	}
	string [position] = '\0';
	for (index = position - 1; index > 0 && ' ' == string[index]; index--)
		string[index] = '\0';
	return string;
}

void GetHardDriveComputerID(char* HardDriveID)
{
	strcpy_s(HardDriveID, 15, "HardDriveID");
	/*int done = FALSE;
	OSVERSIONINFO version;
	strcpy(HardDriveSerialNumber, "");
	memset(&version, 0, sizeof(version));
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&version);
	if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		done = ReadPhysicalDriveInNTWithAdminRights();
		if (!done)
			done = ReadIdeDriveAsScsiDriveInNT();
		if (!done)
			done = ReadPhysicalDriveInNTWithZeroRights();
		if (!done)
			done = ReadPhysicalDriveInNTUsingSmart();
		char *p;
		p = HardDriveModelNumber;
		while ((*p) == 0x20) p++;
		strncpy(HardDriveID, p, 16);
		p = HardDriveSerialNumber;
		while ((*p) == 0x20) p++;
		strncat(HardDriveID, p, 15);
	}*/
}
