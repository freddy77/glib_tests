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

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/mainloop/timedateex", test_timedateex);

  return g_test_run ();
}
