#include <kbss.h>
#include <stdc.h>
#include <syslimits.h>

static struct {
  int argc;
  char **argv;
  char **user_argv;
  char **user_envv;
} _kenv;

static const char *whitespace = " \t";
static const char *token_separator = " \t\"";
static const char *quot_str = "\"";
static const char quot_char = '\"';
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

static char **extract_qtd_tokens(char *qseq) {
  size_t len = strcspn(++qseq, quot_str);
  qseq[len] = '\0';
  char **ret = extract_tokens(qseq, NULL);
  qseq[len] = quot_char;
  return ret;
}

char *kenv_get(const char *key) {
  unsigned n = strlen(key);

  for (int i = 1; i < _kenv.argc; i++) {
    char *arg = _kenv.argv[i];
    if ((strncmp(arg, key, n) == 0) && (arg[n] == '='))
      return arg + n + 1;
  }
  return NULL;
}

static char *flatten_argv(int pfm_argc, char **pfm_argv) {
  size_t args_len = 1;
  for (int i = 0; i < pfm_argc; i++)
    args_len += strlen(pfm_argv[i]) + 1;

  char *args_seq = kbss_grow(args_len * sizeof(char));
  char *p = args_seq;
  for (int i = 0; i < pfm_argc; i++) {
    p += strlcpy(p, pfm_argv[i], ARG_MAX);
    *p++ = *whitespace;
  }

  return args_seq;
}

void setup_kenv(int pfm_argc, char **pfm_argv) {
  char *args_seq = flatten_argv(pfm_argc, pfm_argv);
  _kenv.argv = extract_tokens(args_seq, &(_kenv.argc));

  char *p;
  if ((p = kenv_get("init")))
    _kenv.user_argv = extract_qtd_tokens(p);
  _kenv.user_envv =
    (p = kenv_get("envv")) ? extract_qtd_tokens(p) : (char *[]){NULL};
}

char **kenv_get_user_argv(void) {
  return _kenv.user_argv;
}

char **kenv_get_user_envv(void) {
  return _kenv.user_envv;
}
