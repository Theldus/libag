%module libag

%{
#include "libag.h"
%}

%include exception.i
%include typemaps.i

/*
 * Maps a list to int npaths, char **target_paths
 * Example: ["foo"] -> npaths = 1, target_paths[0] = "foo"
 *
 * This is specially used in the ag_search() routine.
 *
 * Typemap extracted from SWIG v4.0.2. in:
 *   Examples/python/multimap/example.i
 */
%typemap(in,fragment="t_output_helper") (int npaths, char **target_paths)
{
	int i;
	if (!PyList_Check($input))
		SWIG_exception(SWIG_ValueError, "Expecting a list");

	$1 = (int)PyList_Size($input);
	if ($1 == 0)
		SWIG_exception(SWIG_ValueError, "List must contain at least 1 element");

	$2 = malloc(($1+1)*sizeof(char *));

	for (i = 0; i < $1; i++)
	{
		PyObject *s = PyList_GetItem($input,i);
%#if PY_VERSION_HEX >= 0x03000000
		if (!PyUnicode_Check(s))
%#else
		if (!PyString_Check(s))
%#endif
		{
			free($2);
			SWIG_exception(SWIG_ValueError, "List items must be strings");
		}
%#if PY_VERSION_HEX >= 0x03000000
		{
			PyObject *utf8str = PyUnicode_AsUTF8String(s);
			const char *cstr;
			if (!utf8str)
				SWIG_fail;
			cstr = PyBytes_AsString(utf8str);
			$2[i] = strdup(cstr);
			Py_DECREF(utf8str);
		}
%#else
		$2[i] = PyString_AsString(s);
%#endif
	}
	$2[i] = 0;
}

/*
 * Complementary typemap from the above typemap =).
 */
%typemap(freearg) (int count, char *argv[])
{
%#if PY_VERSION_HEX >= 0x03000000
	int i;
	for (i = 0; i < $1; i++)
		free($2[i]);
%#endif
}

/*
 * Typemap that maps a tuple to the arguments of the ag_free_all_results
 * routine. This typemap recreates the (struct ag_results **) based on
 * the individual elements present in the tuple and feeds that into the
 * function. There is no need to free the pointer here, as
 * ag_free_all_results already does.
 */
%typemap(in) (struct ag_result **results, size_t nresults)
{
	struct ag_result *r_c;
	PyObject *r_py;
	size_t i;

	if (!PyTuple_Check($input))
		SWIG_exception(SWIG_ValueError, "Expecting a tuple");

	/* nresults argument. */
	$2 = (size_t)PyTuple_Size($input);
	if (!$2)
		SWIG_exception(SWIG_ValueError, "Tuple must not be empty!");

	/* struct ag_result **results, argument. */
	$1 = malloc($2 * sizeof(struct person *));

	/* Add all tuple elements into the structure. */
	for (i = 0; i < $2; i++)
	{
		r_py = PyTuple_GetItem($input, i);

		/* Get C pointer from PyObject. */
		if (SWIG_ConvertPtr(r_py, (void **)&r_c, SWIGTYPE_p_ag_result,
			SWIG_POINTER_EXCEPTION) == -1)
		{
			return NULL;
		}
		$1[i] = r_c;
	}
	/* Pointer is freed via the proper function. */
}

/*
 * Typemap that converts an 'struct ag_result **' to a Python tuple.
 * Each tuple element is a 'struct ag_result *' and still requires
 * to do a proper cleanup later.
 *
 * Please note that this typemap is different from the one below
 * (ag_match): it releases the first pointer, and we rebuild it
 * again into another typemap. This proved to be necessary since
 * when we return a tuple, we lose the address of the first pointer,
 * which would be a memory leak, so we release it.
 *
 * In ag_match's typemap, this is not necessary, as the original
 * address is always saved in the structure itself, and we can
 * clear everything later (in the ag_free_result/ag_free_all_results
 * routines).
 *
 */
%typemap(out) struct ag_result **
{
	PyObject *list;
	PyObject *py_ptr;
	struct ag_result **tmp;
	struct ag_result *p;
	size_t size;

	if (!$1)
	{
		$result = Py_None;
		goto out;
	}

	size = 0;
	tmp  = $1;

	while (*tmp != NULL)
	{
		size++;
		tmp++;
	}

	$result = PyTuple_New(size);
	if (!$result)
	{
		PyErr_SetString(PyExc_Exception,
			"(TypeMap ag_result) Cannot allocate Tuple");
		return (NULL);
	}

	for (size_t i = 0; i < size; ++i)
	{
		p = $1[i];
		py_ptr = SWIG_NewPointerObj(SWIG_as_voidptr(p),
			SWIGTYPE_p_ag_result, 0|0);
		PyTuple_SetItem($result, i, py_ptr);
	}
	free($1);
out:
	;
}

/*
 * Typemap that converts an 'struct ag_match **' to a Python tuple.
 * Each tuple element is a 'struct ag_match *' and still requires
 * to do a proper cleanup later.
 */
%typemap(out) struct ag_match **
{
	PyObject *py_ptr;
	struct ag_match **tmp;
	struct ag_match *p;
	size_t size;

	if (!$1)
	{
		$result = Py_None;
		goto out;
	}

	size = 0;
	tmp  = $1;

	while (*tmp != NULL)
	{
		size++;
		tmp++;
	}

	$result = PyTuple_New(size);
	if (!$result)
	{
		PyErr_SetString(PyExc_Exception, "(TypeMap) Cannot allocate Tuple");
		return (NULL);
	}

	for (size_t i = 0; i < size; ++i)
	{
		p = $1[i];
		py_ptr = SWIG_NewPointerObj(SWIG_as_voidptr(p),
			SWIGTYPE_p_ag_match, 0|0);
		PyTuple_SetItem($result, i, py_ptr);
	}
	/* Not required a free here, since we do not lost the pointer. */
out:
	;
}

/*
 * Typemap that maps the return _and_ the 'size_t *nresults'
 * argument of the ag_search routine: both 'get merged' and become
 * a tuple. So the expected way to invoke ag_search (in Python)
 * would be:
 *
 *    nresults, results = ag_search("query", [list]);
 *
 * Where 'nresults' stores the number of results (0 if none) and
 * 'results' the result tuple (None if none).
 */
%typemap(in, numinputs=0) size_t *nresults(size_t tmp) {
  $1 = &tmp;
}

%typemap(argout) size_t *nresults
{
	PyObject *old_result;
	old_result = $result;

	$result = PyTuple_New(2);
	if (!$result)
	{
		PyErr_SetString(PyExc_Exception, "Cannot allocate Tuple");
		return (NULL);
	}

	/* Check if have some output, if not, set tuple (0, None). */
	if (!old_result || (old_result == Py_None))
	{
		PyTuple_SetItem($result, 0, PyLong_FromSize_t(0));
		PyTuple_SetItem($result, 1, Py_None);
	}

	else
	{
		/* Add nresult. */
		PyTuple_SetItem($result, 0, PyLong_FromSize_t(*$1));
		/* Add proper result. */
		PyTuple_SetItem($result, 1, old_result);
	}
}

%include "libag.h"
