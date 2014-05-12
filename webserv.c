/* webserv.c - a minimal web server (version 0.2)
 *      usage: webserv portnumber
 *      features: supports the GET command only
 *                runs in the current directory
 *                forks a new child to handle each request
 *                has MAJOR security holes, for demo purposes only
 *                has many other weaknesses, but is a good start
 *      build: make webserv
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>
#include	<stdlib.h> // exit()
#include	<unistd.h> // execl()

#include	"socklib.c"

/* ------------------------------------------------------ *
   read_til_crnl(FILE *)
   skip over all request info until a CRNL is seen
   ------------------------------------------------------ */

void read_til_crnl(FILE *fp)
{
	char	buf[BUFSIZ];
	while( fgets(buf,BUFSIZ,fp) != NULL && strcmp(buf,"\r\n") != 0 )
		;
}

/*
###############################################################################
###############################################################################
###############################################################################
*/

/* ------------------------------------------------------ *
   the reply header thing: all functions need one
   if content_type is NULL then don't send content type
   ------------------------------------------------------ */

void header( FILE *fp, char *content_type )
{
	fprintf(fp, "HTTP/1.0 200 OK\r\n");
	if ( content_type )
		fprintf(fp, "Content-type: %s\r\n", content_type );
}

/* ------------------------------------------------------ *
   simple functions first:
        cannot_do(fd)       unimplemented HTTP command
    and do_404(item,fd)     no such object
   ------------------------------------------------------ */

void cannot_do(int fd)
{
	FILE	*fp = fdopen(fd,"w");

	fprintf(fp, "HTTP/1.0 501 Not Implemented\r\n");
	fprintf(fp, "Content-type: text/plain\r\n");
	fprintf(fp, "\r\n");

	fprintf(fp, "That command is not yet implemented\r\n");
	fclose(fp);
}

void do_404(char *item, int fd)
{
	FILE	*fp = fdopen(fd,"w");

	fprintf(fp, "HTTP/1.0 404 Not Found\r\n");
	fprintf(fp, "Content-type: text/plain\r\n");
	fprintf(fp, "\r\n");

	fprintf(fp, "The item you requested: %s\r\nis not found\r\n", item);
	fclose(fp);
}

/* ------------------------------------------------------ *
   the directory listing section
   isadir() uses stat, not_exist() uses stat
   do_ls runs ls. It should not
   ------------------------------------------------------ */

int isadir(char *f)
{
	struct stat info;
	return ( stat(f, &info) != -1 && S_ISDIR(info.st_mode) );
}

int not_exist(char *f)
{
	struct stat info;
	return( stat(f,&info) == -1 );
}

void do_ls(char *dir, int fd)
{
	FILE	*fp ;

	fp = fdopen(fd,"w");
	header(fp, "text / plain");
	fprintf(fp,"\r\n");
	fflush(fp);

	dup2(fd,1);
	dup2(fd,2);
	close(fd);
	execlp("ls","ls","-l",dir,NULL);
	perror(dir);
	exit(1);
}

/* ------------------------------------------------------ *
   the cgi stuff.  function to check extension and
   one to run the program.
   ------------------------------------------------------ */

char * file_type(char *f)
// returns 'extension' of file //
{
	char	*cp;
	if ( (cp = strrchr(f, '.' )) != NULL )
		return cp+1;
	return "";
}

int ends_in_cgi(char *f)
{
	return ( strcmp( file_type(f), "cgi" ) == 0 );
}

void do_exec( char *prog, int fd )
{
	FILE	*fp ;

	fp = fdopen(fd,"w"); // open socket for writing

	header(fp, NULL);
	fflush(fp);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
	//printf(prog);
	//"QUERY_STRING=%s"
	execl(prog,prog,NULL);
	perror(prog);
}

void poster( char *arg, int fd )
{
	
}
/* ------------------------------------------------------ *
   do_cat(filename,fd)
   sends back contents after a header
   ------------------------------------------------------ */

void do_cat(char *f, int fd)
{
	char	*extension = file_type(f);
	char	*content = "text/plain";
	FILE	*fpsock, *fpfile;
	int	c;

	if ( strcmp(extension,"html") == 0 )
		content = "text/html";
	else if ( strcmp(extension, "gif") == 0 )
		content = "image/gif";
	else if ( strcmp(extension, "jpg") == 0 )
		content = "image/jpeg";
	else if ( strcmp(extension, "jpeg") == 0 )
		content = "image/jpeg";

	fpsock = fdopen(fd, "w");
	fpfile = fopen( f , "r");
	if ( fpsock != NULL && fpfile != NULL )
	{
		header( fpsock, content );
		fprintf(fpsock, "\r\n");
		while( (c = getc(fpfile) ) != EOF )
			putc(c, fpsock);
		fclose(fpfile);
		fclose(fpsock);
	}
	exit(0);
}

/* ------------------------------------------------------ *
   process_rq( char *rq, int fd )
   do what the request asks for and write reply to fd 
   handles request in a new process
   rq is HTTP command:  GET /foo/bar.html HTTP/1.0
   ------------------------------------------------------ */

int process_rq( char *rq, int fd )
{
	printf("process the request!");
	char	cmd[BUFSIZ], arg[BUFSIZ];

	// create a new process and return if not the child
	if ( fork() != 0 )
		return 1;

	strcpy(arg, "./");		// precede args with ./
	if ( sscanf(rq, "%s%s", cmd, arg+2) != 2 )
		return 1;

	//char test[100] = "http://eve.kean.edu/bin/save.cgi?todo=go+shopping+at+PathMart&action=save.cgi";
	char *t, temp[100]="", *queryString;
	char resultString[100]="";
	int indexLength=0, index=0, tempIndex=0;
	//find the "?" and the lenght of the string with "?"-- query string
	queryString=strstr(arg, "?");
	queryString++;//advance to next character to exclude '?'
	printf("queryString: %s\n", queryString);

	if(strcmp(cmd,"POST")== 1)
	{
		printf("arg: %s\n", arg);
		poster( arg, fd ); // if post
	}
	else if((strcmp(cmd,"GET") != 0))
	{
		printf("!get");
		cannot_do(fd); // if not get / post
	}
	else if ( not_exist( arg ) )
	{
		printf("not_exist");
		do_404(arg, fd ); // error 404
	}
	else if ( isadir( arg ) )
	{
		printf("isaDir");
		do_ls( arg, fd ); // if folder print ls output
	}
	else if ( ends_in_cgi( arg ) )
	{
		printf("CGI!");
		do_exec( arg, fd ); // if cgi exec it and send output
	}
	else
	{
		printf("uh?");
		do_cat( arg, fd ); // if html like just print internals
	}
	return 0;
}

int main(int ac, char *av[])
{
	printf("we started.\n");
	int 	sock, fd;
	FILE	*fpin;
	char	request[BUFSIZ];

	if ( ac == 1 ){
		fprintf(stderr,"usage: webserv <port number>\n");
		exit(1);
	}else{
		printf("running\n");
	}
	
	sock = make_server_socket( atoi(av[1]) );
	
	if ( sock == -1 )
	{
		printf("can't open socket.");
		exit(2);
	}

	// main loop here

	while(1){
		printf("at top of while in main\n");
		// take a call and buffer it
		fd = accept( sock, NULL, NULL );
		fpin = fdopen(fd, "r" );

		// read request //
		fgets(request,BUFSIZ,fpin);
		printf("got a call: request = %s", request);
		read_til_crnl(fpin);

		// do what client asks
		process_rq(request, fd);

		fclose(fpin);
	}
}