#include <glib.h>
#include <JavaScriptCore/JavaScript.h>

#include "javascriptcore/javascript.h"

/**
 * This is a native callback that will be exposed
 * as a javascript function. Note that there's no
 * JavaScriptCore types referenced here.
 * It returns an array used by the javascript
 * code executed at the end of the program.
 */
static GJSCValue *
huvsnivs (GJSCObject *function,
	  GJSCObject *this,
	  guint n_args,
	  GList *args)
{
  g_message("Huvsnivs:");
  for (GList *i = args; i != NULL; i = i->next) {
    GJSCValue *arg = (GJSCValue *)i->data;
    g_message("\t%s", jscore_value_as_string(arg));
  }

  return jscore_value_new_from_json("[{\"uri\":\"http://example.com\"}, {\"uri\":\"http://example.org\"}]");
}

/**
 * And this is just another one.
 */
static GJSCValue *
nivsnow (GJSCObject *function,
	 GJSCObject *this,
	 guint n_args,
	 GList *args)
{
  g_message("Nivsnow:");
  for (GList *i = args; i != NULL; i = i->next) {
    GJSCValue *arg = (GJSCValue *)i->data;
    g_message("\t%s", jscore_value_as_string(arg));
  }

  return NULL;
}

gint
main (gint argv, gchar **argc)
{
  // Initializing the library (will be done by WebKit directly)
  jscore_init();

  // Retrieving the main context (already done by WebKit)
  GJSCContext *context = jscore_context_get_default();

  // and the global object
  GJSCObject *obj = jscore_context_get_global_object(context);
  
  // glue API so that we don't need to instantiate a GJSCValue object
  jscore_object_set_property_from_string(obj, "name", "TevOS");

  if (jscore_object_has_property(obj, "name")) {
    // we can also have a glue API here
    GJSCValue *value = jscore_object_get_property(obj, "name");
    gchar *name = jscore_value_as_string(value);

    g_message("Global object is called %s!", name);

    g_free(value);
    g_free(name);
  } else {
    g_message("Ooops");
  }

  // This call creates the javascript functions on obj and associates with our callbacks
  GJSCObject *huvsref = jscore_object_make_function_with_callback(obj, "huvsnivs", huvsnivs);
  GJSCObject *nivsref = jscore_object_make_function_with_callback(obj, "nivsnow", nivsnow);

  // as we associated our callbacks with the global object, they can be called as global functions
  GError *err = NULL;
  jscore_context_evaluate_script(context->instance, "var data = huvsnivs(); data.map(function (i) {nivsnow(i.uri);});", &err);
  if (err != NULL) {
    g_warning("Error executing script: %s", err->message);
    g_error_free(err);
    err = NULL;
  }

  // this script will generate an error
  jscore_context_evaluate_script(context->instance, "return;", &err);
  if (err != NULL) {
    g_warning("Error executing script: %s", err->message);
    g_error_free(err);
    err = NULL;
  }

  g_free(huvsref);
  g_free(nivsref);

  return 0;
}
