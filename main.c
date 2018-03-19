#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

void listdir(const char *name, int *fd)
{
    DIR *dir;
    FILE *fp;
    struct dirent *entry;
    char path[1024];

    if (!(dir = opendir(name))){
    	printf("Cannot open directory\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

    	sprintf(path, "%s/%s", name, entry->d_name);

        fp = fopen(path, "r");
        fseek(fp, 0L, SEEK_END);
        int sz = ftell(fp);

        struct Entry* e;
        e = makeEntry(sz, path);

        close(fd[0]);
        write(fd[1], e, sizeof(struct Entry*));

        if (entry->d_type == DT_DIR) {
        	listdir(path, fd);
        }
    }
    closedir(dir);
}

struct Entry {
	int filesize;
	char *pathname;
	struct Entry *next;
};

struct LList {
	struct Entry *head;
	struct Entry *tail;
};

void sortedInsert(struct LList* list, struct Entry *add) {
	struct Entry* current;
	struct Entry* head;
	struct Entry* tail;

	head = list->head;
	tail = list->tail;

	if (head == NULL || head->filesize >= add->filesize) {
		add->next = head;
		head = add;
	} else {
		current = head;
		while (current->next != NULL && current->next->filesize < add->filesize) {
			current = current->next;
		}
		add->next = current->next;
		current->next = add;
	}
}

struct Entry* makeEntry(int s, char* n) {
	struct Entry* e;
	e = (struct Entry*) malloc(sizeof(struct Entry));
	e->filesize = s;
	e->pathname = n;
	e->next = NULL;
	return e;
}

struct LList* makeList() {
	struct LList* l;
	l = (struct LList*) malloc(sizeof(struct LList));
	l->head = NULL;
	l->tail = NULL;
	l->head->next = l->tail;
	return l;
}

void printList(struct LList* list){
	struct Entry* node;
	node = list->head;
    while(node != NULL){
        printf("%i\t%s\n", node->filesize, node->pathname);
        node = node->next;
    }
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Invalid Input");
	}

	char* initPath = argv[1];

	struct LList* list;
	list = makeList();

	pid_t pid;
	int fd[2];
	pipe(fd);

	pid = fork();

	switch (pid) {

	case -1:
		perror("Unable to Fork");
		exit(1);
		break;
	case 0: // child
		listdir(initPath, fd);
		exit(0);
		break;
	default: // parent
		int status=0;
		while(waitpid(pid, &status, WNOHANG) == 0){ //until the child process is done
			 close(fd[1]);

			 struct Entry* e;
			 read(fd[0], e, sizeof(struct Entry*));

			 sortedInsert(list, e);
		}
		printList(list);

		break;
	}
}










