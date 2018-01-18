#include <glib.h>
#include <stdio.h>
#include <string.h>

/* this test came from a timedatex bug, see
 * https://github.com/mlichvar/timedatex/issues/4
 * https://bugzilla.redhat.com/show_bug.cgi?id=1450628
 */
static gboolean
timedateex_set_quit (gpointer user_data)
{
  gboolean *quit = user_data;
  *quit = TRUE;
  return TRUE;
}

static void
test_timedateex (void)
{
  int i;

  for (i = 0; i < 10; ++i) {
    gboolean main_quit = FALSE;
    guint timeout_id = g_timeout_add (30, timedateex_set_quit, &main_quit);
    g_assert (timeout_id != 0);

    // as only the timeout event if set it should be always be
    // triggered not depending on events added or removed
    g_main_context_iteration (g_main_context_default (), TRUE);
    g_assert_cmpint (main_quit, ==, TRUE);

    g_source_remove (timeout_id);
  }
}

/* this test case came from comment 19 on
 * https://bugzilla.gnome.org/show_bug.cgi?id=761102
 *
 *    thread A                 thread B
 *  ------------             ------------
 *  for (;;) {
 *    acquire
 *    prepare
 *    query
 *    release
 *    ppoll()
 *                           g_source_attach
 *    acquire
 *    check
 *    dispatch
 *    release
 *  }
 */
static gboolean
never_reached (gpointer user_data)
{
  g_assert_not_reached ();
  return TRUE;
}

static gpointer
attach_source_delayed (gpointer data)
{
  volatile gint *source_added = (volatile gint *) data;

  g_usleep (50 * 1000);

  guint id = g_timeout_add (50, never_reached, NULL);
  g_atomic_int_inc (source_added);

  return GUINT_TO_POINTER (id);
}

static void
test_attach_not_owned (void)
{
  gint priority, timeout, n_fds;
  GPollFD fds[64];
  volatile gint source_added = 0;

  GMainContext *context = g_main_context_default ();

  guint timeout_id = g_timeout_add (3 * 1000, never_reached, NULL);

  g_main_context_acquire (context);
  g_main_context_prepare (context, &priority);
  n_fds = g_main_context_query (context, priority, &timeout, fds, G_N_ELEMENTS(fds));
  g_main_context_release (context);

  g_assert_cmpint (timeout, >=, 2000);

  // the poll should be woke up by the attach but the attached event
  // should not be ready at the end (unless the wake came from some
  // timeout)
  GThread *th = g_thread_new (NULL, attach_source_delayed, (gpointer) &source_added);
  g_poll (fds, n_fds, timeout);
  // check was waked up by adding, not some pending event
  g_assert_cmpint (g_atomic_int_get (&source_added), ==, 1);
  guint source_id = GPOINTER_TO_UINT (g_thread_join (th));

  g_main_context_acquire (context);
  g_main_context_check (context, priority, fds, n_fds);
  g_main_context_dispatch (context);
  g_main_context_release (context);

  g_source_remove (source_id);
  g_source_remove (timeout_id);
}

/* this test case came from comment 25 on
 * https://bugzilla.gnome.org/show_bug.cgi?id=761102
 *
 *    thread A                 thread B
 *  ------------             ------------
 *  for (;;) {
 *    acquire
 *    prepare
 *    query
 *                           g_source_attach
 *    release
 *    ppoll()
 *    acquire
 *    check
 *    dispatch
 *    release
 *  }
 */
static gpointer
attach_source (gpointer data)
{
  guint id = g_timeout_add (50, never_reached, NULL);

  return GUINT_TO_POINTER (id);
}

static void
test_attach_owned (void)
{
  gint priority, timeout, n_fds;
  GPollFD fds[64];

  GMainContext *context = g_main_context_default ();

  guint timeout_id = g_timeout_add (3 * 1000, never_reached, NULL);

  g_main_context_acquire (context);
  g_main_context_prepare (context, &priority);
  n_fds = g_main_context_query (context, priority, &timeout, fds, G_N_ELEMENTS(fds));
  // we should have not pending events here
  g_assert_cmpint (g_poll (fds, n_fds, 0), ==, 0);
  GThread *th = g_thread_new (NULL, attach_source, NULL);
  guint source_id = GPOINTER_TO_UINT (g_thread_join (th));
  g_main_context_release (context);

  g_assert_cmpint (timeout, >=, 2000);

  // the poll should be woke up by the attach but the attached event
  // should not be ready at the end (unless the wake came from some
  // timeout)
  g_poll (fds, n_fds, timeout);

  g_main_context_acquire (context);
  g_main_context_check (context, priority, fds, n_fds);
  g_main_context_dispatch (context);
  g_main_context_release (context);

  g_source_remove (source_id);
  g_source_remove (timeout_id);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/mainloop/timedateex", test_timedateex);
  g_test_add_func ("/mainloop/attach_not_owned", test_attach_not_owned);
  g_test_add_func ("/mainloop/attach_owned", test_attach_owned);

  return g_test_run ();
}
