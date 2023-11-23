#include <sys/select.h>
#include <sys/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <math.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <wchar.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "st.h"
#include "font.h"
#include "win.h"
#include "input.h"

#include "config.h"

/* macros */
#define IS_SET(flag) ((win.mode & (flag)) != 0)
#define TRUERED(x) (((x)&0xff0000) >> 8)
#define TRUEGREEN(x) (((x)&0xff00))
#define TRUEBLUE(x) (((x)&0xff) << 8)

typedef union {
  struct {
    unsigned char r, g, b;
  };
  unsigned int val;
} color_t;

static inline color_t get_color(int i) {
  return (color_t){.val = colors[i]};
}

typedef unsigned int fbval_t;

struct {
  int fd;
  struct fb_var_screeninfo info;
  void *fb; /* mmap()ed FB memory */
  int bpp;  /* bytes per pixel */
  int bpr;  /* bytes per row */
  size_t len;
} fb;

struct {
  int tw, th; /* tty width and height */
  int w, h;   /* window width and height */
  int ch;     /* char height */
  int cw;     /* char width  */
  int mode;   /* window state/mode flags */
  int cursor; /* cursor style */
  int cols, rows;
  font_t *font;
} win;

static int fb_init() {
  if ((fb.fd = open(fb_dev, O_RDWR)) < 0)
    return 0;
  if (ioctl(fb.fd, FBIOGET_VSCREENINFO, &fb.info) < 0)
    return 0;

  fcntl(fb.fd, F_SETFD, fcntl(fb.fd, F_GETFD) | FD_CLOEXEC);

  fb.bpp = (fb.info.bits_per_pixel + 7) >> 3;
  fb.bpr = fb.info.xres * fb.bpp;
  fb.len = fb.bpr * fb.info.yres;
  fb.fb = mmap(NULL, fb.len, PROT_READ | PROT_WRITE, MAP_SHARED, fb.fd, 0);
  if (fb.fb == MAP_FAILED)
    return 0;

  return 1;
}

static inline void *fb_mem(int r) {
  return fb.fb + r * fb.bpr;
}

static void draw_rect(color_t color, int x, int y, int width, int height) {
  for (int i = 0; i < height; i++) {
    fbval_t *row = fb_mem(i + y);
    for (int j = 0; j < width; j++) {
      row[j + x] = color.val;
    }
  }
}

static void clear_rect(int x1, int y1, int x2, int y2) {
  draw_rect(get_color(IS_SET(MODE_REVERSE) ? defaultfg : defaultbg), x1, y1,
            x2 - x1, y2 - y1);
}

static void get_glyph_col(Glyph g, color_t *colfg, color_t *colbg) {
  color_t temp;

  if (IS_TRUECOL(g.fg)) {
    colfg->r = TRUERED(g.fg);
    colfg->g = TRUEGREEN(g.fg);
    colfg->b = TRUEBLUE(g.fg);
  } else
    *colfg = get_color(g.fg);

  if (IS_TRUECOL(g.bg)) {
    colbg->r = TRUERED(g.bg);
    colbg->g = TRUEGREEN(g.bg);
    colbg->b = TRUEBLUE(g.bg);
  } else
    *colbg = get_color(g.bg);

  /* Change basic system colors [0-7] to bright system colors [8-15] */
  if ((g.mode & ATTR_BOLD_FAINT) == ATTR_BOLD && BETWEEN(g.fg, 0, 7))
    *colfg = get_color(g.fg + 8);

  if (IS_SET(MODE_REVERSE)) {
    if (colfg->val == get_color(defaultfg).val) {
      *colfg = get_color(g.bg);
    } else {
      colfg->r = ~colfg->r;
      colfg->g = ~colfg->g;
      colfg->b = ~colfg->b;
    }

    if (colbg->val == get_color(defaultbg).val) {
      *colbg = get_color(g.fg);
    } else {
      colbg->r = ~colbg->r;
      colbg->g = ~colbg->g;
      colbg->b = ~colbg->b;
    }
  }

  if ((g.mode & ATTR_BOLD_FAINT) == ATTR_FAINT) {
    colfg->r /= 2;
    colfg->g /= 2;
    colfg->b /= 2;
  }

  if (g.mode & ATTR_REVERSE) {
    temp = *colfg;
    *colfg = *colbg;
    *colbg = temp;
  }

  if (g.mode & ATTR_BLINK && win.mode & MODE_BLINK)
    *colfg = *colbg;

  if (g.mode & ATTR_INVISIBLE)
    *colfg = *colbg;
}

static void draw_glyph(Glyph base, int x, int y) {
  int charlen = ((base.mode & ATTR_WIDE) ? 2 : 1);
  int winx = borderpx + x * win.cw, winy = borderpx + y * win.ch,
      width = charlen * win.cw;
  Rune rune = base.u;
  color_t colfg, colbg;

  get_glyph_col(base, &colfg, &colbg);

  if (rune > 127)
    rune = '?';

  /* Intelligent cleaning up of the borders. */
  if (x == 0) {
    clear_rect(
      0, (y == 0) ? 0 : winy, borderpx,
      ((winy + win.ch >= borderpx + win.th) ? win.h : (winy + win.ch)));
  }
  if (winx + width >= borderpx + win.tw) {
    clear_rect(
      winx + width, (y == 0) ? 0 : winy, win.w,
      ((winy + win.ch >= borderpx + win.th) ? win.h : (winy + win.ch)));
  }
  if (y == 0)
    clear_rect(winx, 0, winx + width, borderpx);
  if (winy + win.ch >= borderpx + win.th)
    clear_rect(winx, winy + win.ch, winx + width, win.h);

  char *src = font_glyph(win.font, rune);
  for (int j = 0; j < win.ch; j++) {
    fbval_t *row = fb_mem(j + winy);
    for (int i = 0; i < win.cw; i++) {
      int b = i % 8;
      if (b == 0 && i != 0)
        src++;

      if ((1 << b) & *src)
        row[(winx + win.cw - 1 - i)] = colfg.val;
      else
        row[(winx + win.cw - 1 - i)] = colbg.val;
    }
    src++;
  }
}

static int win_init() {
  int col, row;

  win.font = font_load(font_file);
  if (!win.font)
    return 0;

  win.ch = win.font->fontheight;
  win.cw = win.font->fontwidth;
  win.w = fb.info.xres;
  win.h = fb.info.yres;

  col = (win.w - 2 * borderpx) / win.cw;
  row = (win.h - 2 * borderpx) / win.ch;
  col = MAX(1, col);
  row = MAX(1, row);

  win.cols = col;
  win.rows = row;
  win.tw = col * win.cw;
  win.th = row * win.ch;

  clear_rect(0, 0, win.w, win.h);

  ttyresize(win.tw, win.th);

  win.mode |= MODE_FOCUSED;

  return 1;
}

void xbell(void) {
}

void xclipcopy(void) {
}

void xdrawcursor(int cx, int cy, Glyph g, int ox, int oy, Glyph og) {
  return;
  color_t drawcol;

  /* remove the old cursor */
  if (selected(ox, oy))
    og.mode ^= ATTR_REVERSE;
  draw_glyph(og, ox, oy);

  if (IS_SET(MODE_HIDE))
    return;

  /*
   * Select the right color for the right mode.
   */
  g.mode &= ATTR_BOLD | ATTR_ITALIC | ATTR_UNDERLINE | ATTR_STRUCK | ATTR_WIDE;

  if (IS_SET(MODE_REVERSE)) {
    g.mode |= ATTR_REVERSE;
    g.bg = defaultfg;
    if (selected(cx, cy)) {
      drawcol = get_color(defaultcs);
      g.fg = defaultrcs;
    } else {
      drawcol = get_color(defaultrcs);
      g.fg = defaultcs;
    }
  } else {
    if (selected(cx, cy)) {
      g.fg = defaultfg;
      g.bg = defaultrcs;
    } else {
      g.fg = defaultbg;
      g.bg = defaultcs;
    }
    drawcol = get_color(g.bg);
  }

  /* draw the new one */
  if (IS_SET(MODE_FOCUSED)) {
    switch (win.cursor) {
      case 7:         /* st extension */
        g.u = 0x2603; /* snowman (U+2603) */
                      /* FALLTHROUGH */
      case 0:         /* Blinking Block */
      case 1:         /* Blinking Block (Default) */
      case 2:         /* Steady Block */
        draw_glyph(g, cx, cy);
        break;
      case 3: /* Blinking Underline */
      case 4: /* Steady Underline */
        draw_rect(drawcol, borderpx + cx * win.cw,
                  borderpx + (cy + 1) * win.ch - cursorthickness, win.cw,
                  cursorthickness);
        break;
      case 5: /* Blinking bar */
      case 6: /* Steady bar */
        draw_rect(drawcol, borderpx + cx * win.cw, borderpx + cy * win.ch,
                  cursorthickness, win.ch);
        break;
    }
  } else {
    draw_rect(drawcol, borderpx + cx * win.cw, borderpx + cy * win.ch,
              win.cw - 1, 1);
    draw_rect(drawcol, borderpx + cx * win.cw, borderpx + cy * win.ch, 1,
              win.ch - 1);
    draw_rect(drawcol, borderpx + (cx + 1) * win.cw - 1, borderpx + cy * win.ch,
              1, win.ch - 1);
    draw_rect(drawcol, borderpx + cx * win.cw, borderpx + (cy + 1) * win.ch - 1,
              win.cw, 1);
  }
}

void xdrawline(Line line, int x1, int y1, int x2) {
  return;
  for (int x = x1; x < x2; x++) {
    draw_glyph(line[x], x, y1);
  }
}

void xfinishdraw(void) {
}

void xloadcols(void) {
}

int xsetcolorname(int x, const char *name) {
  return 0;
}

int xgetcolor(int x, unsigned char *r, unsigned char *g, unsigned char *b) {

  return 0;
}

void xseticontitle(char *p) {
}

void xsettitle(char *p) {
}

int xsetcursor(int cursor) {
  if (!BETWEEN(cursor, 0, 7)) /* 7: st extension */
    return 1;
  win.cursor = cursor;
  return 0;
}

void xsetmode(int set, unsigned int flags) {
  int mode = win.mode;
  MODBIT(win.mode, set, flags);
  if ((win.mode & MODE_REVERSE) != (mode & MODE_REVERSE))
    redraw();
}

void xsetpointermotion(int set) {
}

void xsetsel(char *str) {
}

int xstartdraw(void) {
  return 1;
}

void xximspot(int x, int y) {
}

static void run() {
  fd_set rfd;
  int ttyfd, drawing, inputfd;
  struct timespec seltv, *tv, now, lastblink, trigger;
  double timeout;

  ttyfd = ttynew(NULL, shell, NULL, NULL);
  inputfd = input_getfd();

  for (timeout = -1, drawing = 0, lastblink = (struct timespec){0};;) {
    FD_ZERO(&rfd);
    FD_SET(ttyfd, &rfd);
    FD_SET(inputfd, &rfd);

    seltv.tv_sec = timeout / 1E3;
    seltv.tv_nsec = 1E6 * (timeout - 1E3 * seltv.tv_sec);
    tv = timeout >= 0 ? &seltv : NULL;

    if (pselect(MAX(inputfd, ttyfd) + 1, &rfd, NULL, NULL, tv, NULL) < 0) {
      if (errno == EINTR)
        continue;
      die("select failed: %s\n", strerror(errno));
    }
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (FD_ISSET(ttyfd, &rfd))
      ttyread();

    if (FD_ISSET(inputfd, &rfd))
      input_read();

    /*
     * To reduce flicker and tearing, when new content or event
     * triggers drawing, we first wait a bit to ensure we got
     * everything, and if nothing new arrives - we draw.
     * We start with trying to wait minlatency ms. If more content
     * arrives sooner, we retry with shorter and shorter periods,
     * and eventually draw even without idle after maxlatency ms.
     * Typically this results in low latency while interacting,
     * maximum latency intervals during `cat huge.txt`, and perfect
     * sync with periodic updates from animations/key-repeats/etc.
     */
    if (FD_ISSET(ttyfd, &rfd) || FD_ISSET(inputfd, &rfd)) {
      if (!drawing) {
        trigger = now;
        drawing = 1;
      }
      timeout = (maxlatency - TIMEDIFF(now, trigger)) / maxlatency * minlatency;
      if (timeout > 0)
        continue; /* we have time, try to find idle */
    }

    /* idle detected or maxlatency exhausted -> draw */

    timeout = -1;

    if (blinktimeout && tattrset(ATTR_BLINK)) {
      timeout = blinktimeout - TIMEDIFF(now, lastblink);
      if (timeout <= 0) {
        if (-timeout > blinktimeout)
          win.mode |= MODE_BLINK;
        win.mode ^= MODE_BLINK;
        tsetdirtattr(ATTR_BLINK);
        lastblink = now;
        timeout = blinktimeout;
      }
    }

    // draw();
    drawing = 0;
  }
}

int main(int argc, char *argv[]) {
  if (!fb_init())
    die("frame buffer init error\n");

  if (!input_init(ev_dev))
    die("input init error\n");

  if (!win_init())
    die("window init error\n");

  tnew(win.cols, win.rows);
  selinit();
  run();

  return 0;
}