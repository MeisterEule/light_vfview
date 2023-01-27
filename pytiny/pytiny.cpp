#include <Python.h>
#include "tinyexpr/tinyexpr.h"
#include <stdint.h>

#include <vector>

typedef struct {
   char *formula;
   uint64_t hash;
   te_expr *expr;
} formula_expr_t;

//typedef struct {
//   char *symbol;
//   te_variable var;
//} symbol_expr_t;

typedef struct {
  PyObject_HEAD
  std::vector<te_variable> vars;
  std::vector<formula_expr_t> formulas;
  std::vector<double> values;
  bool ready;
} pytiny_t;

static int pytiny_tp_clear (pytiny_t *self) {
  return 0;
}

static void pytiny_tp_dealloc (pytiny_t *self) {
  pytiny_tp_clear(self);
}

static int pytiny_tp_traverse (pytiny_t *self, visitproc visit, void *arg) {
  return 0;
}

static int pytiny_tp_init (pytiny_t *self, PyObject *args, PyObject *kwargs) {
  self->ready = false;
  return 0;
}

static PyObject *pytiny_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwargs) {
  pytiny_t *self = (pytiny_t*)type->tp_alloc(type, 0);
  return (PyObject*)self; 
}

static PyObject *register_symbols (pytiny_t *self, PyObject *args) {
  PyObject *pList;
  if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &pList)) {
    printf ("Error: Argument must be a list\n");
    return Py_BuildValue("i", -1);
  }
  Py_ssize_t n = PyList_Size(pList);
  for (int i = 0; i < n; i++) {
    PyObject *pItem = PyList_GetItem(pList, i);
    PyObject *string = PyUnicode_AsUTF8String(pItem);
    self->values.push_back(0.0);
    te_variable var;
    var.name = strdup(PyBytes_AsString(string));
    var.address = self->values.data() + self->values.size();
    self->vars.push_back(var);
  }

//  te_variable *all_vars = (te_variable*)malloc(self->symbols.size() * sizeof(te_variable));
//
  int err;
  for (formula_expr_t e: self->formulas) {
    self->ready = (e.expr = te_compile(e.formula, self->vars.data(), self->vars.size(), &err)) != NULL;
  }
  return Py_BuildValue("i", 0);
}

static PyObject *register_formulas (pytiny_t *self, PyObject *args) {
  PyObject *pList;
  if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &pList)) {
    printf ("Error: Argument must be a list\n");
    return Py_BuildValue("i", -1);
  }
  Py_ssize_t n = PyList_Size(pList);

  for (int i = 0; i < n; i++) {
    PyObject *pItem = PyList_GetItem (pList, i);
    PyObject *string = PyUnicode_AsUTF8String(pItem);
    formula_expr_t e;
    e.formula = strdup(PyBytes_AsString(string));
    e.hash = 0;
    int err;
    self->ready = (e.expr = te_compile(e.formula, self->vars.data(), self->vars.size(), &err)) != NULL;
    self->formulas.push_back(e);
  }

  Py_RETURN_NONE;
}

static PyObject *show (pytiny_t *self) {
  printf ("READY: %s\n", self->ready ? "YES" : "NO");
  printf ("Nr. of symbols: %d\n", self->vars.size());
  for (te_variable var: self->vars) {
	  printf ("Symbol name: %s\n", var.name);
	  printf ("Associated: 0x%lx\n", var.address);
  }
  printf ("Nr. of values: %d\n", self->values.size());
  for (double v: self->values) {
	  printf ("%lf\n", v);
  }
  Py_RETURN_NONE;
}

static PyMethodDef pytiny_tp_methods[] = {
	{
		"register_symbols", (PyCFunction)register_symbols, METH_VARARGS,
		"register new symbols"
	},
	{
		"register_formulas", (PyCFunction)register_formulas, METH_VARARGS,
		"register new formulas"
	},
	{
		"show", (PyCFunction)show, METH_NOARGS, "show state"
	},
	{NULL, NULL, 0, NULL}
};

static PyTypeObject pytinyType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pytiny",
  .tp_basicsize = sizeof(pytiny_t),
  .tp_dealloc = (destructor)pytiny_tp_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_traverse = (traverseproc)pytiny_tp_traverse,
  .tp_clear = (inquiry)pytiny_tp_clear,
  .tp_methods = pytiny_tp_methods,
  .tp_init = (initproc)pytiny_tp_init,
  .tp_new = pytiny_tp_new,
};

static struct PyModuleDef pytiny_definition = {
	PyModuleDef_HEAD_INIT,
	"pytiny",
	"Python interface for tinyexpr",
	-1,
	pytiny_tp_methods
};

PyMODINIT_FUNC PyInit_pytiny (void) {
	Py_Initialize();
	PyObject *thisPy = PyModule_Create(&pytiny_definition);
	PyModule_AddType(thisPy, &pytinyType);
	return thisPy;
}
