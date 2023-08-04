#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
//thread_arg struct, known as args is the linked list where arguments for the pthreads are stored
typedef struct thread_arg{
	char file1[255];
	char file2[255];
	struct thread_arg *next;
}args;
//struct for stroring linked list of threads (p_thread ids) known as list
typedef struct thread_list{
	pthread_t thread;
	struct thread_list *next;
}list;
//init initializes a linked list of pthreads where the pthread_ids are stored and then joined
list *init();
//init_a initalizes the args * linked list, which is the linked list for holding 
args *init_a();
//insert the file names into args, args is the pthread argument for copy file
void insert_a(args *,char *, char *);
//copy_file is the thread function and copies one file into another
void *copy_file(void *);
//insert into the pthread linked list *list
void insert(list *,pthread_t);
//get an element from args linked list
args *get_arg(args *a);
//join all the pthreads in the list
void join_list(list *a);
//free the list
void free_list(list *a);
//free the args
void free_args(args *a);
//file that iterates through the directory and handles threading
void file_system(char *,int);
//main just handles the command line arg and gets the current working directory, and does the file system function
int main(int argc, char *argv[]){
	//overwrite is the code for when you need an overwrite/ O is no overwrite the default, 1 is do overwrite
	int overwrite = 0;
	//handle the command line args
	if(argc == 2){
		if(strcmp(argv[1],"-r") == 0){
			overwrite = 1;
		}
	}
	//get current working directory and call file_system on current_dir and overwrite
	char current_dir[256];
	getcwd(current_dir,sizeof(current_dir));
	file_system(current_dir,overwrite);
}
//file_system goes into the current directory named by similarly named current_dir and backs-ups the files into a new directory .backup
//if overwrite is 1,instead restore the backups from .backup
void file_system(char *current_dir,int overwrite){
	//initialize linked list arg_array for storing arguments to threads
	args *arg_array = init_a();
	//initialize linked list threads for threads
	list *threads = init();
	//initialize thread_coun. It is a debugging tool artifact
	int thread_count = 0;
	//dir and back are used to read directies (dir (current working) and back (.backup))
	DIR *dir;
	DIR *back;
	struct dirent *dp,*dp1;
	dir = opendir(current_dir);
	struct stat new_dir, *a = &new_dir;
	//create pathname for the .backup folder and store it in duplicate
	char back_up[] = "/.backup";
	char duplicate[256] = "";
	strcpy(duplicate,current_dir);
	strcat(duplicate,back_up);
	//check if .backup folder exists, if not create the folder
	if(lstat(duplicate,a) == -1){
		int make_dir = mkdir(duplicate,0777);
		if(make_dir == -1){
			int errnum = errno;
			fprintf(stderr,"Directory Error: %s\n",strerror(errnum));
		}
	}
	//change permissions just in case
	chmod(duplicate,0777);
	struct stat old_dir,*b = &old_dir;
	//if .backup exists and overwrite is 1, restore the backup folders to the current dir
	if((lstat(duplicate,a) != -1) && overwrite == 1){
		//open the backup directory and read it in a while loop
		back = opendir(duplicate);
		while((dp1 = readdir(back)) != NULL){
			//create pathname for file to restore and save the regular file you read (the backup file) to buffer
			char buffer[256];
			strcpy(buffer,dp1->d_name);
			char pathname[256];
			strcpy(pathname,duplicate);
			strcat(pathname,"/");
			strcat(pathname,buffer);
			//check if pathname is regular file and exists, then create the pathname for the restore file
			if(stat(pathname,a) == 0){
				if(S_ISREG (a->st_mode) == 1){
					int size = strlen(buffer);
					int first = 0;
					for(int l = 0;l < size;l++){
						if(buffer[l] == '.'){
							first = l;
						}
					}
					buffer[first] = '\0';
					char sample[256];
					strcpy(sample,buffer);
					char path2[255];
					strcpy(path2,current_dir);
					strcat(path2,"/");
					strcat(path2,sample);
					struct stat c;
					int success = lstat(path2,&c);
					//if backup file is more current than the original file, backup the original file
					if(a->st_mtime > c.st_mtime){
						//insert pathname and arg2 to arg_array
                                        	insert_a(arg_array,pathname,path2);
                                        	pthread_t id;
						args *e = get_arg(arg_array);
						printf("Restoring %s\n",sample);
						//create pthread
						int pthread_error = pthread_create(&id,NULL,copy_file,(void *)e);
						insert(threads,id);
						thread_count++;
					}
					else{
						printf("File %s more current than the backup file\n",sample);
					}
				}
			}
		}
		//close the directory for the backup folder
		closedir(back);

	}
	else{
		//read the directory and save the file name to the buffer. If buffer is equal to filenameback up the file
		//if buffer is directory recurse
		while((dp = readdir(dir)) != NULL){
			char buffer[255];
			strcpy(buffer,dp->d_name);
			int check;
			if((check = stat(buffer,b)) == 0){
				//if regular file create pathnames
				if((S_ISREG (b->st_mode) == 1) && (strcmp(buffer,".") != 0) && strcmp(buffer,"..") != 0){
					char end[] = ".backup";
					char dir_path[255];
					strcpy(dir_path,current_dir);
					strcat(dir_path,"/");
					strcat(dir_path,buffer);
					char temp[256] = "";
					strcpy(temp,buffer);
					strcat(temp,end);
					char directory[255];
			  	 	strcpy(directory,duplicate);
					strcat(directory, "/");
					strcat(directory,temp);
					char old[256];
					strcpy(old,buffer);
					strcpy(buffer,dir_path);
					//check if you can access directory, 									
					if(access(directory,R_OK) == 0){
						struct stat c;
						int success = lstat(directory,&c);
						//if file is not more current than its backup, back it up
						if(b->st_mtime > c.st_mtime){
                                        		insert_a(arg_array,buffer,directory);
                                        		pthread_t id;
							args *e = get_arg(arg_array);
							printf("backing up file %s\n",old);
							printf("Warning overwriting the backup file for %s\n",old);
							int pthread_error = pthread_create(&id,NULL,copy_file,(void *)e);
							insert(threads,id);
							thread_count++;
                               
						}
						else{
							printf("File %s is at its most current version\n",old);
						}
					}
					//if you cant access the back up files create back-up files for every file
					else{
						insert_a(arg_array,buffer,directory);
						pthread_t id;
						printf("backing up file %s\n",old);
						args *e = get_arg(arg_array);
						int pthread_error = pthread_create(&id,NULL,copy_file,(void *)e);
						insert(threads,id);
						
						thread_count++;

				}
				//clean the strings				
				strcpy(buffer,"");
				strcpy(directory,"");
				
			}
			//handles directories here
			if((strcmp(".",buffer) != 0) && (strcmp("..",buffer) != 0) && strcmp(".backup",buffer) != 0){
				if(S_ISDIR(b->st_mode) == 1){
					char path[256];
					strcpy(path,current_dir);
					strcat(path, "/");
					strcat(path,buffer);
					//chmod(path,7777);
					file_system(path,overwrite);
				}
			}
		}
		//error check
		if(check == -1){
			int errnum = errno;
			fprintf(stderr,"Error: %s\n",strerror(errnum));
		}
	 }

	}
	//join the pthreads
	join_list(threads);
	//free everything
	free_list(threads);
	free_args(arg_array);
	//close the dir
	int finish = closedir(dir);
}
//coppy_file takes argument of void *arg and converts it to an args *. It takes the fields from the arg pointer and 
//reads the lines of one file into a buffer. The buffer is then written to the other file. This is the thread function
void *copy_file(void *arg){
	//convert arg to args *answer
	args *answer = (args*)arg;
	//create strings old and new, they will store the file names
	char const *old,*new;
	//make sure answer is not null
	if(answer != NULL){
		//set answer->file1 to old, and andwer->file2 to new
		old = answer->file1;
		new = answer->file2;
		//open the files old, new as original and copy respectively
        	FILE *original = fopen(old,"r");
        	FILE *copy = fopen(new, "w");
        	//error catching here
        	if(copy == NULL){
        		int errnum = errno;
			fprintf(stderr,"Error: %s\n",strerror(errnum));
        	}
        	else if(original == NULL){
        		int errnum = errno;
			fprintf(stderr,"Error: %s\n",strerror(errnum));
        	}
        	else{
        		char *buffer = NULL;
        		size_t size;
        		int characters;
        		//while you can read original, write the buffer to copy line by line
        		while((characters = getline(&buffer,&size,original)) != -1){
                		if(buffer != NULL){
                			fwrite(buffer,1,characters,copy);
                		}
        		}
        		//free everything
        		free(buffer);
        		fclose(original);
        		fclose(copy);
        	}
        	
        }
}
//init linked list for linked list of pthreads
list *init(){
	list *new = malloc(sizeof(list));
	new->next = NULL;
	return new;
}
void insert(list *a,pthread_t thread){
	list *new = malloc(sizeof(list));
	while(a->next != NULL){
		a = a->next;
	}
	a->next = new;
	new->next = NULL;
	new->thread = thread;
}
//initalize the args linked list
args *init_a(){
	args *new = malloc(sizeof(args));
	new->next = NULL;
	return new;
}
//insert some strings into the linked list of args a. This is the argument linked list for the pthreads
void insert_a(args *a,char *old,char *new){
	args *new_a = malloc(sizeof(args));
	while(a->next != NULL){
		a = a->next;
	}
	a->next = new_a;
	new_a->next = NULL;
	strcpy(new_a->file1,old);
	strcpy(new_a->file2,new);
}
//return the last element from the linked list
args *get_arg(args *a){
	while(a->next != NULL){
		a = a->next;
	}
	return a;
}
//join all the pthreads in the linked list a
void join_list(list *a){
	a = a->next;
	while(a != NULL){
		pthread_join(a->thread,NULL);
		a = a->next;
	}
}
//free list frees the pthread list
void free_list(list *a){
	while(a != NULL){
		list *b = a;
		a = a->next;
		free(b);
		b = NULL;
	}
}
//free args frees the args array
void free_args(args *a){
	while(a != NULL){
		args *b = a;
		a = a->next;
		free(b);
		b = NULL;
	}
}
