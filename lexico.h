/* fichero:     lexico.h

   descripcion: 

   Descompone una cadena de caracteres en tokens
   Un token es una palabra o uno de los
   caracteres especiales o combinaciones que se
   detallan a continuacion

*/
   

/* Limite admitido de la cadena de caracteres */

#define MAX_CARACTERES 4096


/* Limite admisible en el numero de tokens */

#define MAX_TOKENS 512


/* Tipo indentificador de token */

typedef int token_t;


/* Tokens que se reconocen */

#define TOK_NOMBRE   0  /* palabra separada por blancos o caracteres
			   especiales */
#define TOK_TUBO     1  /* | */
#define TOK_MAYOR    2  /* > */
#define TOK_MENOR    3  /* < */
#define TOK_APPEND   4  /* >> */
#define TOK_STDERR   5  /* 2> */


int trocea (char *s);
/* Descompone en tokens la cadena s
   Retorna:
     0 si no hay errores
     -1 en caso de error

   Errores:
     sobrepasa alguno de los limites admisibles */


int numero_de_tokens();
/* Retorna:
     el numero de tokens detectados en la ultima
     llamada a trocea
     0 si no se ha llamado todavia a trocea */


token_t tipo_token (int i);
/* Retorna:
     -1 si i>= numero_de_tokens() o i < 0
     el i-esimo token si i < numero_de_tokens() */


char * token_str (int i);
/* Retorna:
     NULL si i>= numero_de_tokens() o i < 0
     NULL si el_token(i) != NOMBRE 
     puntero al nombre asociado al token i-esimo */

