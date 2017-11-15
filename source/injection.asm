;#Mode=CON

.386
.model flat, stdcall
option casemap :none

include windows.inc
include user32.inc
include kernel32.inc
include masm32.inc

includelib user32.lib
includelib kernel32.lib
includelib masm32.lib
include macro.asm

.data?
	buffer	db 256 dup(?)
	hGame		HWND ?
	hProc		HANDLE ?
	idGame	dd ?
	lpAddr	LPVOID ?
	pFunAddr FARPROC ?
	hThread 	HANDLE ?
.data
	dllPath  db "cd.dll",0
.CODE
START:
	invoke GetCurrentDirectory,MAX_PATH,addr buffer
	invoke szCatStr,addr buffer,addr dllPath
	
	invoke FindWindow,CTXT("Valve001"),CTXT("Counter-Strike")
	test eax,eax
	jz l1
	mov hGame,eax
	invoke GetWindowThreadProcessId,eax,addr idGame
	invoke OpenProcess,PROCESS_ALL_ACCESS,FALSE,idGame
	mov hProc,eax
	invoke VirtualAllocEx,eax,NULL,sizeof buffer,MEM_COMMIT or MEM_RESERVE,PAGE_EXECUTE_READWRITE
	mov lpAddr,eax
	invoke WriteProcessMemory,hProc,lpAddr,addr buffer,sizeof buffer,0
	
	invoke GetModuleHandle,CTXT("kernel32.dll")
	invoke GetProcAddress,eax,CTXT("LoadLibraryA")
	mov pFunAddr,eax
	invoke CreateRemoteThread,hProc,NULL,0,pFunAddr,lpAddr,0,NULL
	mov hThread,eax
	invoke WaitForSingleObject,eax,INFINITE
	jmp l2
l1:	invoke StdOut,CTXT("****请先运行游戏****")
	invoke StdIn,addr buffer,sizeof buffer
l2:	invoke ExitProcess,0
	
end START
