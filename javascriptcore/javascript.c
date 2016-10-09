#include "javascript.h"

/**
 * To be able to use our own callbacks without referencing JavaScriptCore
 * types directly on the client code, I created a JavaScriptCore compatible
 * callback inside the library (internal_native_bridge) that, when called,
 * looks for our newly typed callback (GJSCNativeCallback) on a GTree, indexed
 * by the JSObjectRef of the object built by JSObjectMakeFunctionWithCallback
 * on the jscore_object_make_function_with_callback API. Thus the need of
 * jscore_init, to setup the GTree.
 */
GTree *native_callback_tree = NULL;

gint tree_compare_func(gconstpointer a, gconstpointer b) {
  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

void
jscore_init()
{
  native_callback_tree = g_tree_new(tree_compare_func);
}

static JSValueRef
internal_native_bridge (JSContextRef ctx,
			JSObjectRef function,
			JSObjectRef this,
			long unsigned int n_args,
			const JSValueRef args[],
			JSValueRef *exception)
{
  g_message("[InternalNative] function: %p, args: %d, this: %p", function, n_args);
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

GJSCValue *
jscore_context_evaluate_script(JSContextRef ctx, gchar *script) {
  JSStringRef str_script = JSStringCreateWithUTF8CString(script);
  GJSCValue *result = g_new0(GJSCValue, 1);
  JSValueRef ret = JSEvaluateScript(ctx, str_script, NULL, NULL, 0, NULL);

  result->context = ctx;
  result->instance = ret;

  JSStringRelease(str_script);
  return result;
}

gboolean
jscore_object_has_property(GJSCObject *obj, const gchar *property_name)
{
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);

  gboolean result = JSObjectHasProperty(obj->context, obj->instance, pname);

  JSStringRelease(pname);

  return result;
}

GJSCValue *
jscore_object_get_property(GJSCObject *obj, const gchar *property_name)
{
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
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);

  JSObjectSetProperty(obj->context, obj->instance, pname, property_value->instance,
		      kJSPropertyAttributeReadOnly, NULL);

  JSStringRelease(pname);
}

void
jscore_object_set_property_from_object(GJSCObject *obj, const gchar *property_name, GJSCObject *function) {
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);

  JSObjectSetProperty(obj->context, obj->instance, pname, (JSValueRef)function->instance,
		      kJSPropertyAttributeReadOnly, NULL);

    JSStringRelease(pname);
}

void
jscore_object_set_property_from_string(GJSCObject *obj, const gchar *property_name, gchar *property_value)
{
  JSStringRef pname = JSStringCreateWithUTF8CString(property_name);
  JSStringRef str_pvalue = JSStringCreateWithUTF8CString(property_value);
  JSValueRef pvalue = JSValueMakeString(obj->context, str_pvalue);

  JSObjectSetProperty(obj->context, obj->instance, pname, pvalue, kJSPropertyAttributeReadOnly, NULL);

  JSStringRelease(pname);
  JSStringRelease(str_pvalue);
}

gchar *
jscore_value_as_string(GJSCValue *value)
{
  JSStringRef str_value = JSValueToStringCopy(value->context, value->instance, NULL);
  gchar *buff = (gchar *)g_malloc(256);

  JSStringGetUTF8CString(str_value, buff, 255);
  JSStringRelease(str_value);

  return buff;
}
