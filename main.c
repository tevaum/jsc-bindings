#include <glib.h>
#include <JavaScriptCore/JavaScript.h>

#include "javascriptcore/javascript.h"

static JSValueRef
native_bridge (JSContextRef ctx,
	       JSObjectRef function,
	       JSObjectRef this,
	       size_t n_args,
	       const JSValueRef args[],
	       JSValueRef *exception)
{
  g_message("[Native] function: %p, args: %d, this: %p", function, n_args, this);
}

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
  jscore_init();
  
  JSGlobalContextRef context = NULL;
  context = JSGlobalContextCreate(NULL);
  JSObjectRef window = JSContextGetGlobalObject(context);

  if (window) {
    g_info("theres a global object");

    GJSCObject *obj = g_new0(GJSCObject, 1);
    obj->context = context;
    obj->instance = window;

    jscore_object_set_property_from_string(obj, "name", "TevOS");

    if (jscore_object_has_property(obj, "name")) {
      GJSCValue *value = jscore_object_get_property(obj, "name");
      gchar *name = jscore_value_as_string(value);
      //gchar *name = "Huvsnivs";

      g_info("Global object is called %s!", name);

      g_free(value);
      g_free(name);
    }
    //JSObjectSetProperty(context, window,

    jscore_object_make_function_with_callback_new(obj, "huvsnivs", huvsnivs);
    jscore_object_make_function_with_callback_new(obj, "nivsnow", nivsnow);

    jscore_context_evaluate_script(context, "huvsnivs();nivsnow(name, name, name, name);");
  }
  return 0;
}
