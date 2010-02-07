#define MAX_LINE_LEN 4096
#define MAX_PIPES 50
#define MAX_ARGS 100

typedef struct {
   int time;
   char buffer[MAX_LINE_LEN];
   char *args[MAX_PIPES][MAX_ARGS];
   char *fsal;
   char *fent;
   char *fsalerr;
   int append;
} CMD_t;

int obtener_orden(char *s, CMD_t *cmd);
