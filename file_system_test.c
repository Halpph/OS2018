#include <stdio.h>
#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
	
	DiskDriver disk_driver;
  const char* filename = "./test.txt";

	DiskDriver_init (&disk_driver, filename, 512);//inizializzo il disk driver con 5 blocchi
	printf("Disk Driver initialized\n");

	SimpleFS sfs;
	
	DirectoryHandle* dir_handle= SimpleFS_init(&sfs, &disk_driver);
	printf("------------SimpleFS init---------\n");
	
	DirectoryHandle* root_dir_handle = dir_handle;// inizialmente root e current combaciano
	
	
	SimpleFS_format(&sfs);
	printf("------------SimpleFS formatted--------------\n\n");

	FileHandle* file_handle;
	char testo[512];
	
	int risposta;
	char nomefile[128];
	int ret;
	char** file_in_directory = malloc(sizeof(char*)*128);
	int i;
	for (i=0; i<128;i++){
		file_in_directory[i]=malloc(sizeof(char)*128);
	}
	
	
	do{
		printf("Selezionare l'operazione da fare: \n");
		printf(" 1) Crea File \n 2) Scrivi su file \n 3) Leggi file  \n 4) Cancella file \n 5) Crea directory\n 6) Stampa contenuto directory \n 7) Cambia directory\n---------Per uscire inserisci 0--------\n");
		scanf("%d",&risposta);
		
		switch(risposta){
		
		case 1://Creazione del file
			printf("-----------Inserire nome del file-----------\n");
			scanf("%s",nomefile);
			
			file_handle = SimpleFS_createFile(dir_handle, nomefile);
			if (file_handle == NULL){
				printf("File non creato\n");
				break;
			}
			printf("File creato\n");
			
			free(file_handle);
					
		break;
		
		case 2:// Scrittura su file
			printf("-----------Inserire nome del file su cui scrivere-----------\n");
			scanf("%s",nomefile);
			file_handle = SimpleFS_openFile(dir_handle, nomefile);
			
			if (file_handle == NULL){
				printf("Impossibile aprire file");
				break;
			}
			
			printf("---------Inserire testo da scrivere nel file----------\n");
			testo[512];
			scanf("%s",testo);
			
			ret = SimpleFS_write(file_handle, testo, strlen(testo));
			if (ret == -1){
				printf("Errore: impossibile scrivere sul file\n");
			}
			printf("Scrittura su file eseguita!!!!!\n");
			
			free(file_handle);
		break;
		
		case 3:// Lettura del file
			printf("---------Inserire nome del file da leggere----------\n");
			scanf("%s",nomefile);
			file_handle = SimpleFS_openFile(dir_handle, nomefile);
			
			if (file_handle == NULL){
				printf("Impossibile aprire file");
				break;
			}
			testo[512];
			ret = SimpleFS_read(file_handle, testo, file_handle->fcb->fcb.written_bytes);
			
			if (ret == -1){
				printf("Errore: impossibile leggere il file\n");
			}
			printf("Contenuto del file: \n'%s' \n", testo);
			
			free(file_handle);
			
		break;
		
		case 4:// rimozione file
		printf("-----------Inserire nome del file da eliminare-----------\n");
		scanf("%s",nomefile);
		
		SimpleFS_remove(dir_handle, nomefile);
		printf("-----------File eliminato-----------\n");
		
		break;
	
		case 5: // creazione directory
		printf("-----------Inserire nome directory-----------\n");		
		scanf("%s",nomefile);
		SimpleFS_mkDir(dir_handle,nomefile + ".dir");
		printf("-----------Directory creata-----------\n");
		
		break;
		
		
		case 6:// Stampa contenuto directory
		
		SimpleFS_readDir(file_in_directory,dir_handle);
		for(i=0; i<dir_handle->dcb->num_entries; i++){
			printf("- %s\n", file_in_directory[i]);
		}
			
		break;
		
		case 7://Cambio directory
		
		
		break;
		
		
		
		
		
		}
		
		
			
	}
	while(risposta!=0);
	 
	
}
