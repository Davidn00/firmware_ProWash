#include <stdio.h>
#include "string.h"
#include <stdlib.h>
#include <stdbool.h>
#include <ncurses.h>

typedef struct TMenuItem {
  char label[100];
  struct TMenuItem *children;
  struct TMenuItem *parent;
  int childrenCount;
  void (*action) ();
  int currentMenu;
} TMenuItem;

TMenuItem newMenuItem() {
  TMenuItem newItem;
  newItem.childrenCount = 0;
  newItem.children = NULL;
  newItem.parent = NULL;
  newItem.currentMenu = 0;
  newItem.action = NULL;
  return newItem;
}

TMenuItem* addMenuItem(TMenuItem* parent, TMenuItem child) {
  child.parent = parent;
  parent->childrenCount++;
  parent->children = realloc(parent->children, (sizeof(TMenuItem)*parent->childrenCount));
  parent->children[parent->childrenCount-1] = child;

  return &parent->children[parent->childrenCount-1];
}

void labelMenu(TMenuItem *item, const char* label) {
  strcpy(item->label, label);
}

//methods to exec
void testMenu() {
  printf("menu activated\n");
}




void readArrowKeys(TMenuItem *item);
void displayMenu(TMenuItem *item);

void displayMenu(TMenuItem *item) {
  for (int i = 0; i < item->childrenCount; i++) {
    printf("%c %d - %s\n", i == item->currentMenu ? '*' : ' ' , i+1,  item->children[i].label);
  }
  
  readArrowKeys(item);
}

void readArrowKeys(TMenuItem *item) {
  while (true) {
    
    int readChar = getch();

    if (readChar == 27) {
      if (item->parent != NULL) {
        displayMenu(item->parent);
      }
    }

    if (readChar == 0x0A) {
      TMenuItem *pointedChild = &item->children[item->currentMenu];
      if (pointedChild->action != NULL) {
        pointedChild->action();
      } else if(pointedChild->childrenCount > 0) {
        displayMenu(pointedChild);
      } else {
        displayMenu(item);
      }
    }
    if (readChar == '\033') {
      printf("%c", getch());
      switch (getch()) {
      case 'A':
        if (item->currentMenu > 0) {
          item->currentMenu--;
          displayMenu(item);
        }
      break;
      case 'B':
        if (item->currentMenu < item->childrenCount - 1) {
          item->currentMenu++;
          displayMenu(item);
        }
      break;
      default:
      break;
      }
    }
  }
}

int main() {
  initscr();
  cbreak();
  TMenuItem main = newMenuItem();

  TMenuItem wash = newMenuItem();
  labelMenu(&wash, "LAVAR");
  TMenuItem prime = newMenuItem();
  labelMenu(&prime, "CEBAR");
  TMenuItem shake = newMenuItem();
  labelMenu(&shake, "AGITAR");
  TMenuItem program = newMenuItem();
  labelMenu(&program, "PROGRAMAR");

  TMenuItem programCreate = newMenuItem();
  labelMenu(&programCreate, "CREAR");
  TMenuItem programEdit = newMenuItem();
  labelMenu(&programEdit, "EDITAR");
  TMenuItem programDelete = newMenuItem();
  labelMenu(&programDelete, "BORRAR");

  TMenuItem washParams = newMenuItem();
  labelMenu(&washParams, "Seleccionar Parametros");



  programCreate.action = testMenu;

  //add to program
  addMenuItem(&program, programCreate);
  addMenuItem(&program, programEdit);
  addMenuItem(&program, programDelete);

  //add to main
  addMenuItem(&main, prime);
  addMenuItem(&main, shake);
  addMenuItem(&main, wash);
  addMenuItem(&main, program);

  washParams = *addMenuItem(&wash, washParams);
 
  displayMenu(&main);
  return 0;
}

