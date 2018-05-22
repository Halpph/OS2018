#include <stdio.h>
#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){

	DiskDriver disk_driver;
    const char* filename = "./disk.txt";

	DiskDriver_init (&disk_driver, filename, 512);//inizializzo il disk driver con 5 blocchi
	printf("------Disk Driver initialized------\n");

	SimpleFS sfs;

    printf("------SimpleFS init---------\n");
	DirectoryHandle* dir_handle = SimpleFS_init(&sfs, &disk_driver);
    if(dir_handle == NULL){
        printf("------SimpleFS formatted---------\n\n");
        DiskDriver_init (&disk_driver, filename, 512);//inizializzo il disk driver con 5 blocchi
        SimpleFS_format(&sfs);
        dir_handle = SimpleFS_init(&sfs, &disk_driver);

    }
	DirectoryHandle* root_dir_handle = dir_handle;// inizialmente root e current combaciano

	FileHandle* file_handle;
	char testo[512];

	int risposta;
	char nomefile[128];
	int is_file[128];
	int ret;
	char** file_in_directory = malloc(sizeof(char*)*128);
	int i;
	for (i=0; i<128;i++){
		file_in_directory[i]=malloc(sizeof(char)*128);
	}


	do{
        printf("\n|@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|");
		printf("\n|--------- Selezionare l'operazione da fare: ---------|\n");
        printf("|               DIRECTORY CORRENTE: '%s'\n",dir_handle->dcb->fcb.name);
		printf("| 1) Crea File \n| 2) Scrivi su file \n| 3) Leggi file  \n| 4) Cancella file/directory \n| 5) Crea directory\n| 6) Stampa contenuto directory \n| 7) Cambia directory\n| 0) Esci\n|-----------------------------------------------------|\n|@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|\n");
		scanf("%d",&risposta);

		switch(risposta){

		case 1://Creazione del file
			printf("------Inserire nome del file------\n");
			scanf("%s",nomefile);
			file_handle = SimpleFS_createFile(dir_handle, nomefile);
			if (file_handle == NULL){
				printf("[!] File non creato\n");
				break;
			}
			printf("[OK] File creato\n");

			free(file_handle);

			break;

		case 2:// Scrittura su file
            printf("------Lista file disponibili------\n");
            SimpleFS_readDir(file_in_directory,is_file,dir_handle);
			for(i=0; i<dir_handle->dcb->num_entries; i++){
				if(is_file[i] == 0) // se è file
					printf("FILE - %s\n", file_in_directory[i]);
			}
			printf("------Inserire nome del file su cui scrivere------\n");
			scanf("%s",nomefile);
			file_handle = SimpleFS_openFile(dir_handle, nomefile);

			if (file_handle == NULL){
				printf("[!] Impossibile aprire file");
				break;
			}

			printf("------Inserire testo da scrivere nel file------\n");
			testo[512];
			scanf("%s",testo);

			ret = SimpleFS_write(file_handle, testo, strlen(testo));
			if (ret == -1){
				printf("[!] Impossibile scrivere sul file\n");
			}
			printf("[OK] Scrittura su file eseguita\n");

			free(file_handle);
			break;

		case 3:// Lettura del file
            printf("------Lista file disponibili------\n");
            SimpleFS_readDir(file_in_directory,is_file,dir_handle);
			for(i=0; i<dir_handle->dcb->num_entries; i++){
				if(is_file[i] == 0) // se è file
					printf("FILE - %s\n", file_in_directory[i]);
			}
			printf("------Inserire nome del file da leggere------\n");
			scanf("%s",nomefile);
			file_handle = SimpleFS_openFile(dir_handle, nomefile);

			if (file_handle == NULL){
				printf("[!] Impossibile aprire file");
				break;
			}
			testo[512];
			ret = SimpleFS_read(file_handle, testo, file_handle->fcb->fcb.written_bytes);

			if (ret == -1){
				printf("[!] impossibile leggere il file\n");
			}
			printf("[OK] Contenuto del file: \n'%s' \n", testo);

			free(file_handle);

			break;

		case 4:// rimozione file/directory
			printf("------Lista file/directory disponibili------\n");
			SimpleFS_readDir(file_in_directory,is_file,dir_handle);
			for(i=0; i<dir_handle->dcb->num_entries; i++){
				if(is_file[i] == 0) // se è file
					printf("FILE - %s\n", file_in_directory[i]);
				else
					printf("DIR  - %s\n", file_in_directory[i]);
			}

			printf("------Inserire nome del file da eliminare------\n");
            //printf("------ATTENZIONE: selezionando una directory, cancellerai tutti i file sottostanti!------\n");
			scanf("%s",nomefile);

			ret = SimpleFS_remove(dir_handle, nomefile);
			if (ret == 0)	printf("[OK] Blocco eliminato\n");
			else if(ret == -1) printf("[!] Errore eliminazione blocco\n");
			break;

		case 5: // creazione directory
            printf("------Lista file/directory disponibili------\n");
            SimpleFS_readDir(file_in_directory,is_file,dir_handle);
			for(i=0; i<dir_handle->dcb->num_entries; i++){
				if(is_file[i] == 0) // se è file
					printf("FILE - %s\n", file_in_directory[i]);
				else
					printf("DIR  - %s\n", file_in_directory[i]);
			}
			printf("------Inserire nome directory------\n");
			scanf("%s",nomefile);
			ret = SimpleFS_mkDir(dir_handle,nomefile);
			if(ret == 0) printf("[OK] Directory creata\n");
            else if(ret == -1) printf("[!] Impossibile creare directory\n");
			break;

		case 6:// Stampa contenuto directory
			printf("\nContenuto Directory:\n");
			SimpleFS_readDir(file_in_directory,is_file,dir_handle);
			for(i=0; i<dir_handle->dcb->num_entries; i++){
				if(is_file[i] == 0) // se è file
					printf("FILE - %s\n", file_in_directory[i]);
				else
					printf("DIR  - %s\n", file_in_directory[i]);
			}

			break;

		case 7://Cambio directory

      printf("------Lista file/directory disponibili------\n");
			SimpleFS_readDir(file_in_directory,is_file,dir_handle);
			for(i=0; i<dir_handle->dcb->num_entries; i++){
				if(is_file[i] == 0) // se è file
					printf("FILE - %s\n", file_in_directory[i]);
				else
					printf("DIR  - %s\n", file_in_directory[i]);
			}

			printf("------Inserire directory ('..' per tornare indietro)------\n");
			scanf("%s",nomefile);
			ret = SimpleFS_changeDir(dir_handle, nomefile);
			if(ret == 0) printf("[OK] Directory cambiata -> Directory corrente: %s\n", dir_handle->dcb->fcb.name);
            else if(ret == -1) printf("[!] Impossibile cambiare directory\n");


			break;

		}

	}
	while(risposta!=0);


}
