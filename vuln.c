#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHUNKS 10
void *chunks[MAX_CHUNKS];

void menu() {
    puts("\n1. Alloc");
    puts("2. Edit");
    puts("3. Free");
    puts("4. Exit");
    printf("> ");
}

int get_int() {
    int x;
    scanf("%d", &x);
    return x;
}

int main() {
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);

    while (1) {
        menu();
        int choice = get_int();

        if (choice == 1) {
            printf("Index: ");
            int idx = get_int();
            if (idx < 0 || idx >= MAX_CHUNKS) continue;
            printf("Size: ");
            size_t size = (size_t)get_int();
            chunks[idx] = malloc(size);
            if (!chunks[idx]) puts("Alloc failed!");

        } else if (choice == 2) {  // edit vuln: off-by-one overflow
            printf("Index: ");
            int idx = get_int();
            if (idx < 0 || idx >= MAX_CHUNKS || !chunks[idx]) continue;
            printf("Data: ");
           
            fgets(chunks[idx], ((size_t*)chunks[idx])[-1] + 1, stdin);

        } else if (choice == 3) {
            printf("Index: ");
            int idx = get_int();
            if (idx < 0 || idx >= MAX_CHUNKS || !chunks[idx]) continue;
            free(chunks[idx]);
            chunks[idx] = NULL;

        } else if (choice == 4) {
            break;
        }
    }
    return 0;
}
