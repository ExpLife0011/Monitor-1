
#ifndef __FILE_ENCRYPT_H_VERSION__
#define __FILE_ENCRYPT_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif


#include "drvcommon.h"
#include "drvversion.h"
#include "ntstrsafe.h"



/*************************************************************************
    Pool Tags
*************************************************************************/

#define BUFFER_SWAP_TAG     'bdBS'
#define CONTEXT_TAG         'xcBS'
#define NAME_TAG            'mnBS'
#define PRE_2_POST_TAG      'LASI'
#define STREAM_HANDLE_CONTEXT_TAG  'shBS'



#define PBUFFER_TAG  'BUF'
#define LENGTH_READ  40



//minifilter ���
extern  PFLT_FILTER gFilterHandle;
extern  HANDLE  handle;
//�ͻ��˾�����Ժ�����
extern  PFLT_PORT gClientPort;

//ͬ���¼�����  
extern PRKEVENT g_pEventObject;  
//�����Ϣ  
extern  OBJECT_HANDLE_INFORMATION g_ObjectHandleInfo;  


extern BOOLEAN EXIT;




extern LIST_ENTRY HidePathListHeader;
extern KSPIN_LOCK HidePathListLock;
#define _CMD_PATH 256


typedef struct _HIDE_PATH_LIST
{
	LIST_ENTRY listNode;
	CHAR xxPath[_CMD_PATH];
}LOG_LIST,*PLOG_LIST;




/*************************************************************************
    Local structures
*************************************************************************/

/************************************************************************/
/* ���ܲ��Ա�ṹ����                                                     */
/************************************************************************/
//���������Ϣ����
#define PROCESS_NAME_LEN      32
typedef struct _PROCESS_INFO
{
	CHAR processName[PROCESS_NAME_LEN];//��������

	_PROCESS_INFO *next;//��һ���ڵ�

} PROCESS_INFO,*PPROCESS_INFO;


//�������͹ؼ�������ṹ
#define TYPE_KEY_WORD_LEN      32
typedef struct _TYPE_KEY_PROCESS
{
	PPROCESS_INFO processInfo;//ƥ�����

	_TYPE_KEY_PROCESS *next;//��һ���ڵ�

} TYPE_KEY_PROCESS,*PTYPE_KEY_PROCESS;





/************************************************************************/
/* ����һ�������Ľṹ�����ڽ�״̬��Ԥ�����������                            */
/************************************************************************/

typedef struct _PRE_2_POST_CONTEXT {

	BOOLEAN IS_DECONGD; 
    PVOID   SwappedBuffer;  //�����Ƿ���Ļ�������ַ���ݸ�Post�����Ա����ͷ�

} PRE_2_POST_CONTEXT, *PPRE_2_POST_CONTEXT;



//������������,�ж��ļ�ͷ��Ϣ
typedef struct _STREAM_HEAD
{
	FILE_STANDARD_INFORMATION fileInfo;//�ļ���Ϣ

	CHAR  fileHead[40];//�ļ�ͷ

	BOOLEAN isRead;//�ļ��Ƿ񱻶���

} STREAM_HEAD,*PSTREAM_HEAD;














//�����������ģ���ȡ�ĸ����̲������ĸ��ļ���·�������ͣ�
typedef struct _STREAM_HANDLE_CONTEXT
{
	//FILE_STANDARD_INFORMATION fileInfo;//�ļ���Ϣ

	UNICODE_STRING  ParentDir;  //�ļ���

	UNICODE_STRING  fileFullPath ;   //�����ļ�·��

	UNICODE_STRING  fileVolumeName; //�ļ����ڵľ���

	UNICODE_STRING  fileStyle; //�ļ����� 

} STREAM_HANDLE_CONTEXT,*PSTREAM_HANDLE_CONTEXT;


//���� API
extern"C" __declspec(dllimport)UCHAR*PsGetProcessImageFileName(IN PEPROCESS Process); 
extern"C" __declspec(dllimport)HANDLE PsGetProcessInheritedFromUniqueProcessId(IN PEPROCESS Process); 



//���ݽ��� ID ���ؽ��� EPROCESS��ʧ�ܷ��� NULL
PEPROCESS LookupProcess(HANDLE Pid);
VOID EnumProcess(ULONG);

/************************************************************************/
/*                    ͨ�ſڻص�����                                      */
/************************************************************************/

//ö��ָ�����̵��߳�
VOID EnumThread(PEPROCESS Process);
PETHREAD LookupThread(HANDLE Tid);


VOID 
	MyMiniFilterLoadImage( 
	__in_opt PUNICODE_STRING FullImageName,
	__in HANDLE ProcessId,
	__in PIMAGE_INFO ImageInfo );


VOID
	MyMiniFilterProcessNotify(
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create
	);








//���ӻص�
NTSTATUS
	MyConnectionCallback(
	__in PFLT_PORT ClientPort,
	__in_opt PVOID ServerPortCookie,
	__in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID *ConnectionPortCookie
	);

//�رջص�
VOID
	MyDisconnectCallback (
	__in_opt PVOID ConnectionCookie
	);

//��Ϣ�ص�
NTSTATUS
	MyMessageCallback (
	__in_opt PVOID PortCookie,
	__in_bcount_opt(InputBufferLength) PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__out_bcount_part_opt(OutputBufferLength,*ReturnOutputBufferLength) PVOID OutputBuffer,
	__in ULONG OutputBufferLength,
	__out PULONG ReturnOutputBufferLength
	);

/*************************************************************************
Prototypes
*************************************************************************/

NTSTATUS
	InstanceSetup (
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_SETUP_FLAGS Flags,
	__in DEVICE_TYPE VolumeDeviceType,
	__in FLT_FILESYSTEM_TYPE VolumeFilesystemType
	);

VOID
	CleanupVolumeContext(
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
	);

VOID
	CleanupStreamHandleContext(
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
	);

NTSTATUS
	InstanceQueryTeardown (
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
	);


/************************************************************************/
/*        �ص�����                                                       */
/************************************************************************/
///IRP_MJ_CREATE
FLT_PREOP_CALLBACK_STATUS
	CreatePre(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
	CreatePost(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
	);


FLT_POSTOP_CALLBACK_STATUS
	ReadPost(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
	);



/////IRP_MJ_WRITE
FLT_PREOP_CALLBACK_STATUS
	WritePre(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
	);


FLT_POSTOP_CALLBACK_STATUS
	WritePost(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
	);





/////SetInformation
FLT_PREOP_CALLBACK_STATUS
	SetInformationPre(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
	SetInformationPost(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
	);


NTSTATUS
	FilterUnload (
	__in FLT_FILTER_UNLOAD_FLAGS Flags
	);


NTSTATUS GetFileInformation(__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__inout PSTREAM_HANDLE_CONTEXT ctx);




CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE,
	0,
	NULL,         //
	CreatePost 
	},
	


	//{ IRP_MJ_WRITE,
	//0,
	//NULL,
	//WritePost
	//},

/*
	{ IRP_MJ_SET_INFORMATION,   
	0,
	SetInformationPre,
	SetInformationPost },*/

	{ IRP_MJ_OPERATION_END }
};


//

//
CONST FLT_CONTEXT_REGISTRATION ContextNotifications[] = {

	{ FLT_STREAMHANDLE_CONTEXT,
	0,
	CleanupStreamHandleContext,
	sizeof(STREAM_HEAD),
	STREAM_HANDLE_CONTEXT_TAG },

	{ FLT_CONTEXT_END }
};




CONST FLT_REGISTRATION FilterRegistration = {

	sizeof( FLT_REGISTRATION ),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	ContextNotifications,			    //  Context
	Callbacks,                          //  Operation callbacks

	FilterUnload,                       //  MiniFilterUnload

	InstanceSetup,						//  InstanceSetup
	InstanceQueryTeardown,				//  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent

};

#endif 

































