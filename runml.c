//  CITS2002 Project 1 2024
//  Student1:   23722513  Jignesh Ramasamy
//  Platform:   Apple MacOS
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //provides function for getpid  

// i used the preprocessor to allow the code not to have embedded numbers. (to meet requirements of project specifications)
#define MAX_LINE_LENGTH 256 // assumption made for maximum length of a line in the .ml file 
#define MAX_IDENTIFIER_LENGTH 12 // maximum length for identifiers such as variables or functions (Project specification)
#define MAX_IDENTIFIERS 50 // maximum number of unique identifiers allowed in the program (also a nother assumption made)
#define MAX_ARGUMENTS 10 // defaulted the number of command-line arguments supported to 10 (project specification)

// created a struct for representing variables with a name and an initialization flag. Meets data structure requirmenet
typedef struct {
  char name[MAX_IDENTIFIER_LENGTH + 1]; //stores name of the variable
  int initialized; //created a varianle that tracks if the variable is initialized (flag created)
} Variable;

typedef struct { // This struct i created to represent global assignments x <- 2.718
  char name[MAX_IDENTIFIER_LENGTH + 1]; // The variable name being assigned to
  char value[MAX_LINE_LENGTH]; // and the expression or value assigned to the variable
} Assignment;

typedef struct { // this struct is  to represent function definitions 
  char name[MAX_IDENTIFIER_LENGTH + 1]; // passes function name
  int param_count; // number of parameters for the function
  char params[MAX_IDENTIFIERS][MAX_IDENTIFIER_LENGTH + 1]; // pass the list of paramete rnames
  int returns_value; // and finally a flag indicating if the function returns a value good practice to let the program know that the condition has met
} Function;

// then i created arrays to store the declared variables, assignments, and functions
Variable global_variables[MAX_IDENTIFIERS]; // global variables in the program 
int global_variable_count = 0; // num of global variables to avoid exceeding the maximum number of variables and to prevent duplicate declarations in the generated C code

Variable function_variables[MAX_IDENTIFIERS]; // stores variables atht are local to a function i did this to separate global variables with local variable so they are treated differently if the syntax requires this
int function_variable_count = 0; //  counts  local variables as it helps in managing the function's scope prevents the reuse of the same variable names in different functions or between global and local scopes

Assignment assignments[MAX_IDENTIFIERS]; // stores variables that are global it makes sure that all global assignments are stored, and they are written to the C code during the transpilation process
int assignment_count = 0; //  allows  transpiler to know how many assignments need to be processed helps generate the corresponding assignment in the C code during the second pass of the transpilation (Below)

Function functions[MAX_IDENTIFIERS]; // List of functions in the detected in the ml program 
int function_count = 0; //tracks  number of functions, declared functions must be defined before they are called in the ml language according to requirement, makes sure all functions are properly declared and transpiled into C. The counter helps with managing this and controlling the order of processing.
char current_function_name[MAX_IDENTIFIER_LENGTH + 1]; // stores the name of the function currently being processed by the transpiler.
int inside_function = 0; // flag tracks whether the transpiler is currently inside a function body helps differentiate between processing global statements and functionn logic
int line_number = 0; //detects the line numbner is on in the ml code. I made this specifically for error processing so that the error message gives the line too. Made it easier for debugging

// found that functions are called before their full definition appears in the code, so their prototypes makes during the compiler knows their structure ahead of time.
void error(const char *message); 
int ends_with_ml(const char *filename); // check to see if the input file has a .ml extension
void transpile_ml_to_c(FILE *ml_file, FILE *c_file); // main function to transpile .ml to C
void process_global_assignment(const char *line);
void process_function_declaration(FILE *ml_file, const char *line,
                                  FILE *c_file);
void process_function_body(FILE *ml_file, FILE *c_file,
                           int *has_return_statement);
void process_statement(const char *line, FILE *c_file, int in_function,
                       const char *indent, int *has_return_statement,
                       Variable *variables, int *variable_count);
void process_expression(const char *expr, FILE *c_file, Variable *variables,
                        int *variable_count);
char *parse_expression(char **expr, Variable *variables, int *variable_count); // recursive function for parsing expressions for expressions +,-,/,* 
char *parse_term(char **expr, Variable *variables, int *variable_count); // this function handles terms within an expression.
char *parse_factor(char **expr, Variable *variables, int *variable_count); //This function is responsible for var, constant, funct etc
void skip_whitespace(char **expr);
void remove_comments(char *line);
int is_valid_identifier(const char *identifier);
int variable_exists(const char *identifier, Variable *variables,
                    int variable_count);
void declare_variable(const char *identifier, Variable *variables,
                      int *variable_count);
int does_function_return_value(const char *func_name);
void compile_and_run(const char *c_filename, const char *exe_filename, int argc,
                     char *argv[]);
const char *get_c_function_name(const char *func_name);
int variable_exists_in_global(const char *identifier);

//Now implementation ofthese prorotypes as methods of each function.
int ends_with_ml(const char *filename) {
  size_t len = strlen(filename);  //calculates the length of the filename string
  return len > 3 && strcmp(filename + len - 3, ".ml") == 0; //check if the filename is at least 4 characters long since we have.ml extension atleast 4 characters needed
} // and then compares the last 3 characters of the filename with the string ".ml"

//for error messaging and identification all errors passed through this funtion andgoes through stderr and ! syntax as required.
void error(const char *message) {
  fprintf(stderr, "! Error present at line %d: %s\n", line_number, message);   // print the error message to the standard error (stderr) stream with the line number where the error occurred.
  exit(EXIT_FAILURE); //error termination
}

void declare_variable(const char *identifier, Variable *variables,
                      int *variable_count) {
  if (*variable_count >= MAX_IDENTIFIERS) {
    error("! Too many identifiers present >50");
  }   //if it has more than 50 identifiers, an error is raised and the program terminates. not syntax error so dont need stderr. 
  if (!variable_exists(identifier, variables, *variable_count)) {
    if (variable_exists_in_global(identifier)) { 
      // this checks if the variable exists globally, there's no need to redeclare it in the local scope, so we exit.
      return;
    }
    // then is declares it in the local scope
    strcpy(variables[*variable_count].name, identifier);
    variables[*variable_count].initialized = 0; //set initialized flag to 0, indicating that the variable hasn't been assigned a value yet.
    (*variable_count)++;    // Increment the variable count to track how many local variables have been declared.
    // If the variable doesn't exist locally or globally, declare it in the local scope Copies the identifier (variable name) into the array of local variables.
  }
}

int variable_exists(const char *identifier, Variable *variables, int variable_count) {
  // iterate the local variables which are variables passed as function arguments
  // loop checks if  given identifier matches any local variables in the current scope.
  for (int i = 0; i < variable_count; i++) {
    // compare  name of the current variable in the list with the identifier.
    // If match  returns 1 to show that the variable exists in the local scope.
    if (strcmp(variables[i].name, identifier) == 0) {
      return 1; // if variable found in local scoep
    }
  }

  // this is for if the variable was not found locally, checks the global variables.
  // to make sure since global variables are accessible within functions, but are declared outside.
  for (int i = 0; i < global_variable_count; i++) {
    //compares the name of the current global variable with the identifier.
    // if match, returns 1 to show that the variable exists in the global scope.
    if (strcmp(global_variables[i].name, identifier) == 0) {
      return 1; // Variable found in global scope
    }
  }

  //if the variable was not found in either the local or global scope,
  //return 0 to indicate that the variable does not exist.
  return 0; // Variable not found
}

int variable_exists_in_global(const char *identifier) {

  //this loop checks if the given identifier matches any of the global variables.
  for (int i = 0; i < global_variable_count; i++) {
    //compares the name of the current global variable with the identifier.
    //if match found returns 1 to show the variable exists in the global scope.
    if (strcmp(global_variables[i].name, identifier) == 0) {
      return 1;  // Variable found in global scope
    }
  }

  //this section shows that the identifier does not exist in the global scope.
  return 0;  // Variable not found in global scope
}

void process_global_assignment(const char *line) {
  //variables to store the left-hand side and right-hand side of the assignment.
  char lhs[MAX_IDENTIFIER_LENGTH + 1];  // i created a buffer to hold the variable name being assigned to lhs.
  char rhs[MAX_LINE_LENGTH];  //and anothr bufffer to hold the expression or value assigned to the variable rhs

  // then used sscanf to parse the line in the format: "lhs <- rhs"
  // %s captures the lhs (variable name) and %[^\n] captures the remainder of the line as the rhs (value/expression).
  if (sscanf(line, "%s <- %[^\n]", lhs, rhs) == 2) {
    
    //check if the parsed lhs (variable name) is a valid identifier according to the language rules.
    if (is_valid_identifier(lhs)) {
      
      // If the identifier is valid it declares the variable in the global scope.
      // makes sure  the variable is properly registered in the global variables list.
      declare_variable(lhs, global_variables, &global_variable_count);
      
      //stores the assignment (lhs and rhs) for later use during code generation.
      //the assignment is saved in the assignments array to be used when generating the C code.
      strcpy(assignments[assignment_count].name, lhs);  //copies the variable name (lhs) into the assignments array.
      strcpy(assignments[assignment_count].value, rhs);  //copies the value/expression (rhs) into the assignments array.
      
      // Increment the assignment count to keep track of the number of global assignments processed.
      assignment_count++;
      
    } else {
      // If the lhs is not a valid identifier, report an error with the appropriate message.
      error("! Identifier not valid in global assignment");
    }
  }
}

void process_function_declaration(FILE *ml_file, const char *line, FILE *c_file) {
  // Implemented a multi pass system to read the file. each pass serves for different purpose
  char func_name[MAX_IDENTIFIER_LENGTH + 1];  // stores function name
  char params_line[MAX_LINE_LENGTH];  //stores list of parameters

  // the format expected is: "function func_name param1 param2 etc".
  if (sscanf(line, "function %12s %[^\n]", func_name, params_line) >= 1) {
    
    //check if the function name is valid according to the project speciication rules
    if (!is_valid_identifier(func_name)) {
      error("! Function name does not follow syntax rules");  // pass to error function to stderr
    }
    
    //parse the function parameters
    int param_count = 0;  //counter to keep track of the number of parameters
    char params[MAX_IDENTIFIERS][MAX_IDENTIFIER_LENGTH + 1];  //array to holds the parameter names
    
    // extraction of individual parameters from the parameter declartion in ml 
    char *param = strtok(params_line, " ");
    while (param) {
      if (is_valid_identifier(param)) {
        //copies the valid parameter name to the data structure above (param)
        strcpy(params[param_count++], param);
      } else {
        error("! Paramater name is not valid according to syntax rules");  //pass to error function for invalid parameter names
      }
      //next paramater extraction
      param = strtok(NULL, " ");
    }
    
    //store the function definition in the functions array
    strcpy(functions[function_count].name, func_name);  
    functions[function_count].param_count = param_count;  
    functions[function_count].returns_value = 0;  
    
    //copy the parameter names to the function's parameter list
    for (int i = 0; i < param_count; i++) {
      strcpy(functions[function_count].params[i], params[i]);
    }
    
    //increment the function count to track how many functions have been declared
    function_count++;
    
    //prepare to process the function body
    inside_function = 1;  //mark that we are inside a function definition
    strcpy(current_function_name, func_name);  //set the current function name for tracking
    int has_return_statement = 0;  
    
    //save the current file position so we can later rewind to process the function body this is part of the pass process i implemented
    long function_body_start = ftell(ml_file);
    
    // this is the beginnign of the pass process - first pass: Check for return statements and collect local variables
    function_variable_count = 0;  //reset the count of local variables for this function
    
    //declares the function's parameters as local variables
    for (int i = 0; i < param_count; i++) {
      declare_variable(params[i], function_variables, &function_variable_count);
    }
    
    //declares a 'result' variable to hold the return value if needed its needed in the transpiled code
    declare_variable("result", function_variables, &function_variable_count);
    
    //first pass to check for return statements and analyze the function body
    process_function_body(ml_file, NULL, &has_return_statement);
    
    //updates the function definition to indicate if the function returns a value
    functions[function_count - 1].returns_value = has_return_statement;
    
    //then after the first pass i decided to rewind to the start of the function body for the second pass
    fseek(ml_file, function_body_start, SEEK_SET);
    line_number--;  //adjust line number to account for rewinding
    
    //output the function definition in C
    //check if the function returns a value (double) or is void
    if (has_return_statement) {
      fprintf(c_file, "double %s(", get_c_function_name(func_name));
    } else {
      fprintf(c_file, "void %s(", get_c_function_name(func_name));
    }
    
    //output the function parameters in the C code
    for (int i = 0; i < param_count; i++) {
      fprintf(c_file, "double %s", params[i]);
      if (i < param_count - 1) {
        fprintf(c_file, ", ");
      }
    }
    fprintf(c_file, ") {\n");  //open the function body in C
    
    fprintf(c_file, "    double result = 0.0;\n"); // declares a result variable in every transpiled function, initializing it to 0.0 for storing return values or intermediate results.
    
    //declare other local variables used in the function
    for (int i = 0; i < function_variable_count; i++) {
      const char *var_name = function_variables[i].name;
      int is_param = 0;  //check if the variable is a parameter
      
      //skip the result variable since it is already declared
      if (strcmp(var_name, "result") == 0) continue;
      
      //check if the variable is a function parameter
      for (int j = 0; j < param_count; j++) {
        if (strcmp(var_name, params[j]) == 0) {
          is_param = 1; 
          break;
        }
      }
      
      //declare the variable only if it is not a parameter and doesn't exist globally
      if (!is_param && !variable_exists_in_global(var_name)) {
        fprintf(c_file, "    double %s = 0.0;\n", var_name);
      }
    }
    
    //this is the second pass where it process the function body and output the C code
    process_function_body(ml_file, c_file, NULL);
    
    //close the function definition in C
    fprintf(c_file, "}\n");
    
    //mark to show that we are no longer inside a function
    inside_function = 0;
  }
}

void process_function_body(FILE *ml_file, FILE *c_file, int *has_return_statement) {
    char line[MAX_LINE_LENGTH];   //store each line from the file
    long last_pos;   //variable to store the file position before reading each line

    //this reads each line from the ml_file until end-of-file. Used ftell to map it to the last position. 
    while ((last_pos = ftell(ml_file)), fgets(line, MAX_LINE_LENGTH, ml_file)) {
        line_number++;  //incremented line_number to allow for error handling, made it easier for debugging

        remove_comments(line);   //remove comments anything after # to focus only on code (Proj specification)

        //remove the trailing newline character from the line if needed, some samples needed this
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';   //replace newline with null terminator to clean the string
            len--;   //adjust the length to reflect the new size
        }

        //check for indentation in the .ml syntax, function bodies must be indented with a tab. (Project specificaition) im assuming no space in the syntax only tab.
        if (line[0] != '\t') {
            //if the line isn't indented with a tabs signals the end of the function body
            //reset file position to the last known position before this line
            fseek(ml_file, last_pos, SEEK_SET);   
            line_number--;   //decrease the line number since this line doesn't belong to the function body
            break;   //exit the loop as the function body has ended
        }

        //remove the leading tab to clean the line for further processing
        char *trimmed_line = line + 1;

        //skip lines that are either completely empty or contain only spaces/tabs
        char *p = trimmed_line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0')
            continue;   //ignore this line and move to the next iteration

        //process the remaining valid statement after removing the tab
        //pass 1 to indicate that this is inside a function, and the indentation string "    " for the C code
        process_statement(trimmed_line, c_file, 1, "    ", has_return_statement, function_variables, &function_variable_count);
    }
}

void process_statement(const char *line, FILE *c_file, int in_function,
                       const char *indent, int *has_return_statement,
                       Variable *variables, int *variable_count) {
  // i intialised a pointer names p to iterate through the line
  //skip leading spaces and tabs to handle indentation and avoid unnecessary processing
  const char *p = line;
  while (*p == ' ' || *p == '\t')
    p++;

  //if the line is empty only whitespace or newline, return without processing.
  if (*p == '\0')
    return;

  //check if the line starts with the print tatement.
  //the function assumes that valid print statements start with print followed by a space or end of line. (Ml syntax rules)
  if (strncmp(p, "print", 5) == 0 && (p[5] == ' ' || p[5] == '\0')) {
    //handle the "print" statement by extracting the expression to be printed.
    char expr[MAX_LINE_LENGTH];
    if (sscanf(p + 5, " %[^\n]", expr) != 1) {
      error("! Invalid expression after print statement"); //error if no valid expression follows the "print"
    }
    
    //if the file pointer is not null, write the C equivalent print statement into the output C file.
    if (c_file) {
      fprintf(c_file, "%sresult = ", indent);  //begin assigning result of expression.
    }

    //process the expression and generate C code if required
    process_expression(expr, c_file, variables, variable_count);

    if (c_file) {
      //print the result formatting as either integer or floating-point based on the value
      fprintf(c_file, ";\n");
      fprintf(c_file,
              "%sif ((double)(int)result == result) printf(\"%%.0f\\n\", result);\n",
              indent);  //print as integer if result is whole number.
      fprintf(c_file, "%selse printf(\"%%.6f\\n\", result);\n", indent);  //print as float if decimal.
    }

  } else if (strncmp(p, "return", 6) == 0 && (p[6] == ' ' || p[6] == '\0')) {
    //handle the return statement.
    //makes sure the return statement is valid only within a function context.
    if (!in_function) {
      error("! Check indentation is with tab: Return statement outside of a function");  //error if return is outside a function.
    }

    char expr[MAX_LINE_LENGTH];
    if (sscanf(p + 6, " %[^\n]", expr) != 1) {
      error("! return statement is not valid");  //error if no valid expression after return.
    }

    if (c_file) {
      fprintf(c_file, "%sreturn ", indent);  //rrite the C return statement in the output file.
    }

    process_expression(expr, c_file, variables, variable_count);

    if (c_file) {
      fprintf(c_file, ";\n");
    }

    //mark that this function has a return statement.
    if (has_return_statement) {
      *has_return_statement = 1;
    }

  } else if (strstr(p, "<-")) {
    //handle variable assignment with the "<-" operator.
    char lhs[MAX_IDENTIFIER_LENGTH + 1];  //left-hand side of assignment (variable name).
    char rhs[MAX_LINE_LENGTH];            //rdight-hand side (expression to be assigned).

    //extract the variable name and expression from the line.
    if (sscanf(p, "%12s <- %[^\n]", lhs, rhs) == 2) {
      if (is_valid_identifier(lhs)) {
        //if the variable does not exist, declare it.
        if (!variable_exists(lhs, variables, *variable_count)) {
          declare_variable(lhs, variables, variable_count);
          if (c_file && !variable_exists_in_global(lhs)) {
            //declare the variable in the C code, initializing it to 0.0.
            fprintf(c_file, "%sdouble %s = 0.0;\n", indent, lhs);
          }
        }

        //generate the assignment statement in C.
        if (c_file) {
          fprintf(c_file, "%s%s = ", indent, lhs);
        }

        //process the expression being assigned to the variable.
        process_expression(rhs, c_file, variables, variable_count);

        if (c_file) {
          fprintf(c_file, ";\n");
        }
      } else {
        error("Identifier is invalid in assignment");  //error if the variable name is invalid. stopped placing ! in error messages as they are redundant due to error function already doing this. 
      }
    } else {
      error("Identifier is invalid in statement");  //error if assignment syntax is wrong.
    }

  } else {
    //handle potential function calls or expressions.
    const char *ptr = p;  //pointer to iterate through the statement.
    char identifier[MAX_IDENTIFIER_LENGTH + 1];  // this is the function name or variable identifier.
    int i = 0;

    //xtract the identifier function name or variable name
    while ((isalpha(*ptr)) && i < MAX_IDENTIFIER_LENGTH) {
      identifier[i++] = *ptr++;
    }
    identifier[i] = '\0';  //null-terminate the identifier.

    skip_whitespace((char **)&ptr);  //skip any whitespace after the identifier.

    //check if it's a function call by looking for the opening parenthesis '(' assumption from the ml syntax given.
    if (*ptr == '(') {
      ptr++;  //skip the '('.
      char args[MAX_LINE_LENGTH] = "";  //to store function arguments

      skip_whitespace((char **)&ptr);
      if (*ptr != ')') {
        //loop through arguments and process them
        while (*ptr && *ptr != ')') {
          char *arg = parse_expression((char **)&ptr, variables, variable_count);
          strcat(args, arg);  //append argument to the argument list using the strcat function in c
          free(arg);
          skip_whitespace((char **)&ptr);

          if (*ptr == ',') {
            strcat(args, ", ");  //handle comma-separated arguments
            ptr++;
            skip_whitespace((char **)&ptr);
          } else if (*ptr != ')') {
            error("Expected a separator (',') or closing parenthesis in function call");  //error for invalid syntax
          }
        }
      }

      if (*ptr != ')') {
        error("Expected a ')' in function call");  // Error if ')' is missing
      }
      ptr++;  //skip the closing ')' after function arguments

      int func_returns = does_function_return_value(identifier);  //check to see if the function returns a value.

      if (c_file) {
        if (func_returns) {
          //generates C code for function call with a return value.
          fprintf(c_file, "%sresult = %s(%s);\n", indent, get_c_function_name(identifier), args);
        } else {
          //generates C code for function call without a return value.
          fprintf(c_file, "%s%s(%s);\n", indent, get_c_function_name(identifier), args);
        }

        //if outside a function and the function returns a value, print the result.
        if (!in_function && func_returns) {
          fprintf(c_file,
                  "%sif ((double)(int)result == result) printf(\"%%.0f\\n\", result);\n",
                  indent);
          fprintf(c_file, "%selse printf(\"%%.6f\\n\", result);\n", indent);
        }
      }

    } else {
      //process the statement as a general expression.
      if (c_file) {
        fprintf(c_file, "%sresult = ", indent);  //start with result = " for expression evaluation.
      }

      //process the expression.
      process_expression(p, c_file, variables, variable_count);

      if (c_file) {
        fprintf(c_file, ";\n");
      }
    }
  }
}

//this ensures that the next non-whitespace character is processed
void skip_whitespace(char **expr) {
  while (**expr == ' ' || **expr == '\t')
    (*expr)++;  //move the pointer forward until a non-whitespace character is found
}


char *parse_expression(char **expr, Variable *variables, int *variable_count) {
  char *left = parse_term(expr, variables, variable_count); //parse the first term (left-hand side of the expression) using parse_term()
  skip_whitespace(expr);  //function call to skip any whitespace after the first term

  while (**expr == '+' || **expr == '-') {  // continue parsing if the next character is a '+' or '-' this handles addition or subtraction expressions
    char op = **expr;  //stores the operator ('+' or '-')
    (*expr)++;  //moves the pointer past the operator

    skip_whitespace(expr);

    char *right = parse_term(expr, variables, variable_count);

    
    char *combined = malloc(strlen(left) + strlen(right) + 4); //use ofe malloc() so theres enough memory to hold a new string that combines the left-hand term, 
    //the operator, and the right-hand term, along with extra characters for the surrounding parentheses and spaces;

    //combines the left term, operator, and right term into a string "(left op right)"
    sprintf(combined, "(%s %c %s)", left, op, right);

    //free the memory for the left and right terms since they are now part of 'combined'
    free(left);
    free(right);

    //the result of this step becomes the new 'left' term for further evaluation
    left = combined;

    //skip any whitespace after the current term
    skip_whitespace(expr);
  }

  //return the final expression string
  return left;
}

char *parse_term(char **expr, Variable *variables, int *variable_count) {
  char *left = parse_factor(expr, variables, variable_count);  //parses the first factor
  skip_whitespace(expr);  //call to function to skip whitespaces
  
  while (**expr == '*' || **expr == '/') {  //handle * and / operators i covered the basic expressions
    char op = **expr;  //store the operator to be used
    (*expr)++;  //move past the operator to detect expression values
    skip_whitespace(expr);  //call again to kip spaces/tabs
    
    char *right = parse_factor(expr, variables, variable_count);  //parse the right-hand factor
    
    //use of malloc again to llocate memory for combined expression left op right
    char *combined = malloc(strlen(left) + strlen(right) + 4);  
    sprintf(combined, "(%s %c %s)", left, op, right);  //combine into a formatted string

    free(left);  //free the old left term
    free(right);  //free the old right term
    left = combined;  // update left to be the combined expression
    skip_whitespace(expr);  //skip spaces/tabs
  }
  
  return left;  // Return the final expression
}

char *parse_factor(char **expr, Variable *variables, int *variable_count) {
  skip_whitespace(expr);  // call again to skip any leading spaces/tabs
  char *result = NULL;

  //handle for unary plus or minus
  if (**expr == '+' || **expr == '-') {
    char op = **expr;  //store the unary operator
    (*expr)++;
    skip_whitespace(expr);  //skip spaces/tabs
    char *factor = parse_factor(expr, variables, variable_count);  //parse the factor after the unary operator
    result = malloc(strlen(factor) + 4);  //use of malloc again to llocate memory for op factor
    sprintf(result, "(%c%s)", op, factor);  //format the result
    free(factor);
    return result;  //return the unary expression
  }

  //handle parenthesizedd expressions for more advanced arithmetics
  if (**expr == '(') {
    (*expr)++;  //skip '('
    result = parse_expression(expr, variables, variable_count);  //parse the expression inside parentheses
    skip_whitespace(expr);
    if (**expr != ')') {
      error("missing the closing parenthesis");
    }
    (*expr)++;  //skip ')'
  }
  //handle function calls or identifiers
  else if (isalpha(**expr)) {
    char identifier[MAX_IDENTIFIER_LENGTH + 1];  // =buffer to hold the identifier name
    int i = 0;
    //extract the identifier (function or variable name)
    while ((isalpha(**expr) || isdigit(**expr)) && i < MAX_IDENTIFIER_LENGTH) {
      identifier[i++] = **expr;
      (*expr)++;
    }
    identifier[i] = '\0';
    skip_whitespace(expr);
    
    //check if it's a function call
    if (**expr == '(') {
      (*expr)++;  //skip '('
      char args[MAX_LINE_LENGTH] = "";  // another buffer to store function arguments
      skip_whitespace(expr);

      //parse function arguments
      if (**expr != ')') {
        while (**expr && **expr != ')') {
          char *arg = parse_expression(expr, variables, variable_count);
          strcat(args, arg);
          free(arg);
          skip_whitespace(expr);
          if (**expr == ',') {
            strcat(args, ", ");  //handle comma-separated arguments in function calls
            (*expr)++;
            skip_whitespace(expr);
          } else if (**expr != ')') {
            error("Expected ',' or ')' in function call");
          }
        }
      }
      if (**expr != ')') {
        error("Expected ')' in function call");
      }
      (*expr)++;  //skip ')'

      //gnerate function call code
      int func_returns = does_function_return_value(identifier);
      if (!func_returns) {
        error("Function in expression has no return value");
      }
      result = malloc(strlen(identifier) + strlen(args) + 3);  // use of malloc to allocate memory for the function call
      sprintf(result, "%s(%s)", get_c_function_name(identifier), args);  //format the function call
    } else {

      if (!variable_exists(identifier, variables, *variable_count)) {
        declare_variable(identifier, variables, variable_count);  //declare the variable if it doesn't exist
      }
      result = malloc(strlen(identifier) + 1);
      strcpy(result, identifier);  //copy the identifier to result
    }
  }
  //handle numbers (constants)
  else if (isdigit(**expr) || **expr == '.') {
    char number[64];  //another uffer for the number
    int i = 0;
    while ((isdigit(**expr) || **expr == '.') && i < 63) {
      number[i++] = **expr;
      (*expr)++;
    }
    number[i] = '\0';
    result = malloc(strlen(number) + 1);
    strcpy(result, number);  //copy the number to result
  }
  //invalid factor
  else {
    error("factor in expression is not balid");
  }
  
  return result;  //return the parsed factor
}

//processes an expression and writes its C equivalent to the output file
void process_expression(const char *expr, FILE *c_file, Variable *variables, int *variable_count) {
  
  //check if the expression is empty
  if (strlen(expr) == 0) {
    error("Empty expression detected");
  }

  //duplicate the expression to work on a local copy
  char *expr_copy = strdup(expr);
  char *expr_ptr = expr_copy;

  //parse the expression and convert it to valid C code
  char *parsed_expr = parse_expression(&expr_ptr, variables, variable_count);

  //ensure no extra characters remain after parsing the expression
  skip_whitespace(&expr_ptr);
  if (*expr_ptr != '\0') {
    error("Unexpected characters in expression");
  }

  //if a file is provided, write the parsed expression (C code) to the output file
  if (c_file) {
    fprintf(c_file, "%s", parsed_expr);
  }

  //free dynamically allocated memory
  free(parsed_expr);
  free(expr_copy);
}

void remove_comments(char *line) {
  char *comment_pos = strchr(line, '#');
  if (comment_pos) {
    *comment_pos = '\0';
  }
  //remove trailing whitespace
  size_t len = strlen(line);
  while (len > 0 && isspace(line[len - 1])) {
    line[--len] = '\0';
  }
}

int is_valid_identifier(const char *identifier) {
  int len = strlen(identifier);
  if (len < 1 || len > MAX_IDENTIFIER_LENGTH)
    return 0;
  //check if it's 'arg' followed by digits
  if (strncmp(identifier, "arg", 3) == 0 && len > 3) {
    for (int i = 3; i < len; i++) {
      if (!isdigit(identifier[i]))
        return 0;
    }
    return 1;
  }
  //otherwise, ensure all characters are lowercase letters
  for (int i = 0; i < len; i++) {
    if (!islower(identifier[i]))
      return 0;
  }
  return 1;
}

int does_function_return_value(const char *func_name) {
  for (int i = 0; i < function_count; i++) {
    if (strcmp(functions[i].name, func_name) == 0) {
      return functions[i].returns_value;
    }
  }
  return 0; //assume function does not return a value if not found
}

const char *get_c_function_name(const char *func_name) {
  if (strcmp(func_name, "main") == 0) {
    return "ml_main";
  }
  return func_name;
}

void transpile_ml_to_c(FILE *ml_file, FILE *c_file) {
  char line[MAX_LINE_LENGTH];
  fprintf(c_file, "#include <stdio.h>\n");
  fprintf(c_file, "#include <stdlib.h>\n\n");
  //first pass: Process global assignments and collect function declarations
  line_number = 0;
  rewind(ml_file);
  while (fgets(line, MAX_LINE_LENGTH, ml_file)) {
    line_number++;
    remove_comments(line);
    //skip empty lines
    char *p = line;
    while (*p == ' ' || *p == '\t' || *p == '\n')
      p++;
    if (*p == '\0')
      continue;
    //process global assignments
    if (strstr(line, "<-")) {
      process_global_assignment(line);
      continue;
    }
    //skip function declarations and their bodies
    if (strncmp(p, "function", 8) == 0) {
      // skip function body
      while (fgets(line, MAX_LINE_LENGTH, ml_file)) {
        line_number++;
        if (*line != '\t') {
          fseek(ml_file, -strlen(line), SEEK_CUR);
          line_number--;
          break;
        }
      }
      continue;
    }
  }
  //declare global variables
  for (int i = 0; i < global_variable_count; i++) {
    fprintf(c_file, "double %s = 0.0;\n", global_variables[i].name);
  }
  //second pass: Process function declarations and definitions
  line_number = 0;
  rewind(ml_file);
  while (fgets(line, MAX_LINE_LENGTH, ml_file)) {
    line_number++;
    remove_comments(line);
    //skip empty lines
    char *p = line;
    while (*p == ' ' || *p == '\t' || *p == '\n')
      p++;
    if (*p == '\0')
      continue;
    //check for function declarations
    if (strncmp(p, "function", 8) == 0) {
      process_function_declaration(ml_file, p, c_file);
      continue;
    }
    //sip lines inside functions (indented lines)
    if (*line == '\t')
      continue;
  }
  //generate the main function
  fprintf(c_file, "\nint main(int argc, char *argv[]) {\n");
  fprintf(c_file, "    double result = 0.0;\n");
  //generate code to handle command-line arguments
  fprintf(c_file, "    int arg_count = argc - 1;\n");
  fprintf(c_file, "    if (arg_count > %d) arg_count = %d;\n", MAX_ARGUMENTS,
          MAX_ARGUMENTS);
  fprintf(c_file, "    double arg_values[arg_count];\n");
  fprintf(c_file, "    for (int i = 1; i <= arg_count; i++) {\n");
  fprintf(c_file, "        arg_values[i - 1] = strtod(argv[i], NULL);\n");
  fprintf(c_file, "    }\n");
  //declare arg0, arg1, variables
  for (int i = 0; i < MAX_ARGUMENTS; i++) {
    fprintf(c_file, "    double arg%d = 0.0;\n", i);
  }
  //assign values to arg0, arg1, bassed on arg_count
  fprintf(c_file, "    switch(arg_count) {\n");
  for (int i = MAX_ARGUMENTS; i > 0; i--) {
    fprintf(c_file, "        case %d: arg%d = arg_values[%d];\n", i, i - 1,
            i - 1);
  }
  fprintf(c_file, "        default: ;\n");
  fprintf(c_file, "    }\n");
  //assign initial values to global variables in main
  for (int i = 0; i < assignment_count; i++) {
    fprintf(c_file, "    %s = ", assignments[i].name);
    process_expression(assignments[i].value, c_file, global_variables,
                       &global_variable_count);
    fprintf(c_file, ";\n");
  }
  //declare variables used in main
  Variable main_variables[MAX_IDENTIFIERS];
  int main_variable_count = 0;
  //first pass to collect variables used in main
  line_number = 0;
  rewind(ml_file);
  while (fgets(line, MAX_LINE_LENGTH, ml_file)) {
    line_number++;
    remove_comments(line);
    //skip empty lines
    char *p = line;
    while (*p == ' ' || *p == '\t' || *p == '\n')
      p++;
    if (*p == '\0')
      continue;
    //skip function declarations and their bodies
    if (strncmp(p, "function", 8) == 0) {
      //kip function body
      while (fgets(line, MAX_LINE_LENGTH, ml_file)) {
        line_number++;
        if (*line != '\t') {
          fseek(ml_file, -strlen(line), SEEK_CUR);
          line_number--;
          break;
        }
      }
      continue;
    }
    //kip global assignments (already processed)
    if (strstr(line, "<-")) {
      continue;
    }
    // collect variables
    process_statement(line, NULL, 0, NULL, NULL, main_variables,
                      &main_variable_count);
  }
  //declare variables used in main
  for (int i = 0; i < main_variable_count; i++) {
    if (!variable_exists(main_variables[i].name, global_variables,
                         global_variable_count) &&
        strncmp(main_variables[i].name, "arg", 3) != 0) {
      fprintf(c_file, "    double %s = 0.0;\n", main_variables[i].name);
    }
  }
  //second pass to generate code
  line_number = 0;
  rewind(ml_file);
  while (fgets(line, MAX_LINE_LENGTH, ml_file)) {
    line_number++;
    remove_comments(line);
    //skip empty lines
    char *p = line;
    while (*p == ' ' || *p == '\t' || *p == '\n')
      p++;
    if (*p == '\0')
      continue;
    //skip function declarations and their bodies
    if (strncmp(p, "function", 8) == 0) {
      //skip function body
      while (fgets(line, MAX_LINE_LENGTH, ml_file)) {
        line_number++;
        if (*line != '\t') {
          fseek(ml_file, -strlen(line), SEEK_CUR);
          line_number--;
          break;
        }
      }
      continue;
    }
    //kip global assignments (already processed)
    if (strstr(line, "<-")) {
      continue;
    }
    //generate code
    process_statement(line, c_file, 0, "    ", NULL, main_variables,
                      &main_variable_count);
  }
  fprintf(c_file, "    return 0;\n}\n");
}

void compile_and_run(const char *c_filename, const char *exe_filename, int argc,
                     char *argv[]) {
  char compile_command[512];
  snprintf(compile_command, sizeof(compile_command), "cc -std=c11 -o %s %s",
           exe_filename, c_filename);
  if (system(compile_command) != 0) {
    fprintf(stderr, "! Compilation failed.\n");
    exit(EXIT_FAILURE);
  }
  //build the execute command with arguments
  char execute_command[1024] = "";
  snprintf(execute_command, sizeof(execute_command), "./%s", exe_filename);
  for (int i = 2; i < argc; i++) {
    strcat(execute_command, " ");
    strcat(execute_command, argv[i]);
  }
  // execute the compiled program
  system(execute_command);
  // remove the generated files
  remove(c_filename);
  remove(exe_filename);
}

int main(int argc, char *argv[]) {
  //chck if a file argument is provided
  if (argc < 2) {
    fprintf(stderr, "! Usage: %s <file.ml> [arguments]\n", argv[0]);
    return 1;
  }

  //verify that the file has a .ml extension
  if (!ends_with_ml(argv[1])) {
    fprintf(stderr, "! file does not have .ml extension.\n");
    return 1;
  }

  //attempt to open the provided .ml file in read mode
  FILE *ml_file = fopen(argv[1], "r");
  if (!ml_file) {
    fprintf(stderr, "! File can not be opened %s\n", argv[1]);
    return 1;
  }

  //get the process ID to create unique filenames for C and executable files
  pid_t pid = getpid();
  char c_filename[256];
  char exe_filename[256];
  
  //create a unique temporary C file name based on the process ID
  sprintf(c_filename, "ml-%d.c", pid);
  
  // create a unique executable file name based on the process ID
  sprintf(exe_filename, "ml-%d", pid);

  // open the temporary C file for writing
  FILE *c_file = fopen(c_filename, "w");
  if (!c_file) {
    fprintf(stderr, "! Can not create temporary C file.\n");
    fclose(ml_file);  //close the .ml file before returning
    return 1;
  }

  //transpile the .ml file content to C and write to the temporary C file
  transpile_ml_to_c(ml_file, c_file);

  //close both the input .ml and output C files
  fclose(ml_file);
  fclose(c_file);

  //compile the C file and run the resulting executable
  compile_and_run(c_filename, exe_filename, argc, argv);

  return 0;  //exit the program
}