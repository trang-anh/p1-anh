#include <stdio.h>
#include <string.h>  // For strlen

/**
 * Is the given character a special character?
 */
int is_special(char ch) {
  return ch == '<' || ch == '>' || ch == '(' || ch == ')' 
  || ch == ';'; 
}

/**
 * Is the given character a pipe?
 */
int is_pipe(char ch) {
  return ch == '|'; 
}

/**
 * is the given character a quote
 * if something is in a quote, then it is considered one token
 */
int is_quote(char ch) {
  return ch == '"';
}

/**
 * Read from input, extract things inside quotation
 */
int read_quote_string(const char *input, char *output) {
  int i = 0; // index for input
  int cur = 0; // index for output
  // skip opening quote
  if (input[i] == '"') {
    i++; // move past first quote
  }
  
  while (input[i] != '"' && input[i] != '\0') {
    output[cur] = input[i];
    i++;   
    cur++; 
  }
  output[cur] = '\0'; // add the terminating byte to output string

  // Move past the closing quote, if present
  if (input[i] == '"') {
    i++;  // Skip the closing quote
  }

  return i;
  } 
  
  int read_word(const char *input, char *output) {
    int i = 0;
    while (input[i] != '\0' && !is_special(input[i]) && !is_pipe(input[i]) && !is_quote(input[i]) && input[i] != ' ') {
      output[i] = input[i];
      ++i;
    }
    output[i] = '\0';  // Null-terminate the word
    return i;
}



// take a string, return a list (array, linked list) of tokens
int main(int argc, char **argv) {
  // TODO: Implement the tokenize demo.
  // temp buffer
  char buf[256]; // input buffer
  char token[256]; // token buffer
  // getting the input
  fgets(buf, sizeof(buf), stdin); 

  // Remove the trailing newline from fgets, if present
  size_t len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n') {
    buf[len - 1] = '\0';
  }
  
  int i = 0; // current position in string
  while (buf[i] != '\0') { // while the end of string is not reached
    // Skip spaces
    if (buf[i] == ' ') {
      i++;
      continue;
    }

    // first check if the current char is a special character
    if (is_quote(buf[i])) {
      // read from the output AND
      // advance the current position by the length of the string
      i += read_quote_string(&buf[i], token); // read quoted string
      printf("%s\n", token);
    } 

    // handle pipe
    else if (is_pipe(buf[i])) {
      printf("%c\n", buf[i]);  // Print pipe character
      i++;  // Move to next character
    }
    else if (is_special(buf[i])) {
      // print special character in another line
      printf("%c\n", buf[i]);
      i++;
    } 
    // Handle regular words
    else {
      i += read_word(&buf[i], token);  // Read the word
      printf("%s\n", token);  // Print the word as a token
    }
  }
  return 0;
}
