#define _POSIX_SOURCE
#include <stdio.h>
#include <windows.h>

#include "sintactico.h"

#define LINIA_SIZE 1000
#define CMD_SIZE 100
#define N_INTERNS 42

#define DEBUG 1
#define SYSLOG 1


// Llista de comandes internes
const char llista_interns[N_INTERNS][8]={
	"assoc",
	"break",
	"calcs",
	"call",
	"cd",
	"chdir",
	"cls",
	"color",
	"copy",
	"date",
	"del",
	"dir",
	"echo",
	"endlocal",
	"erase",
	"exit",
	"for",
	"ftype",
	"goto",
	"if",
	"mkdir",
	"move",
	"path",
	"pause",
	"popd",
	"prompt",
	"pushd",
	"rd",
	"rem",
	"ren",
	"rename",
	"rmdir",
	"set",
	"setlocal",
	"shift",
	"start",
	"time",
	"title",
	"type",
	"ver",
	"verify",
	"vol"
};

// Variables Globals
FILE *cronout;						// Fitxer de logging

// Funcions de Error

void printmsg(char *msg) {
	LPTSTR e_buf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			      GetLastError(), 0, (LPTSTR)&e_buf, 0, NULL);
	fprintf(stderr, "%5d %s: %s", GetCurrentThreadId(), msg, e_buf);
	LocalFree(e_buf);
}

void fatal(char *msg) {
	printmsg(msg);
	exit(1);
}

// Esta funci� comprova, donat un conjunt d'ordres amb arguments que no es
// tracta de una ordre interna
int ComprovaInternaV(char *Lcmd[][MAX_ARGS]) {
	int i;
	
	for(i=0; Lcmd[i][0]; i++) {
		if(ComprovaInterna(Lcmd[i][0])) return 1;
	}

	return 0;
}

// Esta funci� comprova que una comanda no siga interna
int ComprovaInterna(char *comanda) {
	int i;

	for(i=0; i<N_INTERNS; i++) 
		if(strcmp(comanda, llista_interns[i])==0) return 1;

	return 0;
}

// Deixa en 'Lexec' la linia de execuci� completa i retorna
// el nombre de par�metres llegits
int LiniaExecucio(char **vParam, char *Lexec) {
	int i;

	for(i=0; vParam[i]; i++) {
		sprintf(Lexec, "%s%s ", Lexec, vParam[i]);
	}
	// A�� corregix la �ltima posici� de la cadena, que es queda a blanc
	Lexec[strlen(Lexec)-1]='\0';

	return i;
}

// Inicialitza el vector de handles a NULL, falta comprovar si aquest valor
// es correcte
void Handle_InicialitzarNULLs(HANDLE *v) {
	int i;

	for(i=0; i<MAXIMUM_WAIT_OBJECTS; i++) {
		v[i]=NULL;
	}
}

// Torna la primera posici� lliure del vector de Handles
DWORD Handle_Lliure(HANDLE *v) {
	DWORD i;

	for(i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
		if(v[i]==NULL) return i;
	
	// Alerta, si no es retorna dins del for s'est� maxacant un descriptor,
	// ja que no caben en el vector els descriptors.
	// Aquesta situaci� es l�mit, i dep�n de MAXIMUM_WAIT_OBJECTS, es a dir
	// de la capacitat de la funci� WaitForMultipleObjects. En W2000Pro son
	// 64 Handles. M�s que suficients per a un �s normal del servidor cron.
	// En 9x ni idea :P

	fprintf(stderr, "ALERT Alerta, vector de descriptors desbordat\n");
	fprintf(stderr, "ALERT Com a mal menor, es generaran zombies al sistema\n");
	return 0;
}

DWORD Handle_Nombre(HANDLE *v) {
	DWORD i;

	for(i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
		if(v[i]==NULL) return i;

	return MAXIMUM_WAIT_OBJECTS;
}

// Aquesta funci� ens ajudara a trobar errors en el vector de HANDLErs
void Handle_Dump(HANDLE *v) {
	DWORD i;

	for(i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
		if(v[i]==NULL) fprintf(stderr, "%c", ' ');
		else fprintf(stderr, "%c", 'H');

	fprintf(stderr, "%c", '\n');
}

// Aquesta funci� situa el �ltim handle del vector a la posici� marcada
// per aix� mantindre el vector uniforme
void Handle_RestauraV(HANDLE *v, DWORD elim) {
	DWORD i, ultim;

	for(i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
		if(v[i]!=NULL) ultim=i;
	
	v[elim]=v[ultim];
	v[ultim]=NULL;
}

DWORD WINAPI Executa(LPVOID arg) {
	char Lexec[LINIA_SIZE]="";
	char str_time[LINIA_SIZE];
	char linia_log[LINIA_SIZE];
	
	DWORD exitcode;
	DWORD rWait;
	CMD_t *cmd;

	DWORD id;						// Usarem el ThreadId per a identificar missatges de debugging
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	FILETIME f_time, f_dummy;
	SYSTEMTIME s_time;
	
	// Descriptors de Proc�s i fil
	HANDLE vHandleP[MAXIMUM_WAIT_OBJECTS];
	HANDLE vHandleT[MAXIMUM_WAIT_OBJECTS];
	HANDLE handleP, handleT;

	// Descriptors de redireccions
	HANDLE hFileIn, hFileOut;

	CRITICAL_SECTION mutex;

	// Fi de la declaraci� de variables
	id=GetCurrentThreadId();
	cmd=(CMD_t *)arg;
	GetStartupInfo(&si);
	LiniaExecucio(cmd->args[0], Lexec);
	
	InitializeCriticalSection(&mutex);
	Handle_InicialitzarNULLs(vHandleP);
	Handle_InicialitzarNULLs(vHandleT);
	
	// Comencem esperant
	Sleep(cmd->time*1000);

	while(1) {
		// Ara, les redireccions
		si.dwFlags=STARTF_USESTDHANDLES;
		si.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
		si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
		if(cmd->fsal) {
			hFileOut=CreateFile(cmd->fsal, GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, 0, NULL);
			// Pot fallar per permisos o perque no te la ruta
			if(hFileOut==INVALID_HANDLE_VALUE) fatal("CreateFile()");
			
			si.hStdOutput=hFileOut;
		}
		if(cmd->fent) {
			hFileIn=CreateFile(cmd->fent, GENERIC_READ, 0, &sa, OPEN_EXISTING, 0, NULL);
			// Si falla, el fitxer no existia
			if(hFileIn==INVALID_HANDLE_VALUE) fatal("CreateFile(): El fitxer no existeix, ");
			
			si.hStdInput=hFileIn;
		}
		
		// I l'execuci�
		if(DEBUG) printf("%5d Execucio: '%s'\n", id, Lexec);
		if(!CreateProcess(NULL, Lexec, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
			fatal("CreateProcess()");

		// Ara tanquem els descriptors d'obertura dels fitxers
		if(cmd->fsal) CloseHandle(hFileOut);
		if(cmd->fent) CloseHandle(hFileIn);

		vHandleP[Handle_Lliure(vHandleP)]=pi.hProcess;
		vHandleT[Handle_Lliure(vHandleT)]=pi.hThread;

		// Ara esperem el interval necessari
		Sleep(cmd->time*1000);
		
		// Si no es dona un timeout, aleshores, un proc�s ha acabat.
		// La �nica pega d'aquesta implementaci� es que nom�s s'esperen
		// fills una despr�s de la execuci� de la comanda i aquesta pot
		// tardar molt entre execuci�ns.
		if(DEBUG) printf("%5d Neteja de processos\n", id);
		while(Handle_Nombre(vHandleP)) {	// Mentres existixquen descriptors oberts
			if(DEBUG) printf("%5d Wait sobre %d objectes\n", GetCurrentThreadId(), Handle_Nombre(vHandleP));
			rWait=WaitForMultipleObjects(Handle_Nombre(vHandleP), vHandleP, FALSE, 0);

			// Si es dona un timeout, una altra vegada ser�
			if(rWait==WAIT_TIMEOUT) break;
			
			// Situaci� de error
			if(rWait==WAIT_FAILED) {
				Handle_Dump(vHandleP);
				Handle_Dump(vHandleT);
				fatal("WaitForMultipleObjects()");
			}

			// Tenim un proc�s al que fer un wait
			handleP=vHandleP[rWait-WAIT_OBJECT_0];
			handleT=vHandleT[rWait-WAIT_OBJECT_0];
			
			// Te�ricament, que es retorne STILL_ACTIVE es impossible
			GetExitCodeProcess(handleP, &exitcode);
			GetProcessTimes(handleP, &f_dummy, &f_time, &f_dummy, &f_dummy);
			FileTimeToSystemTime(&f_time, &s_time);
			
			if(DEBUG) printf("%5d Eliminant el descriptor del vector i del sistema\n", id);
			Handle_RestauraV(vHandleP, rWait-WAIT_OBJECT_0);
			Handle_RestauraV(vHandleT, rWait-WAIT_OBJECT_0);

			// Comprovem que el proc�s tanca els seus descriptors
			if(!CloseHandle(handleP)) fatal("CloseHandle()");
			if(!CloseHandle(handleT)) fatal("CloseHandle()");

			sprintf(str_time, "%d:%d:%d", s_time.wHour, s_time.wMinute, s_time.wSecond);
			sprintf(linia_log, "%s %s Term: %d\n", str_time, Lexec, exitcode);
		
			// Escriptura en el log
			EnterCriticalSection(&mutex);	// Protegim la escriptura en el log
			if(DEBUG) printf("%5d Escriptura al log: SC... ", id);
			fprintf(cronout, "%s", linia_log);
			if(DEBUG) printf("finalitzada\n");
			LeaveCriticalSection(&mutex);	// Eixim de la secci� cr�tica
			
			fflush(cronout);				// Exigim la escriptura al disc
		}
	}
}

int main(int argc, char *argv[]) {
	char linia[LINIA_SIZE];
	int i, n_linies;

	static CMD_t cmd[CMD_SIZE];		// Cal declarar aquest vector est�ticament ja que excedeix
									// el segment de la pila
	FILE *cronfile;					// Fitxer de configuraci�

	HANDLE *vFils;					// Descriptors dels fils
	DWORD *vFilsId;					// Identificadors dels fils
	HANDLE hTub;

	// Fi de declaraci� de variables, entrada de dades
	switch(argc) {
	case 1:	// S'obren els fitxers per defecte
		cronfile=fopen("conf\\cronfile", "r");
		cronout=fopen("conf\\cronout", "w");
		break;
	case 2: // S'especifica tan sols el fitxer de configuraci�
		cronfile=fopen(argv[1], "r");
		cronout=fopen("conf\\cronout", "w");
		break;
	case 3: // S'especifiquen el fitxer de configuraci� i el de log
		cronfile=fopen(argv[1], "r");
		cronout=fopen(argv[2], "w");
		break;
	default:	// Comprovaci� del nombre de par�metres
		fprintf(stderr, "Nombre incorrecte de par�metres\n");
		fprintf(stderr, "Us: %s <fitxer de conf> <fitxer de log>", argv[0]);
		exit(1);
	}
	
	// Fem una comprovaci� dels fitxers
	if(cronfile==NULL || cronout==NULL) {
		fprintf(stderr, "Hi ha hagut un error en l'obertura de fitxers\n");
		fprintf(stderr, "Assegure les rutes i els permisos\n");
		exit(1);
	}

	// Creem el costat servidor del recron. Implementat amb una tuberia
	// amb nom
	hTub=CreateNamedPipe("//./pipe/recron", PIPE_ACCESS_INBOUND, 0, 1, 0, 0, INFINITE, NULL);
	if(!hTub) fatal("Error al crear el tub");

	do {
		// Primer fem un "parsing" del fitxer de configuraci�
		for(i=0; fgets(linia, LINIA_SIZE, cronfile); i++) {
			// Llegim una linia del fitxer
			if(obtener_orden(linia, &(cmd[i]))==-1) {
				fprintf(stderr, "Error sintactic, revise el fitxer de configuracio\n");
				exit(1);
			}
		}
	
		// Tanquem el descriptor, ja no ens fa falta
		fclose(cronfile);
	
		// Ara sabem que tenim que crear 'i' fils d'execuci�
		n_linies=i;
		vFils=(HANDLE *)malloc(sizeof(HANDLE)*n_linies);
		vFilsId=(DWORD *)malloc(sizeof(DWORD)*n_linies);

		if(SYSLOG) printf("  LOG Escanejades %d ordres amb exit\n", n_linies);

		for(i=0; i<n_linies; i++) {
			// Comprovem que la linia no cont� ordres internes
			if(ComprovaInternaV(cmd[i].args)) {
				if(SYSLOG) printf("  LOG El fitxer de configuracio conte una ordre interna. Abortant l'execucio de la linia\n");
			} 
			// En cas contrari executem
			else {
				if(DEBUG) printf(" MAIN Creant fil %d\n", i);
				vFils[i]=CreateThread(NULL, 0, Executa, (LPVOID)(&(cmd[i])), 0, &(vFilsId[i]));
				if(vFils[i]==NULL) {
					printf(" MAIN Error en el CreateThread\n");
					exit(1);
				}
			}
		}
	
		// El programa deura reiniciar-se quan es reba una senyal desde el tub.
		// Idealment enviada pel programa recron
		// Simplement connectant-se al tub, el servidor ent�n que te que reiniciar el servidor
		if(ConnectNamedPipe(hTub, NULL)==FALSE) {
			if(SYSLOG) printmsg("   LOG Client malintencionat: Ha fet mal la conexi� al tub");
		}
		DisconnectNamedPipe(hTub);

		// Acabem amb tots els fils
		for(i=0; i<n_linies; i++) TerminateThread(vFils[i], 10);
		
		// Ara toca tornar a obrir el fitxer de entrar
		if(argc==1) cronfile=fopen("conf\\cronfile", "r");
		else cronfile=fopen(argv[1], "r");

		if(SYSLOG) printf("   LOG recron ha reiniciat el servidor\n");
		// I tot torna a la normalitat...
	} while(1);
	
	// Aqui no es deuria arribar mai
	exit(0);
}
