/*
 * Copyright 2021 Davidson Francis <davidsondfgl@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MACROS_H
#define MACROS_H

	#include <stdint.h>

	/**
	 * 'Convert' size_t to the nearest JS type.
	 */
	#if SIZE_MAX == 0xffffffffffffffff
		#define SIZE_T uint64
		#define napi_create_sizet napi_create_uint64
		#define napi_get_value_uint64(e,v,r) \
			napi_get_value_bigint_uint64((e),(v),(r),NULL)
		#define napi_create_uint64(e,v,r) \
			napi_create_bigint_uint64((e),(v),(r))
	#else
		#define SIZE_T uint32
		#define napi_create_sizet napi_create_uint32
	#endif

	/* Facility to declare a napi_method. */
	#define DECLARE_NAPI_METHOD(name, func)\
		{name, 0, func, 0, 0, 0, napi_default, 0}

	/* Facility to declare a napi field/variable. */
	#define DECLARE_NAPI_FIELD(name)\
		{#name, 0, 0, get_##name, set_##name, 0, napi_default, 0}

	/**
	 * For a given structure @p str with type @p type and with the
	 * name @p field, define a getter method to it.
	 *
	 * This macro is generic and should work for any primitive type
	 * that is supported by C and JS.
	 */
	#define DEFINE_GET(str, field, type)\
		napi_value get_##field(napi_env env, napi_callback_info info)\
		{\
			struct str *st;      \
			napi_status status;  \
			napi_value jsthis;   \
			napi_value value;    \
			status = napi_get_cb_info(env, info, NULL, NULL, &jsthis, NULL);\
			if (status != napi_ok)\
				return (NULL);\
			status = napi_unwrap(env, jsthis, (void**)&st);\
			if (status != napi_ok)\
				return (NULL);\
			status = napi_create_##type(env, st->field, &value);\
			if (status != napi_ok)\
				return (NULL);\
			return (value);\
		}

	/**
	 * For a given structure @p str with type @p type and with the
	 * name @p field, define a setter method to it.
	 *
	 * This macro is generic and should work for any primitive type
	 * that is supported by C and JS.
	 */
	#define DEFINE_SET(str, field, type)\
		napi_value set_##field(napi_env env, napi_callback_info info)\
		{\
			struct str *st;     \
			napi_status status; \
			napi_value value;   \
			napi_value jsthis;  \
			size_t argc = 1;    \
			status = napi_get_cb_info(env, info, &argc, &value, &jsthis, NULL);\
			if (status != napi_ok) \
				return (NULL);     \
			status = napi_unwrap(env, jsthis, (void**)&st);\
			if (status != napi_ok) \
				return (NULL);     \
			status = napi_get_value_##type(env, value, &st->field); \
			if (status != napi_ok) \
				return (NULL);     \
			return (NULL);         \
		}

	/**
	 * For a given structure @p str with type @p type and with the
	 * name @p field, define a getter and setter method to it.
	 *
	 * This macro is generic and should work for any primitive type
	 * that is supported by C and JS.
	 */
	#define DEFINE_GETTER_AND_SETTER(str, field, type)\
		DEFINE_GET(str, field, type)\
		DEFINE_SET(str, field, type)

	/**
	 * Since every structure/wrapper allocates C memory, this memory
	 * should be release when out of context. Thankfully JS provides
	 * us a way to do that by providing a finish method.
	 */
	#define DEFINE_FINISH_STRUCT(str)\
		void str##_finish(napi_env env, void *finalize_data,\
			void *finalize_hint) {  \
			((void)finalize_hint);  \
			free(finalize_data);    \
		}

	/** 
	 * Define a tag for a given structure.
	 *
	 * Tag is a nice way to identify wrapped objects and ensure that
	 * we're unwrapping the right JS objects.

	 */
	#define DEFINE_TAG(str, tag_lower, tag_upper)\
		static const napi_type_tag str##_tag = {\
  			tag_lower, tag_upper \
		};

	/**
	 * For a given structure @p str and a list of variadic arguments,
	 * defines a structure wrapper for a JS object.
	 *
	 * This is the 'constructor' (actually just a function) for every
	 * structure that wants to be wrapped by JS.
	 */
	#define DEFINE_STRUCT(str, ...)\
		DEFINE_FINISH_STRUCT(str) \
		static napi_value str(napi_env env, napi_callback_info info)\
		{\
			struct str *st;     \
			napi_status status; \
			napi_value obj;     \
	                                       \
			st = calloc(1, sizeof(*st));   \
			if (!st)                       \
				return (NULL);             \
	                                       \
			/* Create 'object'. */         \
			status = napi_create_object(env, &obj); \
			if (status != napi_ok)  \
				return (NULL);      \
									\
			/* Associates a tag with the object. */ \
			status = napi_type_tag_object(env, obj, \
				& str##_tag);       \
			if (status != napi_ok)  \
				return (NULL);      \
	                                \
			/* Set variables. */    \
			napi_property_descriptor properties[] = \
				__VA_ARGS__;          \
	                                  \
			/* Define properties. */  \
			status = napi_define_properties(env, obj,  \
				sizeof(properties)/sizeof(properties[0]), properties); \
			                          \
			if (status != napi_ok)    \
				return (NULL);        \
	                                  \
			/* Wrap everything. */    \
			status = napi_wrap(env, obj, st, str##_finish, NULL, NULL); \
			if (status != napi_ok) \
				return (NULL);     \
			return (obj);          \
		}

	/**
	 * This is a generic wrapper for void functions that return a simple
	 * primitive type, compatible with C and JS.
	 */
	#define DEFINE_SIMPLE_WRAPPER_RET_VOID(name, js_ret, c_ret)\
		static napi_value wrap_##name(napi_env env, napi_callback_info info)\
		{\
			napi_value napi_ret; /* JS return value. */   \
			napi_status status;  /* JS status code.  */   \
			c_ret func_ret;      /* C return value.  */   \
			size_t argc;         /* Argument count.  */   \
                                                          \
			/* Get arguments. */                          \
			argc = 0;                                     \
			status = napi_get_cb_info(env, info, &argc, NULL, NULL, NULL); \
			if (status != napi_ok) \
				return (NULL);     \
                                   \
			/* Check number of arguments. */ \
			if (argc != 0)                   \
			{\
				napi_throw_error(env, NULL,\
					"Error, "#name " _do not_ expects any arguments!\n"); \
				return (NULL); \
			}                  \
                               \
			/* Call C routine and return. */ \
			func_ret = name();               \
                                             \
			status = napi_create_##js_ret(env, func_ret, &napi_ret); \
			if (status != napi_ok)   \
				return (NULL);       \
                                     \
			return (napi_ret);       \
		}

#endif /* MACROS_H */
