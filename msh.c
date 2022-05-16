/*-
 * msh.c
 *
 * Minishell C source
 * Show how to use "obtain_order" input interface function
 *
 * THIS FILE IS TO BE MODIFIED
 */
 /*Manu este esto es lo que he copiado de mi hermano de la practica 2 le faltan cosas (algunos mandatos internos) pero solo falta eso
 Tambien me preocupa que el codigo no este lo suficientemente ambiado como para que no detecten la copia asique de moemnto lo que hare sera ponerme con la memoria*/

#include <stddef.h>			/* NULL */
#include <stdio.h>			/* setbuf, printf */
#include <stdlib.h>			
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define LONG_CADENA 128

extern int obtain_order();		/* See parser.y for description */

struct command{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands 
  char ***argvv;
  // Store the I/O redirection
  char *filev[3];
  // Store if the command is executed in background or foreground
  int bg;
};

void free_command(struct command *cmd)
{
   if((*cmd).argvv != NULL){
     char **argv;
     for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
     {
      for (argv = *(*cmd).argvv; argv && *argv; argv++)
      {
        if(*argv){
           free(*argv);
           *argv = NULL;
        }
      }
     }
   }
   free((*cmd).args);
   int f;
   for(f=0;f < 3; f++)
   {
     free((*cmd).filev[f]);
    (*cmd).filev[f] = NULL;
   }
}

void store_command(char ***argvv, char *filev[3], int bg, struct command* cmd)
{
  int num_commands = 0;
  while(argvv[num_commands] != NULL){
    num_commands++;
  }

  int f;
  for(f=0;f < 3; f++)
  {
    if(filev[f] != NULL){
      (*cmd).filev[f] = (char *) calloc(strlen(filev[f]), sizeof(char));
      strcpy((*cmd).filev[f], filev[f]);
    }
  }

  (*cmd).bg = bg;
  (*cmd).num_commands = num_commands;
  (*cmd).argvv = (char ***) calloc((num_commands+1), sizeof(char **));
  (*cmd).args = (int*) calloc(num_commands, sizeof(int));
  int i;
  for( i = 0; i < num_commands; i++){
    int args= 0;
    while( argvv[i][args] != NULL ){
      args++;
    }
    (*cmd).args[i] = args;
    (*cmd).argvv[i] = (char **) calloc((args+1), sizeof(char *));
    int j;
    for (j=0; j<args; j++) {
       (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]), sizeof(char));
       strcpy((*cmd).argvv[i][j], argvv[i][j] );
    }
  }
}
int main(void)
{
	char ***argvv;	
	int command_counter;
	int num_commands;
	int args_counter;
	char *filev[3];
	int bg;
	int ret;
	
	time_t ini_t; /*Para el comando interno mytime*/
	time(&ini_t);
	int pid;
	/*Para los pipes(mas de un mandato)se inicializan dos arrays*/
	int tuberia1[2];
	int tuberia2[2];
	char t[256];/*Se utiliza para guardar la ruta de un directorio*/
	int stat; /*Para guardar el estatus de un proceso(si acaba o no)*/
	char *puntero; /*Para apuntar a un directorio*/
	DIR* direccion; /*Para guardar la ruta de un directorio*/
	struct dirent *gdirec;/*Estructura que sirve para guardar el directorio de una direccion */

	setbuf (stdout, NULL);			/* Unbuffered */
	setbuf (stdin, NULL);
	/*Se crea un bucle while(nos lo dan)para que el programa siga ejecutandose hasta que el usuario pulse Ctrl+D*/
	while (1) 
	{
		/*Si ret == 0 entonces el usuario a pulsado Ctrl+D y por lo tanto se termina el bucle*/
		fprintf(stderr, "%s", "msh> ");	/* Prompt */
		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0) break;		/* EOF */
		if (ret == -1) continue;	/* Syntax error */
		num_commands = ret - 1;		/* Line */
		if (num_commands == 0) continue;	/* Empty line */
		/*Al tener el numero de comandos(num_commands) usaremos un switch para los casos en los que hay uno o dos o mas mandatos e implementamos en cada caso el codigo necesario para emular los comandos la consola de Linux*/
		switch(num_commands){
			case 1:/*un solo mandato*/
				/*Comando interno exit*/
				if(strcmp("exit",argvv[0][0])==0){
					free(argvv);
					printf("Goodbye!\n");
					return 0;
				}
				/*comando interno mytime*/
				if(strcmp("mytime",argvv[0][0])==0){
					time_t fin_t;
					time(&fin_t);
					double dif;
					dif=difftime(fin_t, ini_t);
					int hora= 00;
					int minuto= 00;
					int sec;
					if(dif>=60){					
						while(dif>=60){
						dif= dif-60;
						minuto++;				
						}
					}
					if(minuto>=60){
						while(minuto>=60){
							minuto= minuto-60;
							hora++;
						}
					}
					sec=dif;
    					printf("Uptime: %i horas %i minutos %i segundos \n", hora, minuto, sec);
				}
				/*Comando interno mycd*/
				if(strcmp("mycd", argvv[0][0])==0){/*bien introducido*/
					if(argvv[0][1]!=NULL){/*Carpeta*/
						if((chdir(argvv[0][1]))==-1) perror("Error en la redireccion");/*Redireccionamos a la carpeta indicada si no hay errores*/
					}else{
						chdir(getenv("HOME"));/*Si no se introduce nada se va a HOME*/
					}
				getcwd(t,256);/*Guardamos la ruta del archivo*/
				printf("%s\n", t);
				}
				if((pid=fork())==0){
				/*En este condicional nos encargaremos de hacer los mandatos con redireccion usando pipes. De la redireccion se encarga el hijo porque si lo hiciera el padre saldria error(nos salio error)*/
					if(filev[0]!=NULL){
		                                int filedesc=open(filev[0],O_RDONLY, 0644);
		                                close(0); /*Cerramos entrada estandar*/
		                                dup(filedesc); /*Se duplica el descriptor del fichero y reemplaza a la entrada*/
		                                close(filedesc);
		                        }
					if(filev[1]!=NULL){
		                                int filedesc=open(filev[1],O_CREAT|O_WRONLY,0666);
				     		close(1); /*Cerramos salida estandar*/
		                                dup(filedesc); /*Se duplica el descriptor del fichero y reemplaza a la salida*/
		                                close(filedesc);
		                        }
					if (filev [2]!=NULL){
		                                int filedesc=open(filev[2],O_CREAT|O_WRONLY,0666);
		                                close(2); /*Cerrar la salida de errores*/
		                                dup(filedesc); /*Se duplica el descriptor del fichero y reemplaza a la salida de errores*/
		                                close(filedesc);
		                        }
					/*El array filev tiene 3 dimensiones:
					filev[0] es el nombre del fichero de una redireccion de entrada
					filev[1] es el nombre del fichero de una redireccion de salida
					filev[2] en el caso de la salida de errores
					Si tanto filev[0] como filev[1] valen null no hay redireccion tanto de entrada como de salida y tambien en filev[2] restringimos la salida de errores*/
					execvp(argvv[0][0], argvv[0]);/*Ejecutamos el mandato que nos piden*/
					exit(-1);
				}else if(pid==-1) {
					perror("Error en el fork");	
				}
			break;
			/*Fin del caso 1(un solo mandato y algunos mandatos internos)*/
			case 2:/*Empezamos a utilizar tuberias para emular mas de un mandato*/
				pipe(tuberia1);/*Creamos el pipe que tiene una entrada y una salida por que es un array de 2*/
				if((pid=fork())==0){
					/*Proceso de conexion de tuberia*/
					close(1);	/*Cerramos la salida estandar del proceso*/
					dup(tuberia1[1]);	/*Duplicamos y conectamos la salida de la tuberia*/
					/*Cerramos la entrada y la salida de la tuberia*/
					close(tuberia1[0]);	
					close(tuberia1[1]);
					/* Redireccionamiento de entrada */
					if(filev[0]!=NULL){
						int filedesc=open(filev[0], 0766);
						close(0);/*Cerramos las entradas*/
						dup(filedesc);/*Reemplazamos las entradas por el descriptor del fichero*/
						close(filedesc);/*cerramos el descriptor del fichero*/
					}
					/*Redireccionamiento de salida de errores*/
					if(filev[2]!=NULL){
						int filedesc=open(filev[2], O_CREAT | O_WRONLY, 0666);
						close(2);/*Errores*/ 				
						dup(filedesc);/*Reemplazamos la salida de errores con el fichero*/
						close(filedesc);/*Cerramos el descriptor del fichero*/
					}
					execvp(argvv[0][0],argvv[0]);
					exit(-1);
				}
				/*Se ejecuta el primer mandato ahora pasamos al siguiente*/
				if((pid=fork())==0){
					close(0);/*Cerramos la entrada estandar del proceso*/
					dup(tuberia1[0]);	/*Duplicamos y conectamos la entrada de la tuberia*/
					close(tuberia1[0]);	
					close(tuberia1[1]);
						if(filev[1]!=NULL){
							int filedesc=open(filev[1],O_CREAT|O_WRONLY,0666);
							close(1);
							dup(filedesc);
							close(filedesc);
						}
						if(filev[2]!=NULL){
							int filedesc=open(filev[2],O_CREAT|O_WRONLY,0666);
							close(2);
							dup(filedesc);
							close(filedesc);
						}
						execvp (argvv[1][0], argvv[1]);
						exit(-1);
				}
				close (tuberia1 [1]);
				close (tuberia1 [0]);
				break;
			/*Fin del caso 2, comienza el caso 3 con tres mandatos*/
			case 3:
				pipe (tuberia1); /*Creamos las tuberias*/
				pipe (tuberia2);
               		 	/*Creamos los hijos*/
                       		if((pid=fork())==0){
					close(1);
					dup(tuberia1[1]);	
		               		close(tuberia1[1]);
		               		close(tuberia1[0]);
					if(filev[0]!=NULL){
		                               int filedesc=open(filev[0],O_RDONLY, 0644);
		                               close(0); /*Se cierra la entrada estandar*/
		                               dup(filedesc); /*Duplicamos el descriptor del fichero y lo cerramos*/
		                               close(filedesc);
		                        }
					if(filev[2]!=NULL){
		                               int filedesc=open(filev[2],O_CREAT|O_WRONLY,0666);
		                               close (2); /*Se cierra la salida de errores*/
		                               dup (filedesc); /*Duplicar el descriptor del fichero y lo cerramos*/
		                               close (filedesc);
		                        }
		                        execvp(argvv[0][0],argvv[0]);/*Ejecutamos el mandato*/
		                        exit(-1);
                       		}
                       		if ((pid=fork())==0){
					close(0);/*Cerrar la entrada estandar del proceso*/
					dup(tuberia1[0]);
					close(tuberia1[1]);
		               		close(tuberia1[0]);
					close(1);/*Cerrar la salida destandar del proceso*/
					dup(tuberia2[1]);
					close(tuberia2[1]);
					close(tuberia2[0]);
					if(filev[2]!=NULL){
		                               int filedesc=open(filev[2],O_CREAT|O_WRONLY,0666);
		                               close(2); /*Se cierra la salida de errores*/
		                               dup(filedesc); /*Se duplica el descriptor del fichero y se sutituye en la salida*/
		                               close(filedesc);
		                        }
		                        execvp(argvv[1][0], argvv[1]);
					exit(-1);
                       		}
				close(tuberia1[1]);
				close(tuberia1[0]);
                      		if ((pid=fork())==0){
					close(0);
					dup(tuberia2[0]);
					close(tuberia2[1]);
		               		close(tuberia2[0]);
					if (filev[1]!=NULL){
		                               int filedesc=open(filev[1],O_CREAT|O_WRONLY,0666);
		                               close(1); /*Se cierra salida estandar*/
		                               dup(filedesc); /*Reemplazamos la salida con el fichero*/
		                               close(filedesc);
		                        }
					if (filev[2]!=NULL){
		                               int filedesc=open(filev[2],O_CREAT|O_WRONLY,0666);
		                               close(2); /* Cerramos salida de errores*/
		                               dup(filedesc); // Reemplazamos la salida de errores con el fichero
		                               close(filedesc);
		                        }
		                       	execvp(argvv[2][0], argvv[2]);/*Se ejecuta el comando*/
		                       	exit(-1);
		                }
				close(tuberia2[1]);
				close(tuberia2[0]);
                break;
		}/*Fin del Switch*/
		if (!bg) //bg ==0
        while (pid != wait (&stat));/*Para procesos zombies*/
		
	} //end while 

	return 0;

} //end main

