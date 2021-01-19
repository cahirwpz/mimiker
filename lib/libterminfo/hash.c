/* DO NOT EDIT
 * Automatically generated from term.h */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <term_private.h>
#include <term.h>

static const char _ti_flagids[][6] = {
	"bw",
	"am",
	"bce",
	"ccc",
	"xhp",
	"xhpa",
	"cpix",
	"crxm",
	"xt",
	"xenl",
	"eo",
	"gn",
	"hc",
	"chts",
	"km",
	"daisy",
	"hs",
	"hls",
	"in",
	"lpix",
	"da",
	"db",
	"mir",
	"msgr",
	"nxon",
	"xsb",
	"npc",
	"ndscr",
	"nrrmc",
	"os",
	"mc5i",
	"xvpa",
	"sam",
	"eslok",
	"hz",
	"ul",
	"xon",
};

__inline
static unsigned int
_ti_flaghash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 20, 35, 5,
       0, 15, 63, 36, 5, 25, 63, 40, 25, 20,
       0, 15, 25, 63, 30, 0, 0, 30, 40, 15,
       0, 63, 40, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
      63, 63, 63, 63, 63, 63
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}
const char *
__unused_flag (register const char *str, register size_t len)
{
  static const char * wordlist[] =
    {
      "", "",
      "xt",
      "xsb",
      "nxon",
      "ndscr",
      "",
      "hs",
      "xhp",
      "xhpa",
      "", "",
      "hc",
      "ccc",
      "chts",
      "", "",
      "os",
      "xon",
      "xenl",
      "eslok",
      "",
      "da",
      "sam",
      "msgr",
      "daisy",
      "",
      "in",
      "npc",
      "mc5i",
      "", "",
      "eo",
      "hls",
      "cpix",
      "nrrmc",
      "",
      "db",
      "gn",
      "crxm",
      "", "",
      "am",
      "bce",
      "xvpa",
      "", "",
      "hz",
      "mir",
      "", "", "",
      "bw",
      "",
      "lpix",
      "", "",
      "ul",
      "", "", "", "",
      "km"
    };
  if (len <= 5 && len >= 2)
    {
      register unsigned int key = _ti_flaghash (str, len);
      if (key <= 62)
        {
          register const char *s = wordlist[key];
          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return s;
        }
    }
  return 0;
}

const char *
_ti_flagid(ssize_t idx)
{

	if ((size_t)idx >= __arraycount(_ti_flagids))
		return NULL;
	return _ti_flagids[idx];
}

ssize_t
_ti_flagindex(const char *key)
{
	uint32_t idx;

	idx = _ti_flaghash((const char *)key, strlen(key));
	if (idx >= __arraycount(_ti_flagids) ||
	    strcmp(key, _ti_flagids[idx]) != 0)
		return -1;
	return idx;
}

static const char _ti_numids[][7] = {
	"bitwin",
	"bitype",
	"bufsz",
	"btns",
	"cols",
	"spinh",
	"spinv",
	"it",
	"lh",
	"lw",
	"lines",
	"lm",
	"ma",
	"xmc",
	"colors",
	"maddr",
	"mjump",
	"pairs",
	"wnum",
	"mcs",
	"mls",
	"ncv",
	"nlab",
	"npins",
	"orc",
	"orl",
	"orhi",
	"orvi",
	"pb",
	"cps",
	"vt",
	"widcs",
	"wsl",
};

__inline
static unsigned int
_ti_numhash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 20, 15, 5,
       5, 56, 5, 56, 30, 0, 0, 56, 0, 25,
       0, 0, 5, 56, 0, 0, 0, 0, 10, 35,
      10, 56, 0, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56
    };
  register unsigned int hval = len;
  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}
const char *
__unused_num (register const char *str, register size_t len)
{
  static const char * wordlist[] =
    {
      "", "",
      "it",
      "orl",
      "",
      "lines",
      "", "",
      "orc",
      "cols",
      "npins",
      "colors",
      "vt",
      "cps",
      "orvi",
      "", "", "",
      "ncv",
      "btns",
      "spinv",
      "bitwin",
      "pb",
      "",
      "nlab",
      "bufsz",
      "bitype",
      "lm",
      "mls",
      "",
      "pairs",
      "",
      "lh",
      "mcs",
      "orhi",
      "mjump",
      "",
      "lw",
      "wsl",
      "wnum",
      "spinh",
      "", "",
      "xmc",
      "",
      "widcs",
      "",
      "ma",
      "", "", "", "", "", "", "",
      "maddr"
    };
  if (len <= 6 && len >= 2)
    {
      register unsigned int key = _ti_numhash (str, len);
      if (key <= 55)
        {
          register const char *s = wordlist[key];
          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return s;
        }
    }
  return 0;
}

const char *
_ti_numid(ssize_t idx)
{

	if ((size_t)idx >= __arraycount(_ti_numids))
		return NULL;
	return _ti_numids[idx];
}

ssize_t
_ti_numindex(const char *key)
{
	uint32_t idx;

	idx = _ti_numhash((const char *)key, strlen(key));
	if (idx >= __arraycount(_ti_numids) ||
	    strcmp(key, _ti_numids[idx]) != 0)
		return -1;
	return idx;
}

static const char _ti_strids[][9] = {
	"acsc",
	"scesa",
	"cbt",
	"bel",
	"bicr",
	"binel",
	"birep",
	"cr",
	"cpi",
	"lpi",
	"chr",
	"cvr",
	"csr",
	"rmp",
	"csnm",
	"tbc",
	"mgc",
	"clear",
	"el1",
	"el",
	"ed",
	"csin",
	"colornm",
	"hpa",
	"cmdch",
	"cwin",
	"cup",
	"cud1",
	"home",
	"civis",
	"cub1",
	"mrcup",
	"cnorm",
	"cuf1",
	"ll",
	"cuu1",
	"cvvis",
	"defbi",
	"defc",
	"dch1",
	"dl1",
	"devt",
	"dial",
	"dsl",
	"dclk",
	"dispc",
	"hd",
	"enacs",
	"endbi",
	"smacs",
	"smam",
	"blink",
	"bold",
	"smcup",
	"smdc",
	"dim",
	"swidm",
	"sdrfq",
	"ehhlm",
	"smir",
	"sitm",
	"elhlm",
	"slm",
	"elohlm",
	"smicm",
	"snlq",
	"snrmq",
	"smpch",
	"prot",
	"rev",
	"erhlm",
	"smsc",
	"invis",
	"sshm",
	"smso",
	"ssubm",
	"ssupm",
	"ethlm",
	"smul",
	"sum",
	"evhlm",
	"smxon",
	"ech",
	"rmacs",
	"rmam",
	"sgr0",
	"rmcup",
	"rmdc",
	"rwidm",
	"rmir",
	"ritm",
	"rlm",
	"rmicm",
	"rmpch",
	"rmsc",
	"rshm",
	"rmso",
	"rsubm",
	"rsupm",
	"rmul",
	"rum",
	"rmxon",
	"pause",
	"hook",
	"flash",
	"ff",
	"fsl",
	"getm",
	"wingo",
	"hup",
	"is1",
	"is2",
	"is3",
	"if",
	"iprog",
	"initc",
	"initp",
	"ich1",
	"il1",
	"ip",
	"ka1",
	"ka3",
	"kb2",
	"kbs",
	"kbeg",
	"kcbt",
	"kc1",
	"kc3",
	"kcan",
	"ktbc",
	"kclr",
	"kclo",
	"kcmd",
	"kcpy",
	"kcrt",
	"kctab",
	"kdch1",
	"kdl1",
	"kcud1",
	"krmir",
	"kend",
	"kent",
	"kel",
	"ked",
	"kext",
	"kf0",
	"kf1",
	"kf2",
	"kf3",
	"kf4",
	"kf5",
	"kf6",
	"kf7",
	"kf8",
	"kf9",
	"kf10",
	"kf11",
	"kf12",
	"kf13",
	"kf14",
	"kf15",
	"kf16",
	"kf17",
	"kf18",
	"kf19",
	"kf20",
	"kf21",
	"kf22",
	"kf23",
	"kf24",
	"kf25",
	"kf26",
	"kf27",
	"kf28",
	"kf29",
	"kf30",
	"kf31",
	"kf32",
	"kf33",
	"kf34",
	"kf35",
	"kf36",
	"kf37",
	"kf38",
	"kf39",
	"kf40",
	"kf41",
	"kf42",
	"kf43",
	"kf44",
	"kf45",
	"kf46",
	"kf47",
	"kf48",
	"kf49",
	"kf50",
	"kf51",
	"kf52",
	"kf53",
	"kf54",
	"kf55",
	"kf56",
	"kf57",
	"kf58",
	"kf59",
	"kf60",
	"kf61",
	"kf62",
	"kf63",
	"kfnd",
	"khlp",
	"khome",
	"kich1",
	"kil1",
	"kcub1",
	"kll",
	"kmrk",
	"kmsg",
	"kmous",
	"kmov",
	"knxt",
	"knp",
	"kopn",
	"kopt",
	"kpp",
	"kprv",
	"kprt",
	"krdo",
	"kref",
	"krfr",
	"krpl",
	"krst",
	"kres",
	"kcuf1",
	"ksav",
	"kBEG",
	"kCAN",
	"kCMD",
	"kCPY",
	"kCRT",
	"kDC",
	"kDL",
	"kslt",
	"kEND",
	"kEOL",
	"kEXT",
	"kind",
	"kFND",
	"kHLP",
	"kHOM",
	"kIC",
	"kLFT",
	"kMSG",
	"kMOV",
	"kNXT",
	"kOPT",
	"kPRV",
	"kPRT",
	"kri",
	"kRDO",
	"kRPL",
	"kRIT",
	"kRES",
	"kSAV",
	"kSPD",
	"khts",
	"kUND",
	"kspd",
	"kund",
	"kcuu1",
	"rmkx",
	"smkx",
	"lf0",
	"lf1",
	"lf2",
	"lf3",
	"lf4",
	"lf5",
	"lf6",
	"lf7",
	"lf8",
	"lf9",
	"lf10",
	"fln",
	"rmln",
	"smln",
	"rmm",
	"smm",
	"mhpa",
	"mcud1",
	"mcub1",
	"mcuf1",
	"mvpa",
	"mcuu1",
	"minfo",
	"nel",
	"porder",
	"oc",
	"op",
	"pad",
	"dch",
	"dl",
	"cud",
	"mcud",
	"ich",
	"indn",
	"il",
	"cub",
	"mcub",
	"cuf",
	"mcuf",
	"rin",
	"cuu",
	"mcuu",
	"pctrm",
	"pfkey",
	"pfloc",
	"pfxl",
	"pfx",
	"pln",
	"mc0",
	"mc5p",
	"mc4",
	"mc5",
	"pulse",
	"qdial",
	"rmclk",
	"rep",
	"rfi",
	"reqmp",
	"rs1",
	"rs2",
	"rs3",
	"rf",
	"rc",
	"vpa",
	"sc",
	"scesc",
	"ind",
	"ri",
	"scs",
	"s0ds",
	"s1ds",
	"s2ds",
	"s3ds",
	"sgr1",
	"setab",
	"setaf",
	"sgr",
	"setb",
	"smgb",
	"smgbp",
	"sclk",
	"setcolor",
	"scp",
	"setf",
	"smgl",
	"smglp",
	"smglr",
	"slines",
	"slength",
	"smgr",
	"smgrp",
	"hts",
	"smgtb",
	"smgt",
	"smgtp",
	"wind",
	"sbim",
	"scsd",
	"rbim",
	"rcsd",
	"subcs",
	"supcs",
	"ht",
	"docr",
	"tsl",
	"tone",
	"u0",
	"u1",
	"u2",
	"u3",
	"u4",
	"u5",
	"u6",
	"u7",
	"u8",
	"u9",
	"uc",
	"hu",
	"wait",
	"xoffc",
	"xonc",
	"zerom",
};

__inline
static unsigned int
_ti_strhash (register const char *str, register size_t len)
{
  static unsigned short asso_values[] =
    {
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 405, 35,
       460, 325, 480, 8, 510, 20, 170, 390, 125, 290,
       295, 10, 270, 115, 80, 70, 1471, 65, 50, 0,
        23, 265, 1471, 205, 10, 0, 115, 35, 65, 3,
       425, 5, 1471, 20, 40, 260, 0, 135, 125, 180,
         0, 5, 1471, 1471, 1471, 1471, 15, 295, 310, 20,
        10, 295, 280, 235, 100, 130, 265, 5, 155, 30,
        20, 100, 120, 35, 5, 0, 0, 210, 165, 400,
        85, 30, 475, 28, 175, 175, 330, 1471, 13, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471, 1471,
      1471, 1471, 1471, 1471
    };
  register unsigned int hval = len;
  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]+8];
      case 2:
        hval += asso_values[(unsigned char)str[1]+1];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}
const char *
__unused_str (register const char *str, register size_t len)
{
  static const char * wordlist[] =
    {
      "", "", "",
      "tsl",
      "", "", "", "",
      "kDC",
      "kslt",
      "", "",
      "sc",
      "dsl",
      "kPRV",
      "", "",
      "rc",
      "kIC",
      "sclk",
      "", "",
      "cr",
      "",
      "kclr",
      "", "", "",
      "tbc",
      "dclk",
      "", "", "", "",
      "krfr",
      "", "",
      "krst",
      "kll",
      "kres",
      "",
      "scs",
      "dl",
      "kri",
      "smln",
      "", "", "",
      "kDL",
      "rmln",
      "", "",
      "scsd",
      "mc5",
      "kPRT",
      "",
      "kbs",
      "rcsd",
      "",
      "kUND",
      "", "", "", "",
      "smir",
      "scesc",
      "",
      "kNXT",
      "",
      "rmir",
      "", "",
      "smsc",
      "",
      "kRIT",
      "", "",
      "rmsc",
      "",
      "csin",
      "", "",
      "kEND",
      "",
      "kRES",
      "", "",
      "kEXT",
      "",
      "sbim",
      "", "", "",
      "cpi",
      "rbim",
      "", "", "",
      "scp",
      "kCRT",
      "", "", "", "",
      "kspd",
      "", "", "", "",
      "smkx",
      "smicm",
      "",
      "oc",
      "rmp",
      "rmkx",
      "rmicm",
      "", "", "",
      "kclo",
      "", "",
      "knxt",
      "",
      "smgt",
      "", "", "",
      "kpp",
      "smgr",
      "", "", "",
      "rs2",
      "kcpy",
      "", "",
      "op",
      "mgc",
      "snlq",
      "", "", "",
      "dch",
      "docr",
      "", "",
      "kRDO",
      "",
      "kLFT",
      "", "",
      "smso",
      "kb2",
      "sshm",
      "", "",
      "rmso",
      "smxon",
      "rshm",
      "", "",
      "il",
      "rmxon",
      "cwin",
      "swidm",
      "",
      "ip",
      "",
      "kcan",
      "rwidm",
      "", "", "",
      "mc5p",
      "smacs",
      "", "", "",
      "dch1",
      "rmacs",
      "", "", "",
      "smam",
      "", "",
      "ll",
      "",
      "rmam",
      "", "", "",
      "knp",
      "s0ds",
      "rmclk",
      "", "", "",
      "smdc",
      "", "", "", "",
      "rmdc",
      "", "", "",
      "cuf",
      "kMOV",
      "", "", "",
      "mc0",
      "kopt",
      "", "", "",
      "cbt",
      "csnm",
      "u4",
      "",
      "uc",
      "",
      "kSPD",
      "", "", "",
      "lpi",
      "mcud",
      "smpch",
      "",
      "u6",
      "smm",
      "kopn",
      "rmpch",
      "", "",
      "rmm",
      "kcmd",
      "kcud1",
      "",
      "rf",
      "slm",
      "cuf1",
      "smgtp",
      "",
      "u0",
      "rlm",
      "krpl",
      "smgrp",
      "",
      "colornm",
      "kf5",
      "kSAV",
      "", "", "",
      "is2",
      "khlp",
      "", "",
      "kf55",
      "ich",
      "krdo",
      "mcud1",
      "",
      "hu",
      "hpa",
      "kRPL",
      "", "",
      "ri",
      "cup",
      "kf57",
      "supcs",
      "", "",
      "rfi",
      "smgl",
      "", "",
      "kEOL",
      "fsl",
      "kcbt",
      "smglr",
      "", "",
      "kel",
      "kf51",
      "initc",
      "",
      "kmsg",
      "",
      "kbeg",
      "", "", "",
      "rs3",
      "ich1",
      "", "",
      "kext",
      "nel",
      "ksav",
      "", "", "",
      "kc3",
      "kil1",
      "", "",
      "ht",
      "kf9",
      "khts",
      "", "", "",
      "pln",
      "kref",
      "cmdch",
      "",
      "slength",
      "kf8",
      "kFND",
      "", "",
      "el",
      "",
      "kCAN",
      "ssupm",
      "", "",
      "vpa",
      "defc",
      "rsupm",
      "",
      "u9",
      "mc4",
      "kdl1",
      "scesa",
      "hts",
      "",
      "cud",
      "kMSG",
      "pctrm",
      "", "", "",
      "kund",
      "", "", "",
      "hup",
      "smul",
      "krmir",
      "",
      "acsc",
      "kf7",
      "rmul",
      "smcup",
      "", "",
      "cuu",
      "kCPY",
      "rmcup",
      "",
      "if",
      "kf2",
      "kCMD",
      "mrcup",
      "pfx",
      "",
      "rep",
      "kBEG",
      "", "",
      "kf25",
      "sum",
      "cud1",
      "", "",
      "u7",
      "rum",
      "kHOM",
      "clear",
      "slines",
      "",
      "ind",
      "kf27",
      "initp",
      "", "", "",
      "xonc",
      "", "",
      "hd",
      "rs1",
      "cuu1",
      "smglp",
      "", "",
      "lf5",
      "kf21",
      "blink",
      "", "",
      "kc1",
      "indn",
      "", "", "",
      "kf0",
      "",
      "kich1",
      "", "", "",
      "kfnd",
      "", "", "",
      "is3",
      "kf58",
      "", "", "",
      "ech",
      "mcuu",
      "", "", "",
      "dl1",
      "smgb",
      "smgtb",
      "", "",
      "rin",
      "",
      "kcuu1",
      "", "",
      "ked",
      "bold",
      "kdch1",
      "", "",
      "dispc",
      "kind",
      "", "", "",
      "cub",
      "kent",
      "subcs",
      "", "", "", "", "", "", "",
      "lf9",
      "kend",
      "mcuu1",
      "", "", "", "",
      "reqmp",
      "", "",
      "lf8",
      "sitm",
      "", "", "",
      "fln",
      "ritm",
      "pfloc",
      "", "", "",
      "s2ds",
      "", "", "",
      "dim",
      "cub1",
      "", "", "", "",
      "kcrt",
      "", "", "",
      "csr",
      "mcuf",
      "", "", "", "",
      "ktbc",
      "", "", "",
      "lf7",
      "kmrk",
      "kcuf1",
      "", "",
      "kf6",
      "", "", "",
      "ff",
      "lf2",
      "kprt",
      "ssubm",
      "", "",
      "is1",
      "prot",
      "rsubm",
      "",
      "pfxl",
      "",
      "mcub",
      "xoffc",
      "", "",
      "kf3",
      "",
      "mcuf1",
      "",
      "u2",
      "kf4",
      "kf28",
      "kcub1",
      "",
      "kf35",
      "",
      "mhpa",
      "flash",
      "",
      "kf45",
      "",
      "kf61",
      "enacs",
      "", "",
      "il1",
      "kf37",
      "smgbp",
      "", "", "",
      "kf47",
      "cnorm",
      "", "",
      "lf0",
      "dial",
      "mcub1",
      "", "", "",
      "kf31",
      "", "", "", "",
      "kf41",
      "", "", "",
      "sgr",
      "kf53",
      "", "", "",
      "setcolor",
      "tone",
      "pulse",
      "", "",
      "pad",
      "bicr",
      "", "",
      "ed",
      "bel",
      "kmov",
      "", "", "", "",
      "kOPT",
      "", "",
      "u8",
      "", "",
      "erhlm",
      "", "",
      "ka3",
      "", "", "", "", "",
      "sgr1",
      "", "", "",
      "rev",
      "s1ds",
      "", "", "", "",
      "devt",
      "", "", "",
      "chr",
      "hook",
      "", "", "",
      "kf1",
      "",
      "elhlm",
      "", "", "",
      "s3ds",
      "kmous",
      "",
      "kf15",
      "",
      "kf59",
      "snrmq",
      "", "", "", "", "", "", "", "",
      "kf17",
      "", "", "", "",
      "kf50",
      "", "", "",
      "lf6",
      "", "", "", "", "",
      "kf11",
      "", "",
      "u1",
      "", "", "", "", "", "",
      "kHLP",
      "", "", "",
      "lf3",
      "kprv",
      "pfkey",
      "", "",
      "lf4",
      "", "", "",
      "u3",
      "",
      "kf23",
      "invis",
      "", "", "", "", "", "", "", "",
      "kf38",
      "", "", "",
      "ka1",
      "kf48",
      "", "", "", "",
      "kf52",
      "", "", "",
      "el1",
      "", "", "",
      "u5",
      "",
      "getm",
      "", "", "", "",
      "home",
      "", "", "", "",
      "kf54",
      "ehhlm",
      "", "", "",
      "setf",
      "", "", "", "", "", "", "", "", "",
      "wait",
      "civis",
      "", "", "", "",
      "defbi",
      "", "", "",
      "kf29",
      "", "", "", "",
      "kf56",
      "", "", "", "",
      "setb",
      "", "", "", "",
      "kf20",
      "", "", "", "", "", "", "", "",
      "lf1",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "",
      "kctab",
      "", "", "",
      "kf18",
      "", "", "", "", "", "", "", "", "",
      "mvpa",
      "ethlm",
      "", "", "", "",
      "qdial",
      "", "", "", "", "", "", "", "",
      "kf22",
      "", "", "", "", "", "", "", "", "",
      "kf63",
      "", "", "", "",
      "wind",
      "minfo",
      "", "", "",
      "kf24",
      "", "", "", "", "", "", "", "", "",
      "kf33",
      "", "", "", "",
      "kf43",
      "khome",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "",
      "kf26",
      "", "", "", "", "",
      "cvvis",
      "", "", "", "", "", "", "", "", "",
      "", "", "",
      "cvr",
      "", "", "", "", "", "",
      "pause",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "",
      "kf60",
      "", "", "", "",
      "kf39",
      "", "", "", "",
      "kf49",
      "", "", "", "", "", "", "", "", "",
      "kf30",
      "", "", "", "",
      "kf40",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "",
      "kf13",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "",
      "kf62",
      "", "", "", "", "",
      "iprog",
      "", "", "",
      "sgr0",
      "",
      "elohlm",
      "", "", "", "", "", "", "",
      "kf32",
      "endbi",
      "", "", "",
      "kf42",
      "", "", "", "", "",
      "evhlm",
      "", "", "", "", "", "", "", "",
      "kf34",
      "", "", "", "",
      "kf44",
      "", "", "", "",
      "kf19",
      "",
      "porder",
      "", "", "", "", "", "", "", "",
      "setaf",
      "", "", "",
      "kf10",
      "", "", "", "",
      "kf36",
      "", "", "", "",
      "kf46",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "",
      "setab",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "",
      "sdrfq",
      "", "", "",
      "kf12",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "",
      "kf14",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "",
      "kf16",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "",
      "wingo",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "lf10",
      "", "", "", "", "",
      "binel",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "",
      "zerom",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "",
      "birep"
    };
  if (len <= 8 && len >= 2)
    {
      register unsigned int key = _ti_strhash (str, len);
      if (key <= 1470)
        {
          register const char *s = wordlist[key];
          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return s;
        }
    }
  return 0;
}

const char *
_ti_strid(ssize_t idx)
{

	if ((size_t)idx >= __arraycount(_ti_strids))
		return NULL;
	return _ti_strids[idx];
}

ssize_t
_ti_strindex(const char *key)
{
	uint32_t idx;

	idx = _ti_strhash((const char *)key, strlen(key));
	if (idx >= __arraycount(_ti_strids) ||
	    strcmp(key, _ti_strids[idx]) != 0)
		return -1;
	return idx;
}
