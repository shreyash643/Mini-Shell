#include "shell.h"

char *cmmd = NULL;
char *ext_cmd[155];
char *builtins[] = {"exit", "cd", "pwd", "echo", NULL}; 
char prompt[100] = "minishell$:";  
int pid = 0, status = 0;
Slist *head = NULL;
char input_string[200];            
char *arr[155];

void scan_input(char *prompt, char *ins)
{
    extract_external_commands(arr);

    while (1)
    {
        printf(ANSI_COLOR_GREEN "%s " ANSI_COLOR_RESET, prompt);
        fflush(stdout);

        if (fgets(ins, 200, stdin) == NULL)
        {
            printf("\nexit\n");
            free_list(head);
            free_external_commands(arr);
            exit(0);
        }

        ins[strcspn(ins, "\n")] = 0;

        if (ins[0] == '\0')
            continue;

      
        if (strncmp(ins, "PS1=", 4) == 0)
        {
            if (ins[4] == '\0' || ins[4] == ' ')
                printf("PS1: command not found\n");
            else
                strcpy(prompt, &ins[4]);
            continue;
        }

       
        if (strcmp(ins, "jobs") == 0)
        {
            print_list(head);
            continue;
        }

      
        if (strcmp(ins, "fg") == 0)
        {
            if (head == NULL)
            {
                printf("-bash: fg: current: no such job\n");
            }
            else
            {
                int stopped_pid = head->pid;
                char *stopped_cmd = strdup(head->cmd);
                delete_first(&head);

                printf("%s\n", stopped_cmd);
                fflush(stdout);
                free(stopped_cmd);

                signal(SIGINT,  SIG_IGN);
                signal(SIGTSTP, SIG_IGN);

                kill(stopped_pid, SIGCONT);
                waitpid(stopped_pid, &status, WUNTRACED);

                signal(SIGINT,  signal_handler);
                signal(SIGTSTP, signal_handler);

                if (WIFSTOPPED(status))
                {
                    pid = stopped_pid;
                    insert_at_first(&head);
                    printf("\n[1]+  Stopped   %s\n", input_string);
                    fflush(stdout);
                    pid = 0;
                }
            }
            continue;
        }

        if (strcmp(ins, "bg") == 0)
        {
            if (head == NULL)
            {
                printf("-bash: bg: current: no such job\n");
            }
            else
            {
                int bg_pid  = head->pid;
                char *bg_cmd = strdup(head->cmd);
                delete_first(&head);
                kill(bg_pid, SIGCONT);
                printf("[1]+ %s &\n", bg_cmd);
                fflush(stdout);
                free(bg_cmd);
            }
            continue;
        }

       
        strncpy(input_string, ins, sizeof(input_string) - 1);
        input_string[sizeof(input_string) - 1] = '\0';

        char *cmd = get_command(ins);

        if (cmd == NULL || cmd[0] == '\0')
        {
            printf("command not found\n");
            continue;
        }

        int cmdtype = check_command_type(cmd);

        if (cmdtype == BUILTIN)
        {
            execute_internal_commands(ins);
        }
        else if (cmdtype == EXTERNAL)
        {
            execute_external_commands(ins);
        }
        else
        {
            printf("%s: command not found\n", cmd); 
        }
    }
}

char *get_command(char *ins)             //working
{

   static char cmd[50];

   int i=0;

   while(ins[i]!=' ' && ins[i]!='\0')
   {
      cmd[i]=ins[i];
      i++;
   }
   cmd[i]='\0';

   return cmd;

}

int check_command_type(char *cmd)        //working
{
    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "cd")   == 0 ||
        strcmp(cmd, "pwd")  == 0 || strcmp(cmd, "echo") == 0)
    {
        return BUILTIN;
    }
 
   
    for (int i = 0; arr[i] != NULL; i++)
    {
        if (strcmp(cmd, arr[i]) == 0)
            return EXTERNAL;
    }
 
    
    char path_try[512];
    char *path_env = getenv("PATH");
    if (path_env)
    {
        char path_copy[2048];
        strncpy(path_copy, path_env, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
 
        char *dir = strtok(path_copy, ":");
        while (dir)
        {
            snprintf(path_try, sizeof(path_try), "%s/%s", dir, cmd);
            if (access(path_try, X_OK) == 0)
                return EXTERNAL;
            dir = strtok(NULL, ":");
        }
    }
 
    return NO_COMMAND;
}




void extract_external_commands(char **excmd)             //working
{

   int fd=open("cmd.txt",O_RDONLY);

     if(fd < 0)
    {
        perror("File not found\n");
        return;
    }

    char ch;

    char buffer[100];
    int i=0;
    int j=0;

    while(read(fd,&ch,1)>0)
    {


      if(ch=='\n')
      {

         buffer[i]='\0';
         excmd[j]=malloc(strlen(buffer)+1);
        
         strcpy(excmd[j],buffer);
          i=0;
          j++;
      }
      else
      {
         buffer[i++]=ch;

      }
    }

    excmd[j]=NULL;

    close(fd);

}

void execute_internal_commands(char *ins)       //working
{
    if (strcmp(ins, "exit") == 0)
    {
        free_list(head);
        free_external_commands(arr);
        exit(0);
    }
    else if (strncmp(ins, "cd", 2) == 0)
    {
         char *path = NULL;
        if (ins[2] == ' ' && ins[3] != '\0')
            path = ins + 3;
        else
            path = getenv("HOME");

        status = 0;
        if (path == NULL || chdir(path) != 0)
        {
            perror("cd");
            status = 1;
        }
    }
    else if (strcmp(ins, "pwd") == 0)
    {
        char path[1024];  
        if (getcwd(path, sizeof(path)) != NULL)
        {
            printf("%s\n", path);
            status = 0;
        }
        else
        {
            perror("pwd");
            status = 1;
        }
    }
    else if (strncmp(ins, "echo", 4) == 0)
    {
        if (strcmp(ins, "echo $$") == 0)
        {
            printf("%d\n", getpid());
        }
        else if (strcmp(ins, "echo $?") == 0)
        {
            printf("%d\n", WEXITSTATUS(status));  
        }
        else if (strcmp(ins, "echo $SHELL") == 0)
        {
            char *sh = getenv("SHELL");
            printf("%s\n", sh ? sh : "/bin/sh");   
        }
        else
        {
             if (ins[4] == ' ')
                printf("%s\n", ins + 5);
            else
                printf("\n");
        }
    }
}

void execute_external_commands(char *input_string)     //working
{
    char local[200];
    strncpy(local, input_string, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';

    char *argv[100];
    int argc = 0;

    char *token = strtok(local, " \t");
    while (token != NULL && argc < 99)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    argv[argc] = NULL;

    if (argc == 0) return;

    int cmd_start[50];
    int pipe_count = 0;
    cmd_start[0] = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "|") == 0)
        {
            argv[i] = NULL;
            pipe_count++;
            cmd_start[pipe_count] = i + 1;
        }
    }

    
    if (pipe_count == 0)
    {
        pid = fork();
        if (pid < 0) { perror("fork"); return; }

        if (pid == 0)
        {
             signal(SIGINT,  SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            execvp(argv[0], argv);
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(127);
        }
        else
        {
            
            signal(SIGINT,  SIG_IGN);
            signal(SIGTSTP, SIG_IGN);

            waitpid(pid, &status, WUNTRACED);

            
            signal(SIGINT,  signal_handler);
            signal(SIGTSTP, signal_handler);

            if (WIFSTOPPED(status))
            {
                insert_at_first(&head);
                printf("\n[%d]+  Stopped   %s\n", 1, input_string);
                fflush(stdout);
            }

            pid = 0;
        }
        return;
    }

   
    int prevfd = -1;
    int fd[2];

    signal(SIGINT,  SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    for (int i = 0; i <= pipe_count; i++)
    {
        if (i != pipe_count)
        {
            if (pipe(fd) == -1) { perror("pipe"); return; }
        }

        pid = fork();
        if (pid < 0) { perror("fork"); return; }

        if (pid == 0)
        {
            signal(SIGINT,  SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (prevfd != -1)
            {
                dup2(prevfd, STDIN_FILENO);
                close(prevfd);
            }
            if (i != pipe_count)
            {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            execvp(argv[cmd_start[i]], &argv[cmd_start[i]]);
            fprintf(stderr, "%s: command not found\n", argv[cmd_start[i]]);
            exit(127);
        }
        else
        {
            if (prevfd != -1) close(prevfd);
            if (i != pipe_count)
            {
                close(fd[1]);
                prevfd = fd[0];
            }
        }
    }

    if (prevfd != -1) close(prevfd);

    for (int i = 0; i <= pipe_count; i++)
        wait(&status);

    signal(SIGINT,  signal_handler);
    signal(SIGTSTP, signal_handler);

    pid = 0;
}





void insert_at_first(Slist **head)        //working   we are going to add the stopped cmd in this list  
{
    Slist *newn= malloc(sizeof(Slist));
    if (newn == NULL) 
    {
         perror("malloc"); 
         return; 
    }

    newn->pid = pid;
    newn->cmd = malloc(strlen(input_string) + 1);
    if (newn->cmd == NULL) 
    {
         free(newn); 
         perror("malloc"); 
         return; 
    }

    strcpy(newn->cmd, input_string);
    newn->link = *head;
    *head = newn;
}

void print_list(Slist *head)         //working   we print the list when user give cmd as jobs 
{
    int i = 0;
    while (head != NULL)
    {
        if (i == 0)
            printf("[%d]+   Stopped   %s\n", i + 1, head->cmd);
        else if (i == 1)
            printf("[%d]-   Stopped   %s\n", i + 1, head->cmd);
        else
            printf("[%d]    Stopped   %s\n", i + 1, head->cmd);
        head = head->link;
        i++;
    }
}

void delete_first(Slist **head)        //working   removing 1st stopped cmd when fg cmd found & also when user exit the shell
{
    if (*head == NULL)
    {
        printf("-bash: fg: current: no such job\n");
        return;
    }
    Slist *temp = *head;
    *head = (*head)->link;
    free(temp->cmd);
    free(temp);
}

void free_list(Slist *head)        //working   freeing the memory when the user exit the shell or when the user press ctrl+d
{
    Slist *temp;
    while (head != NULL)         
    {
        temp = head;
        head = head->link;
        free(temp->cmd);
        free(temp);
    }
}

void free_external_commands(char **ex)      //working  this function is for clearing the memory allocated for external commands 
{
    for (int i = 0; ex[i] != NULL; i++)
        free(ex[i]);
}

void signal_handler(int sign)        //working
{
    if (sign == SIGINT || sign == SIGTSTP)               // if this signal found than just print promt and flushout the stdout
    {
        printf("\n" ANSI_COLOR_GREEN "%s " ANSI_COLOR_RESET, prompt);
        fflush(stdout);
    }
    else if (sign == SIGCHLD)                // if this signal is found than just wait for the child process to finish and print the promt and flushout the stdout
    {
        waitpid(-1, NULL, WNOHANG);
    }
}