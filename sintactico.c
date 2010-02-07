#define _POSIX_SOURCE
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lexico.h"
#include "sintactico.h"

#define ST_INIT 	0
#define ST_ARGS 	1
#define ST_FENT 	2
#define ST_FSAL 	3
#define ST_PIPE 	4
#define ST_FRED 	5
#define ST_ERR  	6
#define ST_APP  	7
#define ST_STDERR 	8

static void reset_cmd(CMD_t *cmd) { memset(cmd, 0, sizeof(CMD_t));}
static int es_numero(char *str) {
   int i;
   int len=strlen(str);
   for (i=0; i<len; i++) if (!isdigit(str[i])) return 0;
   return 1;
}

/* 
   retorna 0 en caso de funcionar correctamente, -1 en caso de error .
   En caso de éxito, retorna la estructura "cmd" rellenada con la orden
   a ejecutar.
*/

int obtener_orden (char *s, CMD_t *cmd) {
   int pipecnt=-1; // Cuenta de tuberías
   int argcnt=0; // Cuenta de argumentos por tubería
   int cnt; // Cuenta de tokens
   int ntoks; // Número de tokens
   int estado=ST_INIT;

   reset_cmd(cmd);
   strcpy (cmd->buffer, s);
   trocea (cmd->buffer);
   ntoks = numero_de_tokens();

   for (cnt=0; cnt<ntoks && estado!=ST_ERR; cnt++) {
      switch (estado) {
      case ST_INIT:
	 estado = ST_ERR;
	 if (tipo_token(cnt) == TOK_NOMBRE) {
	    if (es_numero(token_str(cnt))) {
	       cmd->time=atoi(token_str(cnt));
	       estado = ST_PIPE;
	    } 
	 }
	 break;

      case ST_ARGS:
	 switch (tipo_token(cnt)) {
	 case TOK_NOMBRE:
	    cmd->args[pipecnt][++argcnt] = token_str(cnt);
	    break;
	 case TOK_MAYOR:
	    estado=ST_FSAL;break;
	 case TOK_MENOR:
	    estado=ST_FENT;break;
	 case TOK_TUBO:
	    estado=ST_PIPE;break;
	 case TOK_APPEND:
	    estado=ST_APP;break;
	 case TOK_STDERR:
	    estado=ST_STDERR;break;
	 default:
	    estado=ST_ERR;break;
	 }
	 break;

      case ST_PIPE:
	 estado=ST_ERR;
	 if (tipo_token(cnt) == TOK_NOMBRE) {
	    cmd->args[++pipecnt][argcnt=0]=token_str(cnt);
	    estado=ST_ARGS;
	 } 
	 break;

      case ST_FENT:
	 estado=ST_ERR;
	 if (tipo_token(cnt) == TOK_NOMBRE && !cmd->fent) {
	    cmd->fent=token_str(cnt);
	    estado = ST_FRED;
	 }
	 break;

      case ST_FSAL:
	 estado=ST_ERR;
	 if (tipo_token(cnt) == TOK_NOMBRE && !cmd->fsal) {
	    cmd->fsal=token_str(cnt);
	    cmd->append=0;
	    estado = ST_FRED;
	 }
	 break;

      case ST_APP:
	 estado=ST_ERR;
	 if(tipo_token(cnt) == TOK_NOMBRE && !cmd->fsal) {
	    cmd->fsal=token_str(cnt);
	    cmd->append=1;
	    estado = ST_FRED;
	 }
	 break;

      case ST_STDERR:
	 estado=ST_ERR;
	 if(tipo_token(cnt) == TOK_NOMBRE && !cmd->fsalerr) {
	    cmd->fsalerr=token_str(cnt);
	    estado = ST_FRED;
	 }
	 break;
	 
      case ST_FRED:
	 estado=ST_ERR;
	 if ((tipo_token(cnt) == TOK_MAYOR) && (!cmd->fsal)) estado=ST_FSAL;
	 if ((tipo_token(cnt) == TOK_MENOR) && (!cmd->fent)) estado=ST_FENT;
	 if ((tipo_token(cnt) == TOK_STDERR) && (!cmd->fsalerr)) estado=ST_STDERR;
	 break;
      }
   }
   
   // Miro estados finales
   if (estado != ST_ARGS && estado != ST_FRED) estado = ST_ERR;

   return (estado==ST_ERR ? -1 : 0);
}
