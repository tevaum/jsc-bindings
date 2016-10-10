#ifndef _GOBJECT_JAVASCRIPTCORE_
#define _GOBJECT_JAVASCRIPTCORE_

#include <glib.h>
#include <JavaScriptCore/JavaScript.h>

typedef struct _GJSCContext {
  JSContextRef instance;
} GJSCContext;

typedef struct _GJSCObject {
  JSContextRef context;
  JSObjectRef instance;
} GJSCObject;

typedef struct _GJSCValue {
  JSContextRef context;
  JSValueRef instance;
} GJSCValue;

typedef GJSCValue *(*GJSCNativeCallback) (GJSCObject *function, GJSCObject *this, guint n_args, GList *args);

void
jscore_init();

GJSCValue *
jscore_value_new_from_json(gchar *json);

gchar *
jscore_value_as_string(GJSCValue *value);

//JSContextRef
GJSCContext*
jscore_context_get_default();

//JSObjectRef
GJSCObject*
jscore_context_get_global_object(GJSCContext *ctx);
//jscore_context_get_global_object(JSContextRef ctx);

GJSCValue *jscore_context_evaluate_script(GJSCContext *ctx, gchar *script, GError **error);

GJSCObject *
jscore_object_make_function_with_callback(GJSCObject *obj,
					  const gchar *name,
					  GJSCNativeCallback callback);

gboolean jscore_object_has_property(GJSCObject *obj, const gchar *property_name);
GJSCValue *jscore_object_get_property(GJSCObject *obj, const gchar *property_name);
void jscore_object_set_property(GJSCObject *obj, const gchar *property_name, GJSCValue *property_value);

void jscore_object_set_property_from_object(GJSCObject *obj,
					    const gchar *property_name,
					    GJSCObject *property_value);

void jscore_object_set_property_from_string(GJSCObject *obj,
					    const gchar *property_name,
					    gchar *property_value);

#endif
