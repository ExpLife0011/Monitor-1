
/************************************************************************/
/* �˲�����Ҫ�ǵ��ú����Ľṹ��                                                                     */
/************************************************************************/
#ifndef __FILE_TOOLS_H_VERSION__
#define __FILE_TOOLS_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <string.h>

#ifdef __cplusplus
}; // extern "C"

#include "FileEncrypt.h"

#include <stdarg.h>
#endif



#define FAT_NTC_FCB               0x0502
#define FAT_NTC_DCB               0x0503
#define FAT_NTC_ROOT_DCB          0x0504
#define NTFS_NTC_DCB              0x0703
#define NTFS_NTC_ROOT_DCB         0x0704
#define NTFS_NTC_FCB              0x0705





/************************************************************************/
/*                           ���峣��                                 */
/************************************************************************/


//����ͨ�ſ�����
#define		SERVER_PORT_NAME	L"\\FileEncryptPort"

//����ͨ�ſ����������
#define		SERVER_MAX_COUNT	1



//������Ϣ�� ����
#define		CODE_OPEN_SYSTEM	0x0001	//��ϵͳ

#define		CODE_CLOSE_SYSTEM	0x0002	//�ر�ϵͳ

#define		CODE_SEND_STRATEGY	0x0003	//���Ͳ���

#define		CODE_SEND_KEY		0x0004	//��������

#define		CODE_IS_RUNNING		0x0005	//��ѯϵͳ�Ƿ�����


//����

#define		CODE_SUCCESS		0x0006	//�����ɹ�

#define		CODE_UNSUCCESS		0x0007  //�������ɹ�

#define		CODE_UNKNOW_CODE	0x0008	//����ָ��

#define		CODE_RUNNING		0x0009	//ϵͳ����

#define		CODE_CLOSED			0x000a	//ϵͳֹͣ


/************************************************************************/
/*                      �������ݽṹ                                     */
/************************************************************************/
//��Ϣ�ṹ��
typedef struct _MESSAGE_DATA
{
	INT32	code;                 //��Ϣ��

	INT32	bufferLen;            //����������

	CHAR	buffOffset[1];        //��������ʼ
} MESSAGE_DATA,*PMESSAGE_DATA;


//��Ϣ����
typedef struct _MESSAGE_BACK
{
	INT32 code;                   //������
} MESSAGE_BACK,*PMESSAGE_BACK;





PCHAR  GetCurrentTimeString();


ULONG	GetTime();
 VOID DbgKeLog(PCHAR lpszLog);

//���ַ����й���һ�����Ա����ر�ͷ
//PTYPE_KEY_PROCESS GetStrategyFromString(CHAR *StrategyString);

//�ͷ�һ�����Ա�
void FreeStrategy(PTYPE_KEY_PROCESS head);


//�жϽ������Ƿ�Ϊ�����͵Ļ��ܽ���
BOOLEAN IsSecretProcess(PTYPE_KEY_PROCESS keyWord,CHAR *processName);


/************************************************************************/
/*     �������                                                        */
/************************************************************************/
//��ȡ������ƫ��
ULONG 
GetProcessNameOffset(VOID);

//��ȡ��������
PCHAR
GetCurrentProcessName(ULONG ProcessNameOffset);



/************************************************************************/
/*                   �ַ�������                                             */
/************************************************************************/

//��wchar_t* ת��char*�ĺ�����
void wstr2cstr(const wchar_t *pwstr , char *pcstr, size_t len);

//��char* ת��wchar_t*��ʵ�ֺ������£�
void cstr2wstr( const char *pcstr,wchar_t *pwstr , size_t len);














#endif // 

























