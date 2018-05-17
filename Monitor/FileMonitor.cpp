///////////////////////////////////////////////////////////////////////////////
///

/// Author(s)        : icedxu
///
/// Purpose          : �ļ����
///
/// Revisions:
///  0000 [2017-05-02] Initial revision.
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

#include "FileEncrypt.h"
#include "tools.h"

#define PAGEDCODE   code_seg("PAGE")  
#define LOCKEDCODE  code_seg() 
 

/************************************************************************/
/* ȫ�ֱ���                                                              */
/************************************************************************/
//
//  This is a lookAside list used to allocate our pre-2-post structure.
//
NPAGED_LOOKASIDE_LIST Pre2PostContextList;

//��������ƫ��
ULONG  ProcessNameOffset = 0;

//minifilter ���
//PFLT_FILTER gFilterHandle;

//�ͻ��˾�����Ժ�����
PFLT_PORT gClientPort;

//��������ͷ
PTYPE_KEY_PROCESS key_word_header;

//ȫ�ֿ���
BOOLEAN IS_SYSTEM_OPEN =FALSE;



//ͨ�Ŷ˿ھ��
PFLT_PORT serverPort=NULL;



#ifdef __cplusplus
extern "C" {
#endif

	////�������
	NTSTATUS DriverEntry(
		IN OUT PDRIVER_OBJECT   DriverObject,
		IN PUNICODE_STRING      RegistryPath
		)
	{
		NTSTATUS status;
		PSECURITY_DESCRIPTOR  sd;
		OBJECT_ATTRIBUTES oa;
		UNICODE_STRING portName=RTL_CONSTANT_STRING(SERVER_PORT_NAME);
			
		KdPrint(("DriverEntry \n"));

		//���˵�ϵͳ�Դ���һЩ����
	//	CHAR StrategyString[]="System;svchost.exe;explorer.exe;vmtoolsd.exe;";
	//key_word_header = GetStrategyFromString(StrategyString);
	
		
	

		//��ȡ��������ƫ��
		ProcessNameOffset=GetProcessNameOffset();

		PsSetCreateProcessNotifyRoutine(MyMiniFilterProcessNotify, FALSE);
		PsSetLoadImageNotifyRoutine(MyMiniFilterLoadImage);//

		InitializeListHead(&HidePathListHeader);
		//KeInitializeSpinLock(&HidePathListLock);


	


		//��ʼ��Lookaside����,����ҳ
		ExInitializeNPagedLookasideList( &Pre2PostContextList,
			NULL,
			NULL,
			0,
			sizeof(PRE_2_POST_CONTEXT),
			PRE_2_POST_TAG,
			0 );

		//ע��
		status=FltRegisterFilter(DriverObject,
								&FilterRegistration,
								&gFilterHandle);
			
		ASSERT(NT_SUCCESS(status));		
		if (NT_SUCCESS(status))
		{
			//����������
			status=FltStartFiltering(gFilterHandle);
			if(!NT_SUCCESS(status))
			{
				ExDeleteNPagedLookasideList( &Pre2PostContextList );
				FltUnregisterFilter(gFilterHandle);
			}

			//������ͨ�����
			status = FltBuildDefaultSecurityDescriptor(&sd,FLT_PORT_ALL_ACCESS);
			if (!NT_SUCCESS(status))
			{
				////////////DbgPrint("ͨ�Ŷ˿�������Ĭ�ϰ�ȫ��ʧ��\n");
				return status;
			}
			InitializeObjectAttributes(&oa,
										&portName,
										OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE,
										NULL,
										sd);
				//ע��ͨ�Ŷ˿�
			status = FltCreateCommunicationPort(gFilterHandle,
												&serverPort,
												&oa,
												NULL,
												MyConnectionCallback,
												MyDisconnectCallback,
												MyMessageCallback,
												SERVER_MAX_COUNT
												);

			if (!NT_SUCCESS(status))
			{
				////////////DbgPrint("ע��������˿�ʧ�� \n");
				ExDeleteNPagedLookasideList( &Pre2PostContextList );
				FltUnregisterFilter(gFilterHandle);
			}
				FltFreeSecurityDescriptor( sd );
		} 
			StartThread();

		return status;
	}

#ifdef __cplusplus
}; // extern "C"
#endif




///////////////////////////////Create/////////////////////////////////////


FLT_PREOP_CALLBACK_STATUS
CreatePre(
			__inout PFLT_CALLBACK_DATA Data,
			__in PCFLT_RELATED_OBJECTS FltObjects,
			__deref_out_opt PVOID *CompletionContext
			)
{

	return  FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


#pragma  LOCKEDCODE
FLT_POSTOP_CALLBACK_STATUS
CreatePost(
			 __inout PFLT_CALLBACK_DATA Data,
			 __in PCFLT_RELATED_OBJECTS FltObjects,
			 __in PVOID CompletionContext,
			 __in FLT_POST_OPERATION_FLAGS Flags
			 )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	PSTREAM_HANDLE_CONTEXT streamCtx = NULL;
	NTSTATUS status;
	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue;
	}
	__try{
		//�¾������жϴ��ļ������Ǵ������򿪵�
		UCHAR create_options = (UCHAR)((Data->Iopb->Parameters.Create.Options>>24)&0xff);
		if (create_options == FILE_CREATE)
		{ 	
			/*�½��ļ�*/
			    STREAM_HANDLE_CONTEXT temCtx;	
				status = GetFileInformation(Data,FltObjects,&temCtx);

				if (NT_SUCCESS(status))
				{
					PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
					PEPROCESS  p = FltGetRequestorProcess(Data);
					ULONG ProcessId = FltGetRequestorProcessId(Data);  
					ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread); 

					UINT32 Pid = 0 , PPid = 0;
					EnumProcess(ProcessId,&Pid,&PPid);
					//KdPrint(("Pid = %d,PPid = %d \n",Pid,PPid));

					//��ȡϵͳ����ʱ�䣬�˺�������ֵ�ѱ�����ֻ���ؿ����������ڵ����������Է�����־�Ŀ�ͷ
			
					//	KdPrint((" ThreadId = %u \n",ThreadId));
					ULONG Time = GetTime();
					CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

					IntegerToChar(Time,T);
					IntegerToChar(PPid,PPID);
					IntegerToChar(Pid,PID);
					//KdPrint(("%s \n",T));
					//KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

					CHAR FileName[260] ={0}; 

					NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
					//KdPrint((" FileName = %s",FileName));
					//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
					//CHAR STR[260] = {"T="};

						CHAR STR[260] = "XXX;";

					if (!IsSecretProcess(procName))
					{
						/*strcat(STR,T);strcat(STR,";OP=1;C="); 
						strcat(STR,procName);strcat(STR,";PID=");
						strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
						strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");*/

						strcat(STR,T);strcat(STR,";1;"); 
						strcat(STR,procName);strcat(STR,";");
						strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
						strcat(STR,FileName);strcat(STR,"\r\n");
						

						PLOG_LIST pathListNode ,pathList;
						pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
						if (pathListNode == NULL)
						{
							KdPrint(("��������ʧ��  \n"));  
						}
						//wcscpy(pathListNode->xxPath,pszDest);
						RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
						InsertTailList(&HidePathListHeader,&pathListNode->listNode);//�����β

					}

				

					/*KdPrint(("�ļ����� = %d,�ļ�·��:= %d ", temCtx.fileStyle.Length,temCtx.fileFullPath.Length));
					KdPrint(("���ھ�:= %d ��Ŀ¼ = %d\n", temCtx.fileVolumeName.Length,temCtx.fileName.Length));
					KdPrint(("�ļ�·��:= %wZ , �ļ����� = %wZ", &temCtx.fileFullPath,&temCtx.fileStyle));	
					KdPrint(("���ھ� =%wZ,��Ŀ¼=%wZ \n ",&temCtx.fileVolumeName,&temCtx.fileName));	*/

				
				} 
			
		}
	}	
	__except(EXCEPTION_EXECUTE_HANDLER)
	{

	}
	return retValue;
}



///////////////////////////////Write/////////////////////////////////////
#pragma  LOCKEDCODE
FLT_PREOP_CALLBACK_STATUS
	WritePre(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
	)
{
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return retValue; 
	}
	PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
	//PEPROCESS  p = FltGetRequestorProcess(Data);
	//ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread); 

	ULONG ProcessId = FltGetRequestorProcessId(Data);  
	UINT32 Pid = 0 , PPid = 0;
	EnumProcess(ProcessId,&Pid,&PPid);
	KdPrint(("Pid = %d,PPid = %d \n",Pid,PPid));



	
	

	
	return retValue;
}


#pragma  LOCKEDCODE
FLT_POSTOP_CALLBACK_STATUS
	WritePost(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
	)
{
	PPRE_2_POST_CONTEXT p2pCtx = (PPRE_2_POST_CONTEXT)CompletionContext;
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );
	NTSTATUS status;
	STREAM_HANDLE_CONTEXT temCtx;	
	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	__try{

	status = GetFileInformation(Data,FltObjects,&temCtx);
	
if (NT_SUCCESS(status))
		{
			PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
			PEPROCESS  p = FltGetRequestorProcess(Data);
			ULONG ProcessId = FltGetRequestorProcessId(Data);  
			ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread);  

			
			UINT32 Pid = 0 , PPid = 0;
			EnumProcess(ProcessId,&Pid,&PPid);

			ULONG Time = GetTime();
			CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

			IntegerToChar(Time,T);
		    IntegerToChar(PPid,PPID);
		    IntegerToChar(Pid,PID);
			//KdPrint(("%s \n",T));
		   // KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

	    	CHAR FileName[260] ={0}; 

			NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
			//KdPrint((" FileName = %s",FileName));
			//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
			//CHAR STR[260] = {"T="};
			CHAR STR[260] = "XXX;";



			if (!IsSecretProcess(procName))
			{

				/*	strcat(STR,T);strcat(STR,";OP=4;C="); strcat(STR,procName);strcat(STR,";PID=");
				strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
				strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");*/
				//KdPrint(("%s",STR));


				strcat(STR,T);strcat(STR,";4;"); 
				strcat(STR,procName);strcat(STR,";");
				strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
				strcat(STR,FileName);strcat(STR,"\r\n"); 


				PLOG_LIST pathListNode ,pathList;
				pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
				if (pathListNode == NULL)
				{
					KdPrint(("��������ʧ��  \n"));  
				}
				//wcscpy(pathListNode->xxPath,pszDest);
				RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
				InsertTailList(&HidePathListHeader,&pathListNode->listNode);//�����β



			}
			
					//KdPrint(("�ļ����� = %d,�ļ�·��:= %d ", temCtx.fileStyle.Length,temCtx.fileFullPath.Length));
					//KdPrint(("���ھ�:= %d ��Ŀ¼ = %d\n", temCtx.fileVolumeName.Length,temCtx.fileName.Length));
					//KdPrint(("�ļ�·��:= %wZ , �ļ����� = %wZ", &temCtx.fileFullPath,&temCtx.fileStyle));	
					//KdPrint(("���ھ� =%wZ,��Ŀ¼=%wZ \n ",&temCtx.fileVolumeName,&temCtx.fileName));	

		
	//		ULONG replyLength;  
	//		SCANNER_REPLY   Reply = {0};  
	//		replyLength = sizeof(SCANNER_REPLY);  

	//		PSCANNER_NOTIFICATION notification =(PSCANNER_NOTIFICATION) ExAllocatePool(NonPagedPool,sizeof(SCANNER_NOTIFICATION)); 
	////		if (notification == NULL)return ;  
	//		RtlZeroMemory(notification, sizeof(SCANNER_NOTIFICATION));  
	//		//notification->bCreate = Create;  
	//		RtlCopyMemory(notification->ProcessName, pName,strlen(pName)+1);

	//		status = FltSendMessage(gFilterHandle,   //���
	//			                    &gClientPort, //�ͻ��˶˿�
	//			                    notification,//���ͻ���
	//			                    sizeof(SCANNER_NOTIFICATION), //���ͻ������Ĵ�С
	//								&Reply,
	//								&replyLength,
	//								NULL
	//								);

	//		if (NT_SUCCESS(status))  
	//			{  
	//			  KdPrint(("���ͳɹ�  %d\n", replyLength));  
	//			}  
	//		else  
	//			{  
	//				KdPrint(("����ʧ��  status = %08x\n",status));  
	//			}  



		   /* CHAR pszDest[30];
		    ULONG cbDest = 30;
			LPCSTR pszFormat = "%s %d + %d = %d.";
			CHAR* pszTxt = "The answer is";

			RtlStringCbPrintfA(pszDest, cbDest, pszFormat, pszTxt, 1, 2, "3\n");*/
		//	KdPrint(("%s",pszDest));




			



			//�����¼�Ϊ���źţ�֪ͨ  
			//KeSetEvent(g_pEventObject, 0, FALSE);  


			//if (pathListNode == NULL)
			//{
			//	return   FLT_POSTOP_FINISHED_PROCESSING;;
			//}
			//wcscpy(pathListNode->xxPath,L"��ã�");
			//KeAcquireSpinLock(&HidePathListLock,&Irql);
			//InsertTailList(&HidePathListHeader,&pathListNode->listNode);
			//KeReleaseSpinLock(&HidePathListLock,Irql);

		} 
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{

		}

	return FLT_POSTOP_FINISHED_PROCESSING;
}








////////////////////////////////SetInformation//////////////////////////////////
/****
 *�����IRP�п��ж���������ɾ������
 **/
#pragma  LOCKEDCODE
FLT_PREOP_CALLBACK_STATUS
SetInformationPre(
					__inout PFLT_CALLBACK_DATA Data,
					__in PCFLT_RELATED_OBJECTS FltObjects,
					__deref_out_opt PVOID *CompletionContext
					)
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	STREAM_HANDLE_CONTEXT temCtx;
	NTSTATUS status;

	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	__try{

	//��ȡ�ļ���Ϣ
	status = GetFileInformation(Data,FltObjects,&temCtx);
	if (NT_SUCCESS(status))
		{

			PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
			PEPROCESS  p = FltGetRequestorProcess(Data);



			ULONG ProcessId = FltGetRequestorProcessId(Data);  
			ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread);
			HANDLE ID =  PsGetCurrentProcessId();


			UINT32 Pid = 0 , PPid = 0;
			EnumProcess(ProcessId,&Pid,&PPid);
			


			if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation)
			{
				if (!IsSecretProcess(procName))
				{
					//��ȡϵͳ����ʱ�䣬�˺�������ֵ�ѱ�����ֻ���ؿ����������ڵ����������Է�����־�Ŀ�ͷ
					ULONG Time = GetTime();
					CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

					IntegerToChar(Time,T);
					IntegerToChar(PPid,PPID);
					IntegerToChar(Pid,PID);
					//KdPrint(("%s \n",T));
					//KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

					CHAR FileName[260] ={0}; 

					NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
					//KdPrint((" FileName = %s",FileName));
					//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
					//CHAR STR[260] = {"T="};
					CHAR STR[260] = "XXX;";

					/*strcat(STR,T);strcat(STR,";OP=2;C="); strcat(STR,procName);strcat(STR,";PID=");
					strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
					strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");*/
					//KdPrint(("%s",STR));

					strcat(STR,T);strcat(STR,";2;"); 
					strcat(STR,procName);strcat(STR,";");
					strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
					strcat(STR,FileName);strcat(STR,"\r\n"); 

				

				   PLOG_LIST pathListNode ,pathList;
				   pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
				   if (pathListNode == NULL)
				   {
					   KdPrint(("���г�ʼ��ʧ��  \n"));  
				   }
				   RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
				   InsertTailList(&HidePathListHeader,&pathListNode->listNode);//�����β

				   //�����¼�Ϊ���źţ�֪ͨ  
				 //  KeSetEvent(g_pEventObject, 0, FALSE); 

				}
		   }


			if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileRenameInformation)
			{
				PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
				if (!IsSecretProcess(procName))
				{
					//��ȡϵͳ����ʱ�䣬�˺�������ֵ�ѱ�����ֻ���ؿ����������ڵ����������Է�����־�Ŀ�ͷ
					ULONG Time = GetTime();
					CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

					IntegerToChar(Time,T);
					IntegerToChar(PPid,PPID);
					IntegerToChar(Pid,PID);

					//KdPrint(("%s \n",T));
					//KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

					CHAR FileName[260] ={0}; 

					NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
					//KdPrint((" FileName = %s",FileName));
					//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
					//CHAR STR[260] = {"T="};
					CHAR STR[260] = "XXX;";

					/*strcat(STR,T);strcat(STR,";OP=3;C="); strcat(STR,procName);strcat(STR,";PID=");
					strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
					strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");*/
				//	KdPrint(("%s",STR));
					strcat(STR,T);strcat(STR,";3;"); 
					strcat(STR,procName);strcat(STR,";");
					strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
					strcat(STR,FileName);strcat(STR,"\r\n"); 



					PLOG_LIST pathListNode ,pathList;
					pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
					if (pathListNode == NULL)
					{
						KdPrint(("���г�ʼ��ʧ��  \n"));  
					}
					RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
					InsertTailList(&HidePathListHeader,&pathListNode->listNode);//�����β

				}
			}

					/*KdPrint(("�ļ����� = %d,�ļ�·��:= %d ", temCtx.fileStyle.Length,temCtx.fileFullPath.Length));
					KdPrint(("���ھ�:= %d ��Ŀ¼ = %d\n", temCtx.fileVolumeName.Length,temCtx.fileName.Length));
					KdPrint(("�ļ�·��:= %wZ , �ļ����� = %wZ", &temCtx.fileFullPath,&temCtx.fileStyle));	
					KdPrint(("���ھ� =%wZ,��Ŀ¼=%wZ \n ",&temCtx.fileVolumeName,&temCtx.fileName));	*/

				
			
		} 
		}	
		__except(EXCEPTION_EXECUTE_HANDLER)
	    {

		}
	return retValue;
}

#pragma  LOCKEDCODE
FLT_POSTOP_CALLBACK_STATUS
SetInformationPost(
					 __inout PFLT_CALLBACK_DATA Data,
					 __in PCFLT_RELATED_OBJECTS FltObjects,
					 __in PVOID CompletionContext,
					 __in FLT_POST_OPERATION_FLAGS Flags
					 )
{

	return FLT_POSTOP_FINISHED_PROCESSING;
}


///////////////////////////ж�غ���/////////////////////////////////
#pragma  LOCKEDCODE
NTSTATUS
FilterUnload (
			  __in FLT_FILTER_UNLOAD_FLAGS Flags
			  )
{

	UNREFERENCED_PARAMETER( Flags );
	//FreeStrategy(key_word_header);	
	FltCloseCommunicationPort(serverPort);

	PsSetCreateProcessNotifyRoutine(MyMiniFilterProcessNotify, TRUE);
	PsRemoveLoadImageNotifyRoutine(MyMiniFilterLoadImage);
	ExDeleteNPagedLookasideList( &Pre2PostContextList );
	FltUnregisterFilter( gFilterHandle );
	return STATUS_SUCCESS;
}
















//////////ignore
#pragma  LOCKEDCODE
NTSTATUS
InstanceSetup (
			   __in PCFLT_RELATED_OBJECTS FltObjects,
			   __in FLT_INSTANCE_SETUP_FLAGS Flags,
			   __in DEVICE_TYPE VolumeDeviceType,
			   __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
			   )
{
	UNREFERENCED_PARAMETER( Flags );
	UNREFERENCED_PARAMETER( VolumeDeviceType );
	UNREFERENCED_PARAMETER( VolumeFilesystemType );
	////KdPrind(("InstanceSetup\n"));
	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
#pragma  LOCKEDCODE
VOID 
CleanupStreamHandleContext(
						   __in PFLT_CONTEXT Context,
						   __in FLT_CONTEXT_TYPE ContextType				 
						   )
{

	UNREFERENCED_PARAMETER( ContextType );

	switch(ContextType)
	{
	case  FLT_STREAMHANDLE_CONTEXT:
		{
			////KdPrind(("����FLT_STREAMHANDLE_CONTEXT \n"));
			break;

		}
	case FLT_VOLUME_CONTEXT:
		{
			PVOLUME_CONTEXT VolCtx =(PVOLUME_CONTEXT)Context;
			////KdPrind(("����FLT_VOLUME_CONTEXT \n"));
			if (VolCtx->Name.Buffer != NULL) 
			{
				ExFreePool(VolCtx->Name.Buffer);
				VolCtx->Name.Buffer = NULL;
			}
			break;
		}

	case FLT_INSTANCE_CONTEXT:
		{
			////KdPrind(("���� FLT_INSTANCE_CONTEXT"));
			break;
		}
			
	case FLT_FILE_CONTEXT:
		{
		//	//KdPrind(("���� FLT_FILE_CONTEXT"));
			break;
			}
			
	case FLT_STREAM_CONTEXT:
		{
			////KdPrind(("���� FLT_STREAM_CONTEXT"));
			break;
			}
		
	case FLT_TRANSACTION_CONTEXT:
		{
			////KdPrind(("���� FLT_TRANSACTION_CONTEXT"));
			break;
			}
			
	case FLT_CONTEXT_END:
		{
			////KdPrind(("���� FLT_CONTEXT_END  "));
			break;
		}
	default:
		{
			////KdPrind(("����default\n"));
			break;
		}
	}


	////KdPrind(("CleanupStreamHandleContext�뿪\n"));
}

//////////////////////////////////////////////////////////////////////////

NTSTATUS
InstanceQueryTeardown (
					   __in PCFLT_RELATED_OBJECTS FltObjects,
					   __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
					   )

{

	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );
	//����жϼ�
	PAGED_CODE();	
	//KdPrind(("����InstanceQueryTeardown \n"));
	return STATUS_SUCCESS;
}

/************************************************************************/
/*                    ͨ�ſڻص�����                                    */
/************************************************************************/

//���ӻص�
NTSTATUS
MyConnectionCallback(
					 __in PFLT_PORT ClientPort,
					 __in_opt PVOID ServerPortCookie,
					 __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
					 __in ULONG SizeOfContext,
					 __deref_out_opt PVOID *ConnectionPortCookie
					 )
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER( ServerPortCookie );
	UNREFERENCED_PARAMETER( ConnectionContext );
	UNREFERENCED_PARAMETER( SizeOfContext);
	UNREFERENCED_PARAMETER( ConnectionPortCookie );
	ASSERT(gClientPort == NULL);
	//KdPrind(("Connect\n"));
	gClientPort = ClientPort; //���湩�Ժ�ʹ��
	return STATUS_SUCCESS;
}


//�رջص�
VOID
MyDisconnectCallback (
					  __in_opt PVOID ConnectionCookie
					  )
{
	//KdPrind(("MyDisconnectCallback"));
	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	//KdPrind(("Disconnect\n"));
	//�ر�ͨ������
	FltCloseClientPort(gFilterHandle , &gClientPort);
}

#pragma  LOCKEDCODE
//��Ϣ�ص�
NTSTATUS
MyMessageCallback (
				   __in_opt PVOID PortCookie,
				   __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
				   __in ULONG InputBufferLength,
				   __out_bcount_part_opt(OutputBufferLength,*ReturnOutputBufferLength) PVOID OutputBuffer,
				   __in ULONG OutputBufferLength,
				   __out PULONG ReturnOutputBufferLength
				   )
{
	
	 PAGED_CODE();

	 UNREFERENCED_PARAMETER( PortCookie );
	 UNREFERENCED_PARAMETER( OutputBufferLength );
	 UNREFERENCED_PARAMETER(InputBuffer);
	 UNREFERENCED_PARAMETER(InputBufferLength);

	  WCHAR *p;
	 __try{
		  
		 p = (PWCHAR)InputBuffer;
		 if (InputBuffer != NULL)
		 {
			 //KdPrint(("�û���������Ϣ�ǣ� %S\n",p));
			 KdPrint(("�û���������Ϣ�ǣ� %S \n",InputBuffer));
			 KdPrint(("InputBufferLength = %d \n",InputBufferLength));
		 }	
	 }
	 __except(EXCEPTION_EXECUTE_HANDLER){
        KdPrint(("%s \n",p));
   }
	return STATUS_SUCCESS;

}



VOID 
	MyMiniFilterLoadImage( __in_opt PUNICODE_STRING FullImageName, __in HANDLE ProcessId, __in PIMAGE_INFO ImageInfo )
{
	UNREFERENCED_PARAMETER(ImageInfo);

	if (FullImageName)
	{
		//DbgPrint("MyMiniFilterLoadImage, image name: %wZ, pid: %d\n", FullImageName, ProcessId);
	}
//	else
		//DbgPrint("MyMiniFilterLoadImage, image name: null, pid: %d\n", ProcessId);
}




VOID
	MyMiniFilterProcessNotify(
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create
	)
{
	//DbgPrint("MyMiniFilterProcessNotify, pid: %d, tid: %d, create: %d\n", ParentId, ProcessId, Create);
}

//ö��ָ�����̵��߳�
VOID EnumThread(PEPROCESS Process)
{
	ULONG i = 0, c = 0;
	PETHREAD ethrd = NULL;
	PEPROCESS eproc = NULL;
	for (i = 4; i<262144; i = i + 4)
	{
		ethrd = LookupThread((HANDLE)i);
		if (ethrd != NULL)
		{
			//����߳���������
			eproc = IoThreadToProcess(ethrd);
			if (eproc == Process)
			{
				//��ӡ�� ETHREAD �� TID
				DbgPrint("ETHREAD=%p, TID=%ld\n",
					ethrd,
					(ULONG)PsGetThreadId(ethrd));
			}
			ObDereferenceObject(ethrd);
		}
	}
}


//�����߳� ID �����߳� ETHREAD��ʧ�ܷ��� NULL
PETHREAD LookupThread(HANDLE Tid)
{
	PETHREAD ethread;
	if (NT_SUCCESS(PsLookupThreadByThreadId(Tid, &ethread)))
		return ethread;
	else
		return NULL;
}