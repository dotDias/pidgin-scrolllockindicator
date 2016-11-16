#include <glib.h>
#include <glib/glist.h>

#define PURPLE_PLUGINS

#include "internal.h"

#include "conversation.h"
#include "plugin.h"
#include "pluginpref.h"
#include "prefs.h"
#include "version.h"

#include "win32/win32dep.h"

#include "NTKbdLites.h"

HANDLE hThread = NULL;
HANDLE heventCancel = NULL;
/**
 * @brief Stores the blinking state
 */
int is_blinking = 0;
/**
 * @brief Stores timeout function reference
 */
gint timeout_func_ref = NULL;

#define CANCEL_EVENT_NAME "FlasherCancelEvent"
#define PLUGIN_ID "core-dotdias-scroll_lock_indicator"

#define PACKAGE "scrolllockindicator"
#define GETTEXT_PACKAGE "scrolllockindicator"

static void start_blinking();
static void stop_blinking();

/* Callbacks */

static void
conversation_updated_cb(PurpleConversation* conv, PurpleConvUpdateType type) {
    gint unseen_count;
    if (type != PURPLE_CONV_UPDATE_UNSEEN)
        return;
    unseen_count = (int)purple_conversation_get_data(conv, "unseen-count");
    if (unseen_count > 0 && !is_blinking) {
        start_blinking();
        return;
    }
    if (unseen_count == 0 && is_blinking) {
        stop_blinking();
    }
}

static gboolean
blinking_timeout_cb(gpointer data) {
    g_source_remove(timeout_func_ref);
    stop_blinking();
    return TRUE;
}

/* Helper functions */

static void start_blinking() {
    int timeout;
    timeout = purple_prefs_get_int("/plugins/core/scrolllockindicator/blinking_timeout");
	heventCancel = CreateEvent(NULL, FALSE, FALSE, CANCEL_EVENT_NAME);
    hThread = FlashKeyboardLightInThread(
                purple_prefs_get_int("/plugins/core/scrolllockindicator/blinking_led"),
                purple_prefs_get_int("/plugins/core/scrolllockindicator/blinking_interval"),
                CANCEL_EVENT_NAME);
	is_blinking = 1;
    if (timeout != 0){
        timeout_func_ref = g_timeout_add(timeout, blinking_timeout_cb, NULL);
    }
}

static void stop_blinking() {
    if (hThread == NULL) {
        return;
    }
	SetEvent(heventCancel);
	WaitForSingleObject(hThread, 0);
	CloseHandle(heventCancel);
	hThread = NULL;
	heventCancel = NULL;
	is_blinking = 0;
}

/* Plugin functions */

static gboolean
plugin_load(PurplePlugin *plugin) {
	purple_signal_connect(purple_conversations_get_handle(), "conversation-updated",
		plugin,
        PURPLE_CALLBACK(conversation_updated_cb),
		NULL
	);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	if (is_blinking) {
		stop_blinking();
	}
	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin) {
    PurplePluginPrefFrame *frame;
    PurplePluginPref *ppref;

    frame = purple_plugin_pref_frame_new();
    
    ppref = purple_plugin_pref_new_with_name_and_label(
                "/plugins/core/scrolllockindicator/blinking_led",
                _("Indicator to use"));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "Num Lock", GINT_TO_POINTER(KEYBOARD_NUM_LOCK_ON));
    purple_plugin_pref_add_choice(ppref, "Caps Lock", GINT_TO_POINTER(KEYBOARD_CAPS_LOCK_ON));
    purple_plugin_pref_add_choice(ppref, "Scroll Lock", GINT_TO_POINTER(KEYBOARD_SCROLL_LOCK_ON));
    purple_plugin_pref_frame_add(frame, ppref);

    ppref = purple_plugin_pref_new_with_name_and_label(
                "/plugins/core/scrolllockindicator/blinking_interval",
                _("Blinking interval, ms"));
    purple_plugin_pref_set_bounds(ppref, 0, INT_MAX);
    purple_plugin_pref_frame_add(frame, ppref);

    ppref = purple_plugin_pref_new_with_name_and_label(
                "/plugins/core/scrolllockindicator/blinking_timeout",
                _("Timeout, ms (0 for never)"));
    purple_plugin_pref_set_bounds(ppref, 0, INT_MAX);
    purple_plugin_pref_frame_add(frame, ppref);

    return frame;
}

/* Plugin info */

static PurplePluginUiInfo prefs_info = {
    get_plugin_pref_frame,
    0,   /* page_num (Reserved) */
    NULL, /* frame (Reserved) */
    /* Padding */
    NULL,
    NULL,
    NULL,
    NULL
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,		/* magic			*/
	PURPLE_MAJOR_VERSION,		/* major version	*/
	PURPLE_MINOR_VERSION,		/* minor version	*/
	PURPLE_PLUGIN_STANDARD,		/* type				*/
	NULL,						/* ui_requirement	*/
	0,							/* flags			*/
	NULL,						/* dependencies		*/
	PURPLE_PRIORITY_DEFAULT,	/* priority			*/

	PLUGIN_ID,					/* id				*/
	"Scroll Lock Indicator",	/* name				*/
    "1.2.2",					/* version			*/

	NULL,						/* summary			*/
	NULL,						/* description		*/
	"Dmitry Tulkin <dotdias@gmail.com>",		/* author			*/
	"http://example.com",		/* homepage			*/

	plugin_load,				/* load				*/
	plugin_unload,				/* unload			*/
	NULL,						/* destroy			*/

	NULL,						/* ui_info			*/
	NULL,						/* extra_info		*/
    &prefs_info,				/* prefs_info		*/
	NULL,						/* actions			*/

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin) {
	#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	#endif /* ENABLE_NLS */
	info.summary = _("Notifies about unseen messages via keyboard LEDs");
	info.description = _("Blinks the keyboard LEDs when there are unseen messages.");

	purple_prefs_add_none("/plugins/core/scrolllockindicator");
    purple_prefs_add_int("/plugins/core/scrolllockindicator/blinking_interval", 400);
    purple_prefs_add_int("/plugins/core/scrolllockindicator/blinking_led", KEYBOARD_SCROLL_LOCK_ON);
    purple_prefs_add_int("/plugins/core/scrolllockindicator/blinking_timeout", 0);
}

PURPLE_INIT_PLUGIN(scroll_lock_indicator, init_plugin, info)
