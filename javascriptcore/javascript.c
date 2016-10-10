#include "javascript.h"

#define JSCORE_ERROR jscore_error_quark ()

static GJSCObject *jscore_object_new_for(JSObjectRef);

enum JSCoreError {
  JSCORE_ERROR_INVALIDSYNTAX
};

GQuark
jscore_error_quark (void)
{
  return g_quark_from_static_string("jscore-error-quark");
}

/**
 * To be able to use our own callbacks without referencing JavaScriptCore
 * types directly on the client code, I created a JavaScriptCore compatible
 * callback inside the library (internal_native_bridge) that, when called,
 * looks for our newly typed callback (GJSCNativeCallback) on a GTree, indexed
 * by the JSObjectRef of the object built by JSObjectMakeFunctionWithCallback
 * on the jscore_object_make_function_with_callback API. Thus the need of
 * jscore_init, to setup the GTree.
 */
static GTree *native_callback_tree = NULL;
static GJSCContext *global_context = NULL;
static GJSCObject *global_object = NULL;

static gint
tree_compare_func(gconstpointer a, gconstpointer b)
{
  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

static JSValueRef
internal_native_bridge (JSContextRef ctx,
			JSObjectRef function,
			JSObjectRef this,
			long unsigned int n_args,
			const JSValueRef args[],
			JSValueRef *exception)
{
  //g_message("[InternalNative] function: %p, args: %d, this: %p", function, n_args);
  GJSCNativeCallback callback = g_tree_lookup(native_callback_tree, (gconstpointer) function);

  if (!callback) {
    g_warning("Callback not found!");
    return NULL;
  }

  /* g_message("Callback address %p", callback); */
  GJSCObject func;
  GJSCObject self;
  GList *arglist = NULL;

  //Initializing objects - dynamic allocation here causes segfault
  func.context = ctx;
  func.instance = function;

  self.context = ctx;
  self.instance = this;

  //handling parameters
  for (int i = 0; i < n_args; i++) {
    JSValueRef arg = args[i];
    GJSCValue *val = g_new0(GJSCValue, 1); //but don't cause segfaults here... :P
    val->context = ctx;
    val->instance = arg;

    arglist = g_list_append(arglist, val);
  }

  GJSCValue *res = callback(&func, &self, n_args, arglist);
  if (res)
    return res->instance;
  return NULL;
}

void
jscore_init()
{
  native_callback_tree = g_tree_new(tree_compare_func);
}

GJSCContext *
jscore_context_get_default()
{
  if (global_context == NULL) {
    global_context = g_new0(GJSCContext, 1);
    global_context->instance = JSGlobalContextCreate(NULL);
  }
  return global_context;
}

GJSCObject*
jscore_context_get_global_object(GJSCContext *ctx)
{
  if (global_object == NULL) {
    global_object = g_new0(GJSCObject, 1);
    global_object->context = ctx->instance;
    global_object->instance = JSContextGetGlobalObject(ctx->instance);
  }
  return global_object;
}

GJSCValue *
jscore_context_evaluate_script(JSContextRef ctx, gchar *script, GError **error) {
  JSStringRef str_script = JSStringCreateWithUTF8CString(script);
  GJSCValue *result = g_new0(GJSCValue, 1);
  JSValueRef err = NULL;
  JSValueRef ret = JSEvaluateScript(ctx, str_script, NULL, NULL, 0, &err);

  if (err != NULL) {
    GJSCObject *gerr = jscore_object_new_for(JSValueToObject(ctx, err, NULL));
    GJSCValue *gmessage = jscore_object_get_property(gerr, "message");
    gchar *message = jscore_value_as_string(gmessage);

    g_set_error(error, JSCORE_ERROR, JSCORE_ERROR_INVALIDSYNTAX,
		"Syntax error: %s", message);
    return NULL;
  }

  result->context = ctx;
  result->instance = ret;

  JSStringRelease(str_script);
  return result;
}

static GJSCObject *
jscore_object_new_for(JSObjectRef obj) {
  GJSCObject *result = g_new0(GJSCObject, 1);
  result->context = global_context->instance;
  result->instance = obj;
  return result;
}

gboolean
jscore_object_has_property(GJSCObject *obj, const gchar *property_name)
{
  //g_message("Checking for property %s on object %p at context %p", property_name, obj->instance, obj->context);
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);

  gboolean result = JSObjectHasProperty(obj->context, obj->instance, pname);

  JSStringRelease(pname);

  return result;
}

GJSCValue *
jscore_object_get_property(GJSCObject *obj, const gchar *property_name)
{
  //g_message("Getting property %s on object %p at context %p", property_name, obj->instance, obj->context);
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);
  GJSCValue *result = g_new0(GJSCValue, 1);

  JSValueRef value = JSObjectGetProperty(obj->context, obj->instance, pname, NULL);

  result->context = obj->context;
  result->instance = value;

  JSStringRelease(pname);
  return result;
}

GJSCObject *
jscore_object_make_function_with_callback(GJSCObject *obj,
					      const gchar *name,
					      GJSCNativeCallback callback)
{
  GJSCObject *result = g_new0(GJSCObject, 1);

  JSObjectRef func = JSObjectMakeFunctionWithCallback(obj->context, NULL, internal_native_bridge);

  result->context = obj->context;
  result->instance = func;

  jscore_object_set_property_from_object(obj, name, result);

  g_tree_insert(native_callback_tree, (gpointer) func, (gpointer) callback);
  return result;
}

void
jscore_object_set_property(GJSCObject *obj, const gchar *property_name, GJSCValue *property_value)
{
  //g_message("Setting property %s on object %p at context %p", property_name, obj->instance, obj->context);
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);

  JSObjectSetProperty(obj->context, obj->instance, pname, property_value->instance,
		      kJSPropertyAttributeReadOnly, NULL);

  JSStringRelease(pname);
}

void
jscore_object_set_property_from_object(GJSCObject *obj, const gchar *property_name, GJSCObject *function) {
  //g_message("Setting property %s on object %p at context %p", property_name, obj->instance, obj->context);
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);

  JSObjectSetProperty(obj->context, obj->instance, pname, (JSValueRef)function->instance,
		      kJSPropertyAttributeReadOnly, NULL);

    JSStringRelease(pname);
}

void
jscore_object_set_property_from_string(GJSCObject *obj, const gchar *property_name, gchar *property_value)
{
  //g_message("Setting property %s on object %p at context %p", property_name, obj->instance, obj->context);
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);
  JSStringRef str_pvalue = JSStringCreateWithUTF8CString(property_value);
  JSValueRef pvalue = JSValueMakeString(obj->context, str_pvalue);

  JSObjectSetProperty(obj->context, obj->instance, pname, pvalue, kJSPropertyAttributeReadOnly, NULL);

  JSStringRelease(pname);
  JSStringRelease(str_pvalue);
}

GJSCValue *
jscore_value_new_from_json(gchar *json)
{
  JSStringRef str_json = JSStringCreateWithUTF8CString(json);
  JSValueRef value = JSValueMakeFromJSONString(global_context->instance, str_json);
  GJSCValue *result = g_new0(GJSCValue, 1);
  result->context = global_context->instance;
  result->instance = value;

  return result;
}

gchar *
jscore_value_as_string(GJSCValue *value)
{
  JSStringRef str_value = JSValueToStringCopy(value->context, value->instance, NULL);
  gchar buff[256];

  JSStringGetUTF8CString(str_value, buff, 255);
  JSStringRelease(str_value);

  return g_strdup(buff);
}
