// POSIX Compliant shell
// TODO: Separate into header files and others and link in Makefile
// Potential solution for quotes to exlore - using strtok
// Also, will consider potentially cleaning input first for quotes and backslashes first, before reading in commands and args
#include <stdio.h>
#include <stdlib.h>  // getenv uses this
#include <string.h>
#include <ctype.h>
#include <unistd.h>  // For execv, access, X_OK and fork
#include <sys/wait.h> // For waitpid
#include <limits.h>

#define MAX_SIZE_OF_INPUT 4000
#define MAX_NUMBER_OF_ARGUMENTS 80
#define MAX_SIZE_OF_AN_ARGUMENT 300
#define MAX_SIZE_OF_A_COMMAND 400
#define MAX_DIRECTORY_BUFFER 4000

int get_command(char *input, char *command);
int read_arguments(char *input, char (*args)[MAX_SIZE_OF_AN_ARGUMENT], int starting_pos);
int num_words(const char *s);  // Helper function to compute the number of words in a string
int execute_command(const char *command, char (*args)[MAX_SIZE_OF_AN_ARGUMENT], int num_args);
int is_builtin(const char *command);
int is_executable(const char *command, char *executable_dir);
int execute_path_command(const char *executable_dir, char **argv);


int main(void) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while (1)
  {
    // Wait for user input
    char input[MAX_SIZE_OF_INPUT] = {0};
    printf("$ ");
    if (fgets(input, sizeof(input), stdin) == NULL)  // Keep reading in from input provided not a terminated shell
      break;
    input[strlen(input) - 1] = '\0';
    char command[MAX_SIZE_OF_A_COMMAND] = {0};
    int curr_pos = 0;
    int num_args;
    int result;
    if ((curr_pos = get_command(input, command)))
    {
      char args[MAX_NUMBER_OF_ARGUMENTS][MAX_SIZE_OF_AN_ARGUMENT];
      for (int i = 0; i < MAX_NUMBER_OF_ARGUMENTS; i++)
      {
        for (int j = 0; j < MAX_SIZE_OF_AN_ARGUMENT; j++)
        {
          args[i][j] = '\0';
        }
      }
      num_args = read_arguments(input, args, curr_pos);  // TODO: Will add some error handling for this soonn
      if ((result = execute_command(command, args, num_args)) || !strcmp(command, "exit"))
      {
        return result;
      }
    }

    else
      printf("%s: command not found\n", command);
  }
  
  return 0;
}










int get_command(char *input, char *command)
{
  int i;
  int in_single_quote = 0;
  int in_double_quote = 0;
  int curr_pos = 0;
  int incremented = 0;
  // Ignore double and single quotes as part of the shell command read
  for (i = 0; (input[curr_pos] != ' ' && input[curr_pos] != '\0'); i++)
  {
    incremented = 0;
    if (input[curr_pos] == '\"' && !in_single_quote)
    {
      in_double_quote = (in_double_quote) ? 0 : 1;
      curr_pos++;
    }

    if (input[curr_pos] == '\'' && !in_double_quote)
    {
      in_single_quote = (in_single_quote) ? 0 : 1;
      curr_pos++;
    }

    if (input[curr_pos] != ' ' && input[curr_pos] != '\0')
    {
      command[i] = input[curr_pos++];
      incremented = 1;
    }

    while (input[curr_pos] == ' ' && (in_double_quote || in_single_quote))
    {
      command[((incremented) ? ++i : i)] = input[curr_pos++]; // Inside ternary for if came from normal character or space
      incremented = 1;
    }

  }
  command[i] = '\0';

  char executable_dir[MAX_DIRECTORY_BUFFER] = {0}; // temporary, just so we can reuse the is_executable function, not needed anywhere
  return (is_builtin(command) || is_executable(command, executable_dir)) ? i : 0;
}

int read_arguments(char *input, char (*args)[MAX_SIZE_OF_AN_ARGUMENT], int starting_pos)
{
  int num_args = num_words(input) - 1;
  int curr_pos = starting_pos + 1;
  int j;
  int escaped_character = 0;
  for (int i = 0; i < num_args; i++)
  {
    // Ignore leading whitespace
    while (input[curr_pos] == ' ')
      curr_pos++;
    // Treat anything wrapped in single quotes as a single argument
    if (input[curr_pos] == '\'')
    {
      curr_pos++;
      for (j = 0; input[curr_pos] != '\'' && input[curr_pos] != '\0'; j++)
      {
        args[i][j] = input[curr_pos++];
      }

      // If next character is a quote, concatenate
      while (input[curr_pos + 1] == '\'' && input[curr_pos] == '\'')
      {
        curr_pos += 2;
        for (; input[curr_pos] != '\'' && input[curr_pos] != '\0'; j++)
        {
          args[i][j] = input[curr_pos++];
        }
      }
    }

    // Treat anything wrapped in double quotes as a single argument
    else if (input[curr_pos] == '\"' && escaped_character <= 0)
    {
      curr_pos++;
      for (j = 0; input[curr_pos] != '\"' && input[curr_pos] != '\0' && input[curr_pos] != '\\'; j++)
      {
        args[i][j] = input[curr_pos++];
      }

      // If current character is a backslash, get next character, for double quotes special characters
      while (input[curr_pos] == '\\')
      {
        int count = 0;
        if (input[curr_pos + 1] == '\\' || input[curr_pos + 1] == '\"')
        {
            args[i][j++] = input[++curr_pos];
            curr_pos++;
            count++;
        }


        for (; input[curr_pos] != '\"' && input[curr_pos] != '\0' && input[curr_pos] != '\\'; j++)
        {
          args[i][j] = input[curr_pos++];
          count++;
        }
        
        // If still stuck on escape and no movement happened, we should just simply read the rest until the end
        if (input[curr_pos] == '\\' && !count)
        {
          for (; input[curr_pos] != '\"' && input[curr_pos] != '\0'; j++)
          {
            args[i][j] = input[curr_pos++];
            count++;
          }
        }

      }

      // If next character is a quote, concatenate (skiping the quote). If no space, also concatenate
      while (((input[curr_pos + 1] == '\"' || input[curr_pos + 1] != ' ') && input[curr_pos] == '\"'))
      {
        curr_pos += (input[curr_pos + 1] == '\"') ? 2 : 1;
        for (; input[curr_pos] != '\"' && input[curr_pos] != '\0' && input[curr_pos] != '\\'; j++)
        {
          args[i][j] = input[curr_pos++];
        }

      // If current character is a backslash, get next character
      while (input[curr_pos] == '\\')
      {
        args[i][j++] = input[++curr_pos];
        curr_pos++;
        for (; input[curr_pos] != ' ' && input[curr_pos] != '\'' && input[curr_pos] != '\"' && input[curr_pos] != '\\' && input[curr_pos] != '\0'; j++)
        {
          args[i][j] = input[curr_pos++];
        }
      }
      }
    }
  
    else
    {
      for (j = 0; input[curr_pos] != ' ' && input[curr_pos] != '\'' && input[curr_pos] != '\"' && input[curr_pos] != '\\' && input[curr_pos] != '\0'; j++)
      {
        args[i][j] = input[curr_pos++];
      }


      // If current character is a backslash, get next character
      while (input[curr_pos] == '\\')
      {
        args[i][j++] = input[++curr_pos];
        curr_pos++;
        for (; input[curr_pos] != ' ' && input[curr_pos] != '\'' && input[curr_pos] != '\"' && input[curr_pos] != '\\' && input[curr_pos] != '\0'; j++)
        {
          args[i][j] = input[curr_pos++];
        }
      }

      // If next character is a single quote, concatenate
      while (input[curr_pos + 1] == '\'' && input[curr_pos] == '\'' && escaped_character <= 0)
      {
        curr_pos += 2;
        for (; input[curr_pos] != '\'' && input[curr_pos] != '\0'; j++)
        {
          args[i][j] = input[curr_pos++];
        }
      }

      // If next character is a double quote, concatenate
      while (input[curr_pos + 1] == '\"' && input[curr_pos] == '\"' && escaped_character <= 0)
      {
        curr_pos += 2;
        for (; input[curr_pos] != '\"' && input[curr_pos] != '\0'; j++)
        {
          args[i][j] = input[curr_pos++];
        }
      }
    }
    
    
    args[i][j++] = '\0';
    curr_pos++;
  }

  return num_args;
}

int execute_command(const char *command, char (*args)[MAX_SIZE_OF_AN_ARGUMENT], int num_args)
{
  if (!strcmp(command, "exit"))
      return atoi(args[0]);

  if (!strcmp(command, "echo"))
  {
    for (int i = 0; i < num_args; i++)
    {
      printf("%s ", args[i]);
    }

    printf("\n");
    return 0;
  }
  char executable_dir[MAX_DIRECTORY_BUFFER] = {0};
  if (!strcmp(command, "type"))
  {
      if (is_builtin(args[0]))
        printf("%s is a shell builtin\n", args[0]);
      
      else if (is_executable(args[0], executable_dir))
        printf("%s is %s\n", args[0], executable_dir);
      
      else
        printf("%s: not found\n", args[0]);
      return 0;
  }
  if (!strcmp(command, "pwd"))
  {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
      printf("%s\n", cwd);
      return 0;
    }

    else
    {
      printf("Oops, something went wrong\n");
      return 1;
    }
  }

  if (!strcmp(command, "cd"))
  {
      if (!strcmp(args[0], "~"))
        return chdir(getenv("HOME"));

      else if (chdir(args[0]) != 0)
        printf("cd: %s: No such file or directory\n", args[0]);

      return 0;

  }


  // Convert into a char * [] array (executes command in path)
  char *argv[num_args + 2];
  argv[0] = command;
  for (int i = 1; i < num_args + 1; i++)
    argv[i] = args[i - 1];
  argv[num_args + 1] = NULL;

  if (is_executable(command, executable_dir))
  {
    return execute_path_command(executable_dir, argv);
  }

  printf("Oops, something went wrong\n");
  return 1;
}

int execute_path_command(const char *executable_dir, char **argv)
{
  // We need to create a child process because execv replaces the current process completely, which would cause the shell to exit always
  pid_t pid = fork(); 
  
  if (pid == 0)
  {
    execv(executable_dir, argv);
    // This is simply a fallback in case execv fails for any reason, since if it executes correctly, it automatically terminates
    // the process before this line
    return 1;
  }

  else if (pid > 0)
  {
    // In the parent we should wait for the child to complete
    int status;
    waitpid(pid, &status, 0);
    return 0;
  }

  else
  {
    // Fork failed, terminate the shell
    return 1;
  }

}

int is_executable(const char *command, char *executable_dir)
{
  char *temporary_path = getenv("PATH");  // To avoid strtok updating this
  if (temporary_path == NULL)
  {
    // No path variable set
    return 0;
  }
  char path[strlen(temporary_path)];
  strcpy(path, temporary_path);

  char *curr_dir = strtok(path, ":");
  do
  {
    int max_executable_path_size = (int) strlen(curr_dir) + strlen(command) + 2;
    char executable_path[max_executable_path_size];
    // Construct path (i.e., combine the curr directory and the command with a '/')
    int second_index = 0;
    int i;
    for (i = 0; i < max_executable_path_size; i++)
    {
      if (i < (int) strlen(curr_dir))
        executable_path[i] = curr_dir[i];
      else if (i == (int) strlen(curr_dir))
        executable_path[i] = '/';
      else
        executable_path[i] = command[second_index++];
    }

    executable_path[i] = '\0';
    if (access(executable_path, X_OK) == 0)
    {
      strcpy(executable_dir, executable_path);
      return 1;
    }
  } while ((curr_dir = strtok(NULL, ":")) != NULL);

  return 0;

}

int is_builtin(const char *command)
{
 return (!strcmp(command, "exit") || !strcmp(command, "echo") || !strcmp(command, "type") || !strcmp(command, "pwd") || !strcmp(command, "cd"));
}

int num_words(const char *s)
{
  int words = 0;
  int found_word = 0;
  int in_single_quote = 0;
  int in_double_quote = 0;
  for (int i = 0; i < (int) strlen(s); i++)
  {
    // Count inside quotes as a single word, as long as there are spaces
    if (s[i] == '\'' && !in_double_quote)
    {
      in_single_quote = (in_single_quote) ? 0 : 1;
      // Anything inside quotes is a word, as long as there is something insde
      if (in_single_quote && s[i + 1] != '\'')
      {
        words++;
        found_word = 1;
      }

    }

     // Count inside double quotes as a single word, as long as there are spaces
    else if (s[i] == '\"' && !in_single_quote)
    {
      in_double_quote = (in_double_quote) ? 0 : 1;
      // Anything inside quotes is a word, as long as there is something inside
      if (in_double_quote && s[i + 1] != '\"')
      {
        words++;
        found_word = 1;
      }
    }

    else if (s[i] != ' ' && s[i] != '\t' && !found_word)
    {
      words++;
      found_word = 1;
    }

    else if ((s[i] == ' ' || s[i] == '\t') && !in_single_quote && !in_double_quote)
      found_word = 0;
  }
  return words;
}
