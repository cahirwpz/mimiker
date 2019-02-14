#include <kbss.h>
#include <stdc.h>
#include <syslimits.h>

static int argc;
static char **argv;

static const char *WHITESPACES = " \t";
static const char *TOKEN_SEPARATORS = " \t\"";
static const char *QUOT_STR = "\"";
static const char QUOT_CHAR = '"';

/* <token>               ::= <key>=<value>
   <value>               ::= <identifier> | "<identifier><identifier_sequence>"
   <key>                 ::= <identifier>
   <identifier_sequence> ::= (<whitespaces><identifier>)*
   <identifier>          ::= any sequence of printable non-whitespace
                             non-" characters
*/

#define skip_spaces(val) val += strspn(val, WHITESPACES)
#define identifier_size(val) strcspn(val, TOKEN_SEPARATORS)

static size_t token_size(const char *input) {
  size_t len = identifier_size(input);
  if (input[len] == QUOT_CHAR)
    len += strcspn(input + len + 1, QUOT_STR) + 2;
  return len;
}

static size_t token_no(const char *input) {
  int ntokens = 0;
  for (; *input; skip_spaces(input)) {
    input += token_size(input);
    ntokens++;
  }
  return ntokens;
}

static char *flatten_argv(int _argc, char **_argv) {
  size_t len = 1;
  for (int i = 0; i < _argc; i++)
    len += strlen(_argv[i]) + 1;

  char *args = kbss_grow(len * sizeof(char));
  char *p = args;
  for (int i = 0; i < _argc; i++) {
    p += strlcpy(p, _argv[i], ARG_MAX);
    *p++ = *WHITESPACES;
  }

  return args;
}

static char **extract_tokens(char *seq, int * /*out*/ pntokens) {
  skip_spaces(seq);

  int ntokens = token_no(seq);
  char **tokens = kbss_grow((ntokens + 1) * sizeof(char *));
  char **ret = tokens;

  for (char *p = seq; *p; skip_spaces(p)) {
    size_t len = token_size(p);
    *tokens = p;
    tokens++;
    p[len] = '\0';
    p += len + 1;
  }

  if (pntokens)
    *pntokens = ntokens;
  return ret;
}

char *kenv_get(const char *key) {
  unsigned n = strlen(key);

  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];
    if ((strncmp(arg, key, n) == 0) && (arg[n] == '='))
      return arg + n + 1;
  }
  return NULL;
}

bool kenv_get_int(const char *key, int *val_p) {
  const char *val_str = kenv_get(key);
  if (!val_str)
    return false;

  skip_spaces(val_str);
  size_t len = identifier_size(val_str);
  unsigned long uval = strntoul(val_str, len, NULL, 0);
  *val_p = uval;
  return true;
}

int kenv_get_strv(const char *key, char **strv, size_t len) {
  char *val = kenv_get(key);
  size_t i = 0;
  char *arg;
  size_t arglen;

  if ((!val) || (len == 0))
    goto end;
  if (*val == QUOT_CHAR)
    val++;

  while ((i < len - 1) && (*val != '\0') && (*val != QUOT_CHAR)) {
    skip_spaces(val);
    arglen = identifier_size(val);
    arg = kbss_grow((arglen + 1) * sizeof(char));
    strlcpy(arg, val, arglen + 1);
    strv[i++] = arg;
    val += arglen;
  }

end:
  strv[i] = NULL;
  return i;
}

void setup_kenv(int _argc, char **_argv, char **_envv) {
  char *args = flatten_argv(_argc, _argv);
  argv = extract_tokens(args, &argc);
  /*TODO: implement environment handling.*/
}

void print_kenv(void) {
  kprintf("Kernel arguments (%d): ", argc);
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");
}
