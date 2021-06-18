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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <node_api.h>

#include <libag.h>
#include "macros.h"

/**
 * @brief For a given JS string, allocates enough space and
 * save into the buffer.
 *
 * @param env N-api environment.
 * @param js_str N-api value corresponding to the string.
 *
 * @return If success, returns a string, otherwise, NULL.
 */
static char* get_string_from_js(napi_env env, napi_value js_str)
{
	napi_status status;
	size_t len;
	char *str;

	str = NULL;
	len = 0;

	/* Get string length. */
	status = napi_get_value_string_utf8(env, js_str, NULL, 0, &len);
	if (status != napi_ok || !len)
		return (NULL);

	/* Allocate space and get string. */
	str = malloc(sizeof(char) * (len+1));
	if (!str)
		return (NULL);

	status = napi_get_value_string_utf8(env, js_str, str, len+1, &len);
	if (status != napi_ok)
		return (NULL);

	str[len] = '\0';
	return (str);
}

/*
 * Struct ag_search_stats
 */
DEFINE_TAG(ag_search_stats, 0x025b1c7e1a1b4644, 0xaa08929ba4760785)
DEFINE_GETTER_AND_SETTER(ag_search_stats, total_bytes, SIZE_T)
DEFINE_GETTER_AND_SETTER(ag_search_stats, total_files, SIZE_T)
DEFINE_GETTER_AND_SETTER(ag_search_stats, total_matches, SIZE_T)
DEFINE_GETTER_AND_SETTER(ag_search_stats, total_file_matches, SIZE_T)
DEFINE_STRUCT(ag_search_stats,
	{
		DECLARE_NAPI_FIELD(total_bytes),
		DECLARE_NAPI_FIELD(total_files),
		DECLARE_NAPI_FIELD(total_matches),
		DECLARE_NAPI_FIELD(total_file_matches)
	}
)

/*
 * Struct ag_config
 */
DEFINE_TAG(ag_config, 0x357c3ee4aa09406e, 0xb0c012487f874d61)
DEFINE_GETTER_AND_SETTER(ag_config, literal,             int32)
DEFINE_GETTER_AND_SETTER(ag_config, disable_recurse_dir, int32)
DEFINE_GETTER_AND_SETTER(ag_config, casing,              int32)
DEFINE_GETTER_AND_SETTER(ag_config, num_workers,         int32)
DEFINE_GETTER_AND_SETTER(ag_config, workers_behavior,    int32)
DEFINE_GETTER_AND_SETTER(ag_config, stats,               int32)
DEFINE_GETTER_AND_SETTER(ag_config, search_binary_files, int32)
DEFINE_STRUCT(ag_config,
	{
		DECLARE_NAPI_FIELD(literal),
		DECLARE_NAPI_FIELD(disable_recurse_dir),
		DECLARE_NAPI_FIELD(casing),
		DECLARE_NAPI_FIELD(num_workers),
		DECLARE_NAPI_FIELD(workers_behavior),
		DECLARE_NAPI_FIELD(stats),
		DECLARE_NAPI_FIELD(search_binary_files)
	}
)

/**
 * Wrapper for ag_init function.
 */
DEFINE_SIMPLE_WRAPPER_RET_VOID(ag_init, int32, int)

/**
 * Wrapper for ag_finish function.
 */
DEFINE_SIMPLE_WRAPPER_RET_VOID(ag_finish, int32, int)

/**
 * Wrapper for ag_start_workers() function.
 */
DEFINE_SIMPLE_WRAPPER_RET_VOID(ag_start_workers, int32, int)

/**
 * Wrapper for ag_stop_workers() function.
 */
DEFINE_SIMPLE_WRAPPER_RET_VOID(ag_stop_workers, int32, int)

/**
 * Wrapper for ag_init_config() function.
 */
static napi_value wrap_ag_init_config(napi_env env, napi_callback_info info)
{
	struct ag_config *config; /* libag ag_config structure. */
	napi_value napi_ret;      /* JS return value.           */
	napi_status status;       /* JS status code.            */
	napi_value args[1];       /* Arguments.                 */
	int func_ret;             /* C return value.            */
	size_t argc;              /* Argument count.            */
	bool is_tag;              /* Tag flag indicator.        */

	/* Get arguments. */
	argc = 1;
	status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
	if (status != napi_ok)
		return (NULL);

	/* Check number of arguments. */
	if (argc != 1)
	{
		napi_throw_error(env, NULL,
			"Error, ag_init_config expects 1 argument of type ag_config!\n");
		return (NULL);
	}

	/* Check argument type. */
	status = napi_check_object_type_tag(env, args[0], &ag_config_tag,
		&is_tag);
	if (status != napi_ok)
		return (NULL);
	if (!is_tag)
	{
		napi_throw_type_error(env, NULL, "Error, ag_config was expected!\n");
		return (NULL);
	}          

	/* Unwrap. */
	status = napi_unwrap(env, args[0], (void**)&config);
	if (status != napi_ok)
		return (NULL);

	/* Call C routine and return. */
	func_ret = ag_init_config(config);

	status = napi_create_int32(env, func_ret, &napi_ret);
	if (status != napi_ok)
		return (NULL);

	return (napi_ret);
}

/**
 * Wrapper for ag_set_config() function.
 */
static napi_value wrap_ag_set_config(napi_env env, napi_callback_info info)
{
	struct ag_config *config; /* libag ag_config structure. */
	napi_value napi_ret;      /* JS return value.           */
	napi_status status;       /* JS status code.            */
	napi_value args[1];       /* Arguments.                 */
	int func_ret;             /* C return value.            */
	size_t argc;              /* Argument count.            */
	bool is_tag;              /* Tag flag indicator.        */

	/* Get arguments. */
	argc = 1;
	status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
	if (status != napi_ok)
		return (NULL);

	/* Check number of arguments. */
	if (argc != 1)
	{
		napi_throw_error(env, NULL,
			"Error, ag_set_config expects 1 argument of type ag_config!\n");
		return (NULL);
	}

	/* Check argument type. */
	status = napi_check_object_type_tag(env, args[0], &ag_config_tag,
		&is_tag);
	if (status != napi_ok)
		return (NULL);
	if (!is_tag)
	{
		napi_throw_type_error(env, NULL, "Error, ag_config was expected!\n");
		return (NULL);
	}          

	/* Unwrap. */
	status = napi_unwrap(env, args[0], (void**)&config);
	if (status != napi_ok)
		return (NULL);

	/* Call C routine and return. */
	func_ret = ag_set_config(config);

	status = napi_create_int32(env, func_ret, &napi_ret);
	if (status != napi_ok)
		return (NULL);

	return (napi_ret);
}

/**
 * Wrapper for ag_get_stats() function.
 */
static napi_value wrap_ag_get_stats(napi_env env, napi_callback_info info)
{
	struct ag_search_stats *stats; /* libag ag_config structure. */
	napi_value napi_ret;           /* JS return value.           */
	napi_status status;            /* JS status code.            */
	napi_value args[1];            /* Arguments.                 */
	int func_ret;                  /* C return value.            */
	size_t argc;                   /* Argument count.            */
	bool is_tag;                   /* Tag flag indicator.        */

	/* Get arguments. */
	argc = 1;
	status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
	if (status != napi_ok)
		return (NULL);

	/* Check number of arguments. */
	if (argc != 1)
	{
		napi_throw_error(env, NULL,
			"Error, ag_get_stats expects 1 argument of type ag_search_stats!\n");
		return (NULL);
	}

	/* Check argument type. */
	status = napi_check_object_type_tag(env, args[0], &ag_search_stats_tag,
		&is_tag);
	if (status != napi_ok)
		return (NULL);
	if (!is_tag)
	{
		napi_throw_type_error(env, NULL,
			"Error, ag_search_stats was expected!\n");
		return (NULL);
	}          

	/* Unwrap. */
	status = napi_unwrap(env, args[0], (void**)&stats);
	if (status != napi_ok)
		return (NULL);

	/* Call C routine and return. */
	func_ret = ag_get_stats(stats);

	status = napi_create_int32(env, func_ret, &napi_ret);
	if (status != napi_ok)
		return (NULL);

	return (napi_ret);
}

/**
 * Wrapper for ag_search() function.
 *
 * @note Important to note that the returned object is a pure JS object
 * and therefore do not need any wrappers, getters and setters. Hence,
 * there is no need to call ag_free_results()/ag_free_all_results().
 */
static napi_value wrap_ag_search(napi_env env, napi_callback_info info)
{
	struct ag_result **func_ret;   /* C return value.        */
	napi_value napi_ret;           /* JS return value.       */
	napi_value element;            /* Array index.           */
	napi_status status;            /* JS status code.        */
	napi_value args[2];            /* Arguments.             */
	uint32_t paths_len;            /* Number of paths.       */
	size_t nresults;               /* Number of results.     */
	bool is_array;                 /* Array flag indicator.  */
	char **paths;                  /* Paths list.            */
	uint32_t idx;                  /* Paths loop index.      */
	size_t argc;                   /* Argument count.        */
	char *query;                   /* Query/Regex.           */

	napi_value js_nresults;        /* JS nresults var.       */
	napi_value js_results;         /* JS results array.      */
	napi_value js_matches;         /* JS matches array.      */
	napi_value js_res_file;        /* JS file var.           */
	napi_value js_res_nmatches;    /* JS nmatches var.       */
	napi_value js_res_flags;       /* JS flags var.          */
	napi_value js_obj_result;      /* JS object result.      */
	napi_value js_obj_match;       /* JS object match.       */
	napi_value js_mat_bstart;      /* JS match byte start.   */
	napi_value js_mat_bend;        /* JS match byte end.     */
	napi_value js_mat_match;       /* JS match string.       */

	idx = 0;
	napi_ret = NULL;
	nresults = 0;

	/* Get arguments. */
	argc = 2;
	status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
	if (status != napi_ok)
		goto out1;

	/* Check number of arguments. */
	if (argc != 2)
	{
		napi_throw_error(env, NULL,
			"ag_search expects 2 arguments:\n  query (string), paths"
				" (array of strings)\n");
		goto out1;
	}

	/* Get query. */
	query = get_string_from_js(env, args[0]);
	if (!query)
	{
		napi_throw_error(env, NULL, "ag_search: cannot get query, please check "
			"if your string is valid!\n");
		goto out1;
	}

	/* Check if 2nd arg is an array and grab its size. */
	status = napi_is_array(env, args[1], &is_array);
	if (status != napi_ok || !is_array)
	{
		napi_throw_error(env, NULL, "ag_search: cannot get paths, please check "
			"if your array is valid!\n");
		goto out2;
	}
	status = napi_get_array_length(env, args[1], &paths_len);
	if (status != napi_ok || !paths_len)
	{
		napi_throw_error(env, NULL, "ag_search: your paths array should contain"
			" at least 1 path\n");
		goto out2;
	}

	/* Creates the path list and iterates over each array element. */
	paths = malloc(sizeof(char *) * paths_len);
	if (!paths)
		goto out2;

	for (idx = 0; idx < paths_len; idx++)
	{
		status = napi_get_element(env, args[1], idx, &element);
		if (status != napi_ok)
			goto out3;

		paths[idx] = get_string_from_js(env, element);
		if (!paths[idx])
		{
			napi_throw_error(env, NULL, "ag_search: array contains"
				" an invalid string!\n");
			goto out3;
		}
	}

	/* Call the search. */
	func_ret = ag_search(query, paths_len, paths, &nresults);
	if (!func_ret)
		goto out3;

	/* Prepare results. */
	status = napi_create_object(env, &napi_ret);
	if (status != napi_ok)
		goto out3;

	/* nresults. */
	status = napi_create_sizet(env, nresults, &js_nresults);
	if (status != napi_ok)
		goto out3;
	status = napi_set_named_property(env, napi_ret, "nresults", js_nresults);
	if (status != napi_ok)
		goto out3;

	/* Array of results. */
	status = napi_create_array_with_length(env, nresults, &js_results);
	if (status != napi_ok)
		goto out3;

	for (size_t i = 0; i < nresults; i++)
	{
		/* Create array element object. */
		status = napi_create_object(env, &js_obj_result);
		if (status != napi_ok)
			goto out3;

		/* Create elements. */
		status = napi_create_string_utf8(env, func_ret[i]->file,
			NAPI_AUTO_LENGTH, &js_res_file);
		if (status != napi_ok) goto out3;
		status = napi_create_sizet(env, func_ret[i]->nmatches, &js_res_nmatches);
		if (status != napi_ok) goto out3;
		status = napi_create_int32(env, func_ret[i]->flags, &js_res_flags);
		if (status != napi_ok) goto out3;

		/* Add names to them. */
		status = napi_set_named_property(env, js_obj_result, "file",
			js_res_file);
		if (status != napi_ok) goto out3;
		status = napi_set_named_property(env, js_obj_result, "nmatches",
			js_res_nmatches);
		if (status != napi_ok) goto out3;
		status = napi_set_named_property(env, js_obj_result, "flags",
			js_res_flags);
		if (status != napi_ok) goto out3;

		/* Create our match array. */
		status = napi_create_array_with_length(env, func_ret[i]->nmatches,
			&js_matches);

		/* Iterate over each match. */
		for (size_t j = 0; j < func_ret[i]->nmatches; j++)
		{
			/* Create array element object. */
			status = napi_create_object(env, &js_obj_match);
			if (status != napi_ok)
				goto out3;

			/* Create elements. */
			status = napi_create_sizet(env, func_ret[i]->matches[j]->byte_start,
				&js_mat_bstart);
			if (status != napi_ok) goto out3;
			status = napi_create_sizet(env, func_ret[i]->matches[j]->byte_end,
				&js_mat_bend);
			if (status != napi_ok) goto out3;
			status = napi_create_string_utf8(env, func_ret[i]->matches[j]->match,
				NAPI_AUTO_LENGTH, &js_mat_match);
			if (status != napi_ok) goto out3;

			/* Add names to them. */
			status = napi_set_named_property(env, js_obj_match, "byte_start",
				js_mat_bstart);
			if (status != napi_ok) goto out3;
			status = napi_set_named_property(env, js_obj_match, "byte_end",
				js_mat_bend);
			if (status != napi_ok) goto out3;
			status = napi_set_named_property(env, js_obj_match, "match",
				js_mat_match);
			if (status != napi_ok) goto out3;

			/* Add object to our array. */
			status = napi_set_element(env, js_matches, j, js_obj_match);
			if (status != napi_ok) goto out3;
		}

		/* Bind array of matches. */
		status = napi_set_named_property(env, js_obj_result, "matches",
			js_matches);
		if (status != napi_ok) goto out3;

		/* Add object to array. */
		status = napi_set_element(env, js_results, i, js_obj_result);
		if (status != napi_ok)
			goto out3;
	}

	/* Bind array with the return. */
	status = napi_set_named_property(env, napi_ret, "results", js_results);
	if (status != napi_ok)
		goto out3;

out3:
	for (uint32_t i = 0; i < idx; i++)
		free(paths[i]);
	free(paths);
	ag_free_all_results(func_ret, nresults);
out2:
	free(query);
out1:
	return (napi_ret);
}

/**
 * Initializes all methods all structures 'constructors'.
 */
napi_value init(napi_env env, napi_value exports)
{
	napi_status status;
	
	napi_property_descriptor properties[] = {
		/* Structs 'constructors' */
		DECLARE_NAPI_METHOD("ag_search_stats", ag_search_stats),
		DECLARE_NAPI_METHOD("ag_config", ag_config),
		/* Wrappers. */
		DECLARE_NAPI_METHOD("ag_init", wrap_ag_init),
		DECLARE_NAPI_METHOD("ag_finish", wrap_ag_finish),
		DECLARE_NAPI_METHOD("ag_start_workers", wrap_ag_start_workers),
		DECLARE_NAPI_METHOD("ag_stop_workers", wrap_ag_stop_workers),
		DECLARE_NAPI_METHOD("ag_init_config", wrap_ag_init_config),
		DECLARE_NAPI_METHOD("ag_set_config", wrap_ag_set_config),
		DECLARE_NAPI_METHOD("ag_get_stats", wrap_ag_get_stats),
		DECLARE_NAPI_METHOD("ag_search", wrap_ag_search)
	};

	status = napi_define_properties(env, exports,
		sizeof(properties)/sizeof(properties[0]), properties); 

	if (status != napi_ok)
		return (NULL);
  
	return (exports);
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
