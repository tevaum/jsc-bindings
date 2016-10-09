#include <glib.h>
#include <JavaScriptCore/JavaScript.h>

#include "javascriptcore/javascript.h"

/**
 * This is a native callback that will be exposed
 * as a javascript function.
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
}

gint
main (gint argv, gchar **argc)
{
  // Initializing the library (will be done by WebKit directly)
  jscore_init();

  // Retrieving the main context (already done by WebKit)
  JSGlobalContextRef context = NULL;
  context = JSGlobalContextCreate(NULL);
  JSObjectRef window = JSContextGetGlobalObject(context);

  if (window) {
    g_info("theres a global object");

    // Start using our currently pseudo-GObjects and the API bindings
    GJSCObject *obj = g_new0(GJSCObject, 1);
    obj->context = context;
    obj->instance = window;

    // glue API so that we don't need to instantiate a GJSCValue object
    jscore_object_set_property_from_string(obj, "name", "TevOS");

    if (jscore_object_has_property(obj, "name")) {
      // we can also have a glue API here
      GJSCValue *value = jscore_object_get_property(obj, "name");
      gchar *name = jscore_value_as_string(value);

      g_info("Global object is called %s!", name);

      g_free(value);
      g_free(name);
    }

    // This call creates the javascript functions on obj and associates with our callbacks
    GJSCObject *huvsref = jscore_object_make_function_with_callback(obj, "huvsnivs", huvsnivs);
    GJSCObject *nivsref = jscore_object_make_function_with_callback(obj, "nivsnow", nivsnow);

    // as we associated our callbacks with the global object, they can be called as global functions
    jscore_context_evaluate_script(context, "huvsnivs(); nivsnow(name, name, name, name);");

    g_free(huvsref);
    g_free(nivsref);
  }
  return 0;
}
