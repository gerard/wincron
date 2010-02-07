#include <windows.h>
#include <stdio.h>

#define RECRON "recron"

int main(void) {
	DWORD basuraD;
	HANDLE hMSlot;

	hMSlot=CreateFile("//./mailslot/recron", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(!hMSlot) fprintf(stderr, "Error al obrir el Mailslot\n");
	
	// Qualsevol cadena deu servir
	WriteFile(hMSlot, RECRON, strlen(RECRON)+1, &basuraD, NULL);

	CloseHandle(hMSlot);
}

