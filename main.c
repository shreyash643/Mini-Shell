#include"shell.h"



int main()
{

  system("clear");


  char prompt[100]="minishell$";

  char inputstr[100];
  signal(SIGINT,  signal_handler);
    signal(SIGTSTP, signal_handler);
     signal(SIGCHLD, signal_handler);

  scan_input(prompt,inputstr);

}