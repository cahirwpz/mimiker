#include <kbss.h>
#include <stdc.h>
#include <syslimits.h>

static int argc;
static char **argv;

static const char *whitespace = " \t";
static const char *token_separator = " \t\"";
static const char *quot_str = "\"";
static const char quot_char = '"';
/*
  <token>        ::= <token_prefix>[<token_suffix>]
  <token_prefix> ::= any sequence of characters distinct
                   than whitespace or quot_char
  <token_suffix> ::= <quot_char>any sequence of characters distinct
                   than quot_char<quot_char>
  <token_sequence> ::=
  <whitespace>*[<token><whitespace>*(<whitespace><token>(<whitespace>)*)*]
*/

static size_t token_size(const char *input) {
  size_t len = strcspn(input, token_separator);
  if (input[len] == quot_char)
    len += strcspn(input + len + 1, quot_str) + 2;
  return len;
}

static size_t token_no(const char *input) {
  int ntokens = 0;
  for (; *input; input += strspn(input, whitespace)) {
    input += token_size(input);
    ntokens++;
  }
  return ntokens;
}

static char **extract_tokens(char *seq, int * /*out*/ pntokens) {
  seq += strspn(seq, whitespace);

  int ntokens = token_no(seq);
  char **tokens = kbss_grow((ntokens + 1) * sizeof(char *));
  char **ret = tokens;

  for (char *p = seq; *p; p += strspn(p, whitespace)) {
    size_t len = token_size(p);
    *tokens = kbss_grow((len + 1) * sizeof(char));
    strlcpy(*tokens++, p, len + 1);
    p += len;
  }

  if (pntokens)
    *pntokens = ntokens;
  return ret;
}

#if 0
static char **extract_qtd_tokens(char *qseq) {
  size_t len = strcspn(++qseq, quot_str);
  qseq[len] = '\0';
  char **ret = extract_tokens(qseq, NULL);
  qseq[len] = quot_char;
  return ret;
}
#endif

static char *flatten_argv(int _argc, char **_argv) {
  size_t len = 1;
  for (int i = 0; i < _argc; i++)
    len += strlen(_argv[i]) + 1;

  char *args = kbss_grow(len * sizeof(char));
  char *p = args;
  for (int i = 0; i < _argc; i++) {
    p += strlcpy(p, _argv[i], ARG_MAX);
    *p++ = *whitespace;
  }

  return args;
}

void setup_kenv(int _argc, char **_argv, char **_envv) {
  char *args = flatten_argv(_argc, _argv);
  argv = extract_tokens(args, &argc);
}

void print_kenv(void) {
  kprintf("Kernel arguments (%d): ", argc);
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");
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
  /* TODO handle hexadecimal numbers if val string begins with 0x */
  /* TODO check if conversion was successful */
  *val_p = strtoul(val_str, NULL, 10);
  return true;
}

int kenv_get_strv(const char *key, char **strv, size_t len) {
  return 0; /* Not implemented! */
}
