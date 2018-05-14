
///////////////////////////////////////////////////////////////////////////////
///

/// �˲�����Ҫ��һЩ������������ʵ��
///
/// 
///
/// 
///  .
///
///////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C" {
#endif
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <string.h>
#ifdef __cplusplus
}; // extern "C"
#endif

#include "tools.h"
#define  ENCRYPTFILE_HEADER_BUFFER_TAG '123'
#define  ENCRYPTFILE_BUFFER_TAG '234'
#define PAGEDCODE code_seg("PAGE")  
#define LOCKEDCODE code_seg()  


PFLT_FILTER gFilterHandle;
HANDLE  handle;
LIST_ENTRY HidePathListHeader;
KSPIN_LOCK HidePathListLock;

PRKEVENT g_pEventObject = NULL;  
//�����Ϣ  
OBJECT_HANDLE_INFORMATION g_ObjectHandleInfo;  
BOOLEAN EXIT = TRUE;



/************************************************************************/
/* ����ƥ�䴮 ����ƥ���ƫ�ƣ���ƥ�䷵��-1                                  */
/************************************************************************/
INT UnicodeStringIndexOf(UNICODE_STRING *sour,UNICODE_STRING *val)
{
	//ԭ�����ȱ�����ڱȽϴ�
	if((sour->Length)<(val->Length))
	{

		return -1;
	}

	int i;
	//ÿ���ַ���ʼ
	int sour_len=sour->Length/2;
	int val_len=val->Length/2;
	int len=sour_len-val_len+1;

	//////////////DbgPrint("sour is %wZ and val is %wZ",sour,val);
	//////////////DbgPrint("sour_len is %d and val_len is %d and len is %d",sour_len,val_len,len);
	for(i=0;i<len;i++)
	{
		//��־����ʾ��ǰƫ�ƿ�ʼ�Ƿ�ƥ��Ŀ�괮
		bool flag=true;
		for(int j=0;j<val_len;j++)
		{
			//�����ƥ�䣬���flag��Ϊfalse,��������Ƚ�;
			WCHAR c1=sour->Buffer[i+j];
			WCHAR c2=val->Buffer[j];
			//////////////DbgPrint("C1 IS %C AND C2 IS %C",c1,c2);
			if(c1!=c2)
			{
				flag=false;
				break;
			}
		}
		//flagΪtrue��֤����ǰƫ��ƥ��ɹ�����
		if(flag)
		{

			return i;
		}
	}
	return -1;
}



//�ͷŽ�������
void FreeProcessInfoList(PPROCESS_INFO head)
{
	PPROCESS_INFO tmp=NULL;
	PPROCESS_INFO next=NULL;

	tmp=head;
	while(tmp!=NULL)
	{
		next=tmp->next;
		ExFreePoolWithTag(tmp,BUFFER_SWAP_TAG) ;
		tmp=next;
	}
}
//�ͷ�һ�����Ա�
void FreeStrategy(PTYPE_KEY_PROCESS head)
{
	PTYPE_KEY_PROCESS tmp=NULL;
	PTYPE_KEY_PROCESS next=NULL;
	tmp=head;
	while(tmp!=NULL)
	{
		next=tmp->next;
		//ɾ����������
		FreeProcessInfoList(tmp->processInfo);
		ExFreePoolWithTag(tmp,BUFFER_SWAP_TAG);
		tmp=next;		
	}

}

//�жϽ������Ƿ�Ϊ�����͵Ļ��ܽ���,keyWork Ϊ����ָ��
BOOLEAN IsSecretProcess(PTYPE_KEY_PROCESS keyWord,CHAR *processName)
{
	if (keyWord==NULL||processName==NULL)
	{
		return FALSE;
	}

	PPROCESS_INFO info = keyWord->processInfo;
	
	while(info!=NULL)
	{
		KdPrint(("��ǰ����processName=%s\n  ���Ա��еĽ��� info->processName=%s\n",processName,info->processName));
		/*if(strncmp(processName,info->processName,strlen(processName))==0)
		{
		return TRUE;
		}*/
		info=info->next;
	}
	return FALSE;
}

/************************************************************************/
/*    ��ȡ��������ƫ��                                                 */
/************************************************************************/
//////////////////////////////////////////////////////////////////////////
//��ȡ��������


ULONG 
GetProcessNameOffset(
    VOID
    )
{
    PEPROCESS       curproc;
    ULONG             i;
 
    curproc = PsGetCurrentProcess();
 
    //
    // Scan for 12KB, hopping the KPEB never grows that big!
    //
    for( i = 0; i < 3*PAGE_SIZE; i++ ) 
	{
     
        if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") )) 
		{

            return i;
        }
    }
    //
    // Name not found - oh, well
    //
    return 0;
}
 

//2017.9.12
PCHAR
GetCurrentProcessName(ULONG ProcessNameOffset)
{
    PEPROCESS       curproc;
    char            *nameptr;
    ULONG           i;
 
    //
    // We only try and get the name if we located the name offset
    //
    if( ProcessNameOffset ) {
    
        //
        // Get a pointer to the current process block
        //
        curproc = PsGetCurrentProcess();
 
        //
        // Dig into it to extract the name. Make sure to leave enough room
        // in the buffer for the appended process ID.
        //
        nameptr   = (PCHAR) curproc + ProcessNameOffset;
		/*
		#if defined(_M_IA64)
        sprintf( szName + strlen(szName), ":%I64d", PsGetCurrentProcessId());
		#else
        sprintf( szName + strlen(szName), ":%d", (ULONG) PsGetCurrentProcessId());
		#endif
		//*/
 
    } else {
		
       nameptr="";
    }
    return nameptr;
}


/************************************************************************/
/*            �ַ�������    chat * ��wchar_t* ��ת  2017.9.11    */
/************************************************************************/

void wstr2cstr(const wchar_t *pwstr , char *pcstr, size_t len)
{     
	for (size_t i=0;i<len;i++)
	{
		pcstr[i*2]=(char)(pwstr[i]>>8);//��λ
		pcstr[i*2+1]=(char)(pwstr[i]);//��λ�ض�
	}
}    

void cstr2wstr( const char *pcstr,wchar_t *pwstr , size_t len)
{     
	for(size_t i=0;i<len;i++)
	{
		pwstr[i]=(WCHAR)pcstr[i];
	}
}  




/** ��ȡ�ļ���·�������ͣ������ļ��У�ȥ����**/

NTSTATUS GetFileInformation(__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__inout PSTREAM_HANDLE_CONTEXT ctx)
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	//�ж��Ƿ��ļ���
	BOOLEAN isDir;	
	status = FltIsDirectory(FltObjects->FileObject,FltObjects->Instance,&isDir);


	if (NT_SUCCESS(status))
	{
		//�ļ���ֱ������
		if (isDir)
		{
			return STATUS_UNSUCCESSFUL;  //���ļ���ֱ�ӷ���ʧ��
		}
		else
		{
			//��ȡ�ļ�����FLT_FILE_NAME_OPENED �ļ����򿪲���
			status=FltGetFileNameInformation(Data,
				FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
				&nameInfo);

			if (NT_SUCCESS(status))
			{
					 
				status = FltParseFileNameInformation(nameInfo);
				
				if (NT_SUCCESS(status))
				{    char FileName[260] = "X:";
				      
				
					if (NPUnicodeStringToChar(&nameInfo->Name, FileName))
					{
						if (strstr(FileName,"1.LOG"))
						{
							return  STATUS_UNSUCCESSFUL;
						}
					}
							
					
					int i = 1;
					int count = 1;
			
					PWCHAR  p = nameInfo->ParentDir.Buffer;
					/**��ȡ��Ŀ¼�еĵ�һ��Ŀ¼  2018_5_6*/
					while (p[i] != '\\')
					{
						//KdPrint(("count = %d ,p[i] = %c\n",count,p[i]));
						count++;
						i++;
					}

					ctx->fileFullPath = nameInfo->Name;
					ctx->fileStyle = nameInfo->Extension;
					ctx->fileVolumeName = nameInfo->Volume;
					ctx->ParentDir  = nameInfo->ParentDir;
						/**��ȡ��Ŀ¼�еĵ�һ��Ŀ¼���Ŀ¼�ϲ�  2018_5_6*/
					ctx->fileVolumeName.Length =  ctx->fileVolumeName.Length +2*count;
					ctx->fileVolumeName.MaximumLength =  ctx->fileVolumeName.MaximumLength +2*count;
					//KdPrint(("V = %wZ,length = %d,mAxL = %d ,count = %d\n",&nameInfo->Volume,nameInfo->Volume.Length,nameInfo->Volume.MaximumLength,count));
				   // KdPrint(("Volume = %wZ,length = %d,mAxL = %d \n",&ctx->fileVolumeName,ctx->fileVolumeName.Length,ctx->fileVolumeName.MaximumLength));
				
				}    
			}
		}

		if (NULL!= nameInfo)
		{
			FltReleaseFileNameInformation(nameInfo);
		}
	}
	

	return status;

}




BOOLEAN NPUnicodeStringToChar(PUNICODE_STRING UniName, char Name[])
	{
		ANSI_STRING	AnsiName;			
		NTSTATUS	ntstatus;
		char*		nameptr;			

		__try {	    		   		    		
			ntstatus = RtlUnicodeStringToAnsiString(&AnsiName, UniName, TRUE);		

			if (AnsiName.Length < 260) {
				nameptr = (PCHAR)AnsiName.Buffer;
				//Convert into upper case and copy to buffer
				strcpy(Name, _strupr(nameptr));						    	    
				//DbgPrint("NPUnicodeStringToChar : %s\n", Name);	
			}		  	
			RtlFreeAnsiString(&AnsiName);		 
		} 
		__except(EXCEPTION_EXECUTE_HANDLER) {
			//DbgPrint("NPUnicodeStringToChar EXCEPTION_EXECUTE_HANDLER\n");	
			return FALSE;
		}
		return TRUE;
	}      



BOOLEAN NPUnicodeStringToChar(PUNICODE_STRING UniName, char Name[],USHORT Length)
{
	ANSI_STRING	AnsiName;			
	NTSTATUS	ntstatus;
	char*		nameptr;			

	__try {	    		   		    		
		ntstatus = RtlUnicodeStringToAnsiString(&AnsiName, UniName, TRUE);		

		if (AnsiName.Length < 260) {
			nameptr = (PCHAR)AnsiName.Buffer;
			//Convert into upper case and copy to buffer

		//	strncpy(Name, nameptr,Length/2+4);	
			strcpy(Name, _strupr(nameptr));	
			//DbgPrint("NPUnicodeStringToChar1 : %wZ  2 = %Z\n", UniName,AnsiName);	
		}		  	
		RtlFreeAnsiString(&AnsiName);		 
	} 
	__except(EXCEPTION_EXECUTE_HANDLER) {
		//DbgPrint("NPUnicodeStringToChar EXCEPTION_EXECUTE_HANDLER\n");	
		return FALSE;
	}
	return TRUE;
}      







 ULONG	GetTime()
{
	LARGE_INTEGER TickCount = {0};
	LARGE_INTEGER GelinTime = {0};
	LARGE_INTEGER LocalTime = {0};
	TIME_FIELDS NowFields;
	ULONG Inc = 0;
	ULONG Day = 0;
	ULONG Hour = 0;
	ULONG Minute = 0;
	ULONG Second = 0;
	ULONG pTime;

	Inc = KeQueryTimeIncrement();
	KeQueryTickCount(&TickCount);
	TickCount.QuadPart *= Inc;
	TickCount.QuadPart /= 10000;
	Day = TickCount.QuadPart / (1000*60*60*24);
	Hour = TickCount.QuadPart % (1000*60*60*24) / (1000*60*60);
	Minute = TickCount.QuadPart % (1000*60*60*24) % (1000*60*60) / (1000*60);
	Second = TickCount.QuadPart % (1000*60*60*24) % (1000*60*60) % (1000*60) / 1000;

	//KdPrint(("ϵͳ������%2d��%2dСʱ%2d����%2d��\n", Day, Hour, Minute, Second));

	pTime = (Hour*3600+Minute*60+Second);

	return  pTime;

	/*KeQuerySystemTime(&GelinTime);
	ExSystemTimeToLocalTime(&GelinTime, &LocalTime);
	RtlTimeToTimeFields(&LocalTime, &NowFields);
	KdPrint(("ϵͳ��ǰʱ�� : %4d��%2d��%2d�� %2d:%2d:%2d\n",
		NowFields.Year, 
		NowFields.Month,
		NowFields.Day,
		NowFields.Hour,
		NowFields.Minute,
		NowFields.Second));*/
}
  /***�ж��Ƿ���Ҫ���˵��Ľ�����  icedxu_2018_5_4**/
 BOOLEAN  IsSecretProcess(CHAR  *processName)
 {
	// PCHAR name = ";System;svchost.exe;vmtoolsd.exe;explorer.exe;SearchProtocol;iexplore.exe;SearchIndexer;taskhost.exe;WmiApSrv.exe;";
	 PCHAR name = "SearchIndexer;taskhost.exe;WmiApSrv.exe;vmtoolsd.exe;;";
	if (strstr(name,processName) > 0)
	{
		return TRUE;
	}
	return FALSE;
 }



 //����תΪchar����
 BOOLEAN IntegerToChar(ULONG pTime,CHAR T[])
 {
	 UNICODE_STRING p = {0};
	 p.Buffer = (PWSTR)ExAllocatePool(PagedPool,BUFFERSIZE);
	 p.MaximumLength = BUFFERSIZE;
	 NTSTATUS status;
	 BOOLEAN bl;
	 status = RtlIntegerToUnicodeString(pTime,10,&p);
	 if (NT_SUCCESS(status))
	 {
		// CHAR T[1] = {0};
		 bl = NPUnicodeStringToChar(&p , T);
		 if (!bl)
		 {
			return FALSE;
		 }
		 return TRUE;
	 }

 }




 //���ݽ��� ID ���ؽ��� EPROCESS��ʧ�ܷ��� NULL
	 PEPROCESS LookupProcess(HANDLE Pid)
 {
	 PEPROCESS eprocess = NULL;
	 if (NT_SUCCESS(PsLookupProcessByProcessId(Pid, &eprocess))) 
		 return eprocess;
	 else
		 return NULL;
 }
 //ö�ٽ���
	 VOID EnumProcess(ULONG processID)
 {
	 ULONG i = 0;
	 PEPROCESS eproc = NULL;
	 for (i = 4; i<262144; i = i + 4)
	 {
		 eproc = LookupProcess((HANDLE)i);
		 if (eproc != NULL)
		 {
			if (processID == (UINT32)PsGetProcessId(eproc))
			{
				if ( !IsSecretProcess((PCHAR)PsGetProcessImageFileName(eproc)) )
				{
				    	DbgPrint("EPROCESS = %p, PID = %ld, PPID = %ld, Name = %s\n", 
						eproc,
						(UINT32)PsGetProcessId(eproc),
						(UINT32)PsGetProcessInheritedFromUniqueProcessId(eproc),
						PsGetProcessImageFileName(eproc));
					    ObDereferenceObject(eproc);
				}
			}			
		 }
	 }
 }





 //
 // Enable log event: for synchronization
 //
 static KEVENT   gs_eventEnableKeLog;

 //----------------------------------------------------------------------
 //
 // initialization interface
 //
 //----------------------------------------------------------------------
 //
 // initialize the global data structures, when the driver is loading. 
 // (Call in DriverEntry())
 //
 NTSTATUS
	 Dbg_LoadInit()
 {
	 // Initialize the event
	 KeInitializeEvent(&gs_eventEnableKeLog, SynchronizationEvent, TRUE);
	 return STATUS_SUCCESS;
 }

 static void WaitForWriteMutex()
 {
	 // Wait for enable log event
	 KeWaitForSingleObject(&gs_eventEnableKeLog, Executive, KernelMode, TRUE, 0);
	 KeClearEvent(&gs_eventEnableKeLog);
 }
 static void ReleaseWriteMutex()
 {
	 // Set enable log event
	 KeSetEvent(&gs_eventEnableKeLog, 0, FALSE);
 }
 //----------------------------------------------------------------------
 //
 // DbgKeLog
 //
 // Trace to file.
 //
 //----------------------------------------------------------------------
 VOID DbgKeLog(PCHAR lpszLog)
 {
	 if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	 {
		// TOKdPrint(("TKeHook: KeLog: IRQL too hight.../n"));
		 KdPrint(("TKeHook: KeLog: IRQL too hight.../n"));
		 return ;
	 }
	 WaitForWriteMutex();

	 KdPrint(("%s", lpszLog));

	 __try
	 {
		 IO_STATUS_BLOCK  IoStatus;
		 OBJECT_ATTRIBUTES objectAttributes;
		 NTSTATUS status;
		 HANDLE FileHandle = NULL;
		 UNICODE_STRING fileName;
		 RtlInitUnicodeString(&fileName,L"//??//C://KeLog.log") ;
	

		 InitializeObjectAttributes (&objectAttributes,
			 (PUNICODE_STRING)&fileName,
			 OBJ_CASE_INSENSITIVE,  //�Դ�Сд����
			 NULL,
			 NULL );

		 status = ZwCreateFile(&FileHandle,
			 FILE_APPEND_DATA, //���������
			 &objectAttributes,
			 &IoStatus,
			 0, 
			 FILE_ATTRIBUTE_NORMAL,
			 FILE_SHARE_WRITE,
			 FILE_OPEN_IF, //��ʹ����Ҳ����
			 FILE_SYNCHRONOUS_IO_NONALERT,
			 NULL,     
			 0 );



			 ZwWriteFile(FileHandle,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatus,
				 lpszLog,
				 strlen(lpszLog),
				 NULL,
				 NULL 
				 );

			 ZwClose(FileHandle);
		


		 ReleaseWriteMutex();
		 return ;
	 }
	 __except(EXCEPTION_EXECUTE_HANDLER)
	 {
		 ReleaseWriteMutex();
		 KdPrint(("TKeHook: DbgKeLog() except: %0xd !!/n", GetExceptionCode()));
		 return ;
	 }
 }








 VOID writeLog(__inout PFLT_CALLBACK_DATA Data,
	 __in PCFLT_RELATED_OBJECTS FltObjects,
	 __in PVOID CompletionContext)
 {
	 PUNICODE_STRING  tmpLog;
	 NTSTATUS status = STATUS_SUCCESS;
	 OBJECT_ATTRIBUTES objectAttributes;
	 UNICODE_STRING fileName;
	 IO_STATUS_BLOCK ioStatus;
	 PFLT_INSTANCE Instance = NULL;
	 //��ʼ��UNICODE_STRING�ַ���
	 RtlInitUnicodeString( &fileName, 
		 L"\\??\\C:\\2.log");

	 InitializeObjectAttributes(
		 &objectAttributes,
		 &fileName,
		 OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		 NULL,
		 NULL );

	 status = FltCreateFile( FltObjects->Filter,
		 FltObjects->Instance,
		 &handle,
		 FILE_ALL_ACCESS,
		 &objectAttributes,
		 &ioStatus,
		 (PLARGE_INTEGER) NULL,
		 FILE_ATTRIBUTE_NORMAL,
		 FILE_SHARE_WRITE,
		 FILE_OPEN_IF,   // ulong  createdisposition
		 FILE_NON_DIRECTORY_FILE,   
		 NULL,
		 NULL,
		 IO_IGNORE_SHARE_ACCESS_CHECK );

	 if (NT_SUCCESS(status))
	 {
		 KdPrint(("open file successfully.\n"));
		/* PVOID fileObject = NULL;
		  status = ObReferenceObjectByHandle(
		 handle,
		 FILE_ALL_ACCESS,
		 NULL,
		 KernelMode,
		 &fileObject,
		 NULL);	*/
		 LARGE_INTEGER offset;
		 offset.QuadPart=0;
		 PVOID buff;
		 ULONG buffLen;
		 CHAR *p;
		 p="niaho\r\n";
		 buffLen = strlen(p);
		 buff = ExAllocatePool(NonPagedPool,buffLen);
		 if(buff==NULL)
		 {
			 DbgPrint("no enough memoy");
			// return STATUS_UNSUCCESSFUL;
			 return ;
		 }
		 RtlZeroMemory(buff,buffLen);
		 RtlCopyMemory(buff,p ,strlen(p));
		 status = FltWriteFile(
			 FltObjects->Instance,
			 FltObjects->FileObject,
			 &offset,  //ByteOffset
			 buffLen   , //Length
			 "nihao", //buffer
			 FLTFL_IO_OPERATION_NON_CACHED, //flags
			 NULL,//ByteWritten
			 NULL,//CallbackRoutine
			 NULL);  //CallbackContext

		 if (!NT_SUCCESS(status))
		 {
			 DbgPrint("fail to write %08x",status);
		 }
		 ExFreePool(buff);


	 }
	 if (handle != NULL)
	 {
		 // FltClose(handle);
		 ZwClose(handle);
	 }

	 return ;
 }


 /****
 *1.���ù�������WorkItem��WorkItem���Ŷ�ע��Ļص������������̴���DISPATCH_LEVEL����ʱ���ص�����������У�
 �����̽��͵�PASSIVE_LEVELʱ����Щ�����еĻص��������ᱻϵͳ���á�
 2.����PsCreateSystemThread��ʽע��һ���̣߳���ע��һ���¼�������һ���ڴ档
 ����������Ϣд���ļ���ʱ���Ƚ���Ϣд���ڴ棬Ȼ��Set����¼���������߳���ѭ��KeWaitForSingleObject���Event��
 Ȼ���ڵ���ZwWriteFile����Ϣд���ļ�������Ҫע��һ������ļ���д��ͬ�����⡣��ʵ�ֵľ������ַ�����
 ***/


 VOID StartThread()
 {
    NTSTATUS status = STATUS_SUCCESS;
	HANDLE   hThread = NULL;
	KEVENT kEvent;
	//KeInitializeEvent(&kEvent,SynchronizationEvent,FALSE);
	status = PsCreateSystemThread(&hThread, //�������߳�
		   (ACCESS_MASK)THREAD_ALL_ACCESS,
		   NULL,
		   NULL,//NtCurrentProcess(),�߳����ڵ�ַ�ռ�Ľ��̵�handle
		   NULL,
		   (PKSTART_ROUTINE)ThreadProc,
		  &kEvent);  //PVOID       StartContext   ��ӦThreadProc�еĲ���
	if (!NT_SUCCESS(status))
	{
		KdPrint(("����ʧ�� \n"));
		ZwClose(hThread);
		return ;
	}
	KdPrint(("�����ɹ� \n"));
	ZwClose(hThread);

	//KeWaitForSingleObject(&kEvent,Executive,KernelMode,FALSE,NULL);
	return ;
 }




 VOID  ThreadProc(IN PVOID pContext)  
 {  
	 DbgPrint("CreateThread Successfully");  
	 PKEVENT pEvent = (PKEVENT)pContext;
	 PLOG_LIST hideList;
	 LARGE_INTEGER Interval;
	 ULONG Msec = 3000;
	 PLIST_ENTRY pListNode;
	 OBJECT_ATTRIBUTES objectAttributes;
	 IO_STATUS_BLOCK iostatus;
	 HANDLE hfile;
	 NTSTATUS  status;
	 UNICODE_STRING logFileUnicodeString;

	 Interval.QuadPart = DELAY_ONE_MILLISECOND;
	 Interval.QuadPart *=Msec;

	// GetTime();
	// GetTime();

	// KeDelayExecutionThread(KernelMode, 0, &Interval);
	// GetTime();

	 RtlInitUnicodeString( &logFileUnicodeString, L"\\??\\C:\\1.LOG");
	 while(TRUE && EXIT){
	
		 while (!IsListEmpty(&HidePathListHeader))
		 {
			 LIST_ENTRY *pEntry = RemoveHeadList(&HidePathListHeader);
			 hideList = CONTAINING_RECORD(pEntry,LOG_LIST,listNode);
			 // KdPrint(("yiyi = %S",hideList->xxPath));
			 InitializeObjectAttributes(&objectAttributes,
				 &logFileUnicodeString,
				 OBJ_CASE_INSENSITIVE,//�Դ�Сд���� 
				 NULL, 
				 NULL );

			 //�����ļ�
			 status = ZwCreateFile( &hfile, 
				 FILE_APPEND_DATA,
				 &objectAttributes, 
				 &iostatus, 
				 NULL,
				 FILE_ATTRIBUTE_NORMAL, 
				 FILE_SHARE_READ,
				 FILE_OPEN_IF,//���ڸ��ļ���� ,�������򴴽�
				 FILE_SYNCHRONOUS_IO_NONALERT, 
				 NULL, 
				 0 );

			 if (!NT_SUCCESS(status))
			 {
				 KdPrint(("The file is not exist!\n"));
				 return;
			 }
			 UNICODE_STRING  p;
			 CHAR *Content;
			 Content="these words is my first write data \r\n����\rnihao\nhao";
			 RtlInitUnicodeString(&p,L"time = 1221 \r\n");
			// ZwWriteFile(hfile,NULL,NULL,NULL,&iostatus,p.Buffer,p.Length,NULL,NULL);

			 ZwWriteFile(hfile,NULL,NULL,NULL,&iostatus,hideList->xxPath,strlen(hideList->xxPath),NULL,NULL);
			 //�ر��ļ����
			 ZwClose(hfile);
			 ExFreePool(hideList);
		
		 }
	 
	 }
	// KeSetEvent(pEvent,IO_NO_INCREMENT,FALSE);

	 KdPrint(("�̺߳�������\n"));




	 /*if (!IsListEmpty(&HidePathListHeader))
	 {
		 for (pListNode = HidePathListHeader.Flink; pListNode!=&HidePathListHeader; pListNode = pListNode->Flink)
		 {
			 hideList = CONTAINING_RECORD(pListNode,HIDE_PATH_LIST,listNode);
			 if (hideList->xxPath)
			 {
				 KdPrint(("hideList->xxPath =%S \n",hideList->xxPath));
				 KeAcquireSpinLock(&HidePathListLock,&Irql);
				 RemoveEntryList(&hideList->listNode);
				 ExFreePool(hideList);	
				 KeReleaseSpinLock(&HidePathListLock,Irql);

			 }
		 }
	 }*/






	 //�����Լ�
     PsTerminateSystemThread(STATUS_SUCCESS);   
	 return ;
 }  


 //RtlStringCbPrintfW(tmp, 1024, L"%x", headStream.fileHead);

 //char to Hex(16����)

 //LENGTH_READ = 40
 BOOLEAN  CharToHex(CHAR C[] ,CHAR Hex[])
 {
    CHAR tmp[LENGTH_READ] = {0};
	__try{
	      for(int i = 0; i < strlen(C);i++)
	        { 
			 RtlStringCbPrintfA(tmp,LENGTH_READ,"%x",C[i]);
		    // strcat(Hex,tmp);
			 RtlStringCbCatA(Hex,LENGTH_READ,tmp);
	       }
		 // KdPrint(("%s",Hex));
		  return TRUE;
	    }
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return FALSE;
	}
 }