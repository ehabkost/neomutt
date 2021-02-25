/**
 * @file
 * Index Bar (status)
 *
 * @authors
 * Copyright (C) 2021 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page index_ibar Index Bar (status)
 *
 * Index Bar (status)
 */

#include "config.h"
#include <assert.h>
#include "gui/lib.h"
#include "lib.h"
#include "context.h"
#include "private_data.h"
#include "shared_data.h"
#include "status.h"

static int recalc_count = 0;
static int repaint_count = 0;
static int event_count = 0;

/**
 * struct IBarPrivateData - XXX
 */
struct IBarPrivateData
{
  struct IndexSharedData *shared; ///< Shared Index data
  struct IndexPrivateData *priv;  ///< Private Index data
  const char *display;            ///< Cached display string
};

/**
 * ibar_data_free - Free the private data attached to the MuttWindow - Implements MuttWindow::wdata_free()
 */
static void ibar_data_free(struct MuttWindow *win, void **ptr)
{
  if (!ptr || !*ptr)
    return;

  // struct IBarPrivateData *ibar_data = *ptr;

  FREE(ptr);
}

/**
 * ibar_data_new - Free the private data attached to the MuttWindow
 */
static struct IBarPrivateData *ibar_data_new(struct IndexSharedData *shared,
                                             struct IndexPrivateData *priv)
{
  struct IBarPrivateData *ibar_data = mutt_mem_calloc(1, sizeof(struct IBarPrivateData));

  ibar_data->shared = shared;
  ibar_data->priv = priv;

  return ibar_data;
}

/**
 * ibar_recalc - Recalculate the Window data - Implements MuttWindow::recalc()
 */
static int ibar_recalc(struct MuttWindow *win)
{
  recalc_count++;
  return 0;
}

/**
 * ibar_repaint - Repaint the Window - Implements MuttWindow::repaint()
 */
static int ibar_repaint(struct MuttWindow *win)
{
  repaint_count++;
  if (!mutt_window_is_visible(win))
    return 0;

  char buf[1024] = { 0 };
  mutt_window_move(win, 0, 0);
  mutt_curses_set_color(MT_COLOR_QUOTED);
  int debug_len = snprintf(buf, sizeof(buf), "(E%d,C%d,P%d) ", event_count,
                           recalc_count, repaint_count);
  mutt_window_clrtoeol(win);

  struct IBarPrivateData *ibar_data = win->wdata;
  struct IndexSharedData *shared = ibar_data->shared;
  struct IndexPrivateData *priv = ibar_data->priv;

  const char *c_status_format = cs_subset_string(shared->sub, "status_format");
  menu_status_line(buf + debug_len, sizeof(buf) - debug_len, priv->menu,
                   shared->mailbox, NONULL(c_status_format));
  mutt_window_move(win, 0, 0);
  mutt_curses_set_color(MT_COLOR_STATUS);
  mutt_draw_statusline(win->state.cols, buf, sizeof(buf));
  mutt_curses_set_color(MT_COLOR_NORMAL);

  const bool c_ts_enabled = cs_subset_bool(shared->sub, "ts_enabled");
  if (c_ts_enabled && TsSupported)
  {
    const char *c_ts_status_format =
        cs_subset_string(shared->sub, "ts_status_format");
    menu_status_line(buf, sizeof(buf), priv->menu, shared->mailbox,
                     NONULL(c_ts_status_format));
    mutt_ts_status(buf);
    const char *c_ts_icon_format =
        cs_subset_string(shared->sub, "ts_icon_format");
    menu_status_line(buf, sizeof(buf), priv->menu, shared->mailbox, NONULL(c_ts_icon_format));
    mutt_ts_icon(buf);
  }

  return 0;
}

/**
 * ibar_index_observer - Listen for changes to the Index - Implements ::observer_t
 */
static int ibar_index_observer(struct NotifyCallback *nc)
{
  event_count++;

  if (!nc->global_data)
    return -1;
  if (nc->event_type != NT_INDEX)
    return 0;

  struct MuttWindow *win_ibar = nc->global_data;
  if (!win_ibar)
    return 0;

  struct IBarPrivateData *ibar_data = win_ibar->wdata;

  if (ibar_data->shared)
    ;

  return 0;
}

/**
 * ibar_create - Create the Index Bar (status)
 * @param shared Shared Index data
 * @param priv   Private Index data
 * @retval ptr New Window
 */
struct MuttWindow *ibar_create(struct IndexSharedData *shared, struct IndexPrivateData *priv)
{
  struct MuttWindow *win_ibar =
      mutt_window_new(WT_INDEX_BAR, MUTT_WIN_ORIENT_VERTICAL,
                      MUTT_WIN_SIZE_FIXED, MUTT_WIN_SIZE_UNLIMITED, 1);

  win_ibar->wdata = ibar_data_new(shared, priv);
  win_ibar->wdata_free = ibar_data_free;
  win_ibar->recalc = ibar_recalc;
  win_ibar->repaint = ibar_repaint;

  notify_observer_add(shared->notify, NT_INDEX, ibar_index_observer, win_ibar);

  return win_ibar;
}
