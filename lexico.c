#define _POSIX_SOURCE
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexico.h"


/* Este módulo sólo debería ser usado por el módulo léxico */

/* Variables y funciones PRIVADAS al módulo léxico */

static token_t token[MAX_TOKENS]; 
static char *nombre[MAX_TOKENS]; 
static int num_tokens=0;

static char especial[]="<>|";

static int separador(char c) {
   if (strchr(especial,c) != NULL) return 1;
   else return 0;
}

/* Funciones públicas */
/* solo tiene sentido usarlas desde un analizador sintáctico */

int trocea (char *s) {
   char c;
   int i=0;
   int len=strlen(s);

   num_tokens = 0;
   while (isspace(s[i])) i++;
  
   c=s[i];
	   
   while (i<len) {

      if ( num_tokens == MAX_TOKENS ) {
	 /* El proximo token ya no cabe. Error */
	 num_tokens = 0;
	 return -1;
      }
      
      switch (c) {
      case '|':
	 token[num_tokens] = TOK_TUBO;
	 c=s[++i];
	 break;
      case '>':
	 if(s[i+1]=='>') {
	    token[num_tokens] = TOK_APPEND;
	    c=s[++i]; // Adelantem una posició més
	 }
	 else token[num_tokens] = TOK_MAYOR;
	 c=s[++i];
	 break;
      case '<': 
	 token[num_tokens] = TOK_MENOR; c=s[++i];
	 break;
      case '2':
	 // Nomes serà un token si es tracta de 2>, així que en altre cas no hi ha break
	 if(s[i+1]=='>') {
	    token[num_tokens] = TOK_STDERR;
	    i=i+2;
	    c=s[i];
	    break;
	 }
      default: 
	 token[num_tokens] = TOK_NOMBRE;
	 nombre[num_tokens] = &s[i++];
	 while (!separador(s[i]) && !isspace(s[i])) i++;
	 c=s[i];
	 s[i] = '\0';
	 if (!separador(c)) c=s[++i];
      }
      while (isspace(s[i])) c=s[++i];
      num_tokens++;
   }
   return 0;
}

token_t tipo_token (int i) {
  if ( i >= 0 && i < num_tokens) {
    return token[i];
  } else {
    return -1;
  }
}

char * token_str (int i) {
  if (i >= 0 && i < num_tokens) {
    return nombre[i];
  } else {
    return NULL;
  }
}

int numero_de_tokens() { return num_tokens;}
