#include <stdio.h>

#include <Python.h>

typedef struct {
    char *name;
    int caller;
    int ncallees;
    int *callees;
    bool precise;
} vftr_stack_t;

typedef enum {
   samp_function_entry,
   samp_function_exit,
   samp_message
} sample_kind;

typedef struct {
   int stack_id;
   long long ts_first;
   long long ts_last; 
} sample_block_t;


typedef struct {
   PyObject_HEAD
   FILE *vfd_fp = NULL;
   int vfd_version;
   char *package_string = NULL;
   char *datestr_start = NULL;
   char *datestr_end = NULL;
   long long interval;
   int nprocesses;
   int processID;
   int nthreads;
   double runtime;
   unsigned int function_samplecount;
   unsigned int message_samplecount;
   unsigned int nstacks;
   unsigned int n_hw_counters;
   unsigned int n_hw_observables;
   long int samples_offset;
   long int stacks_offset;
   long int threadtree_offset;
} vfd_parse_t;

static int vfdParse_tp_init (vfd_parse_t *self, PyObject *args, PyObject *kwargs) {
   char *filename;
   if (!PyArg_ParseTuple(args, "s", &filename)) {
	   printf ("Could not init VFD parser: invalid argument\n");
	   return -1;
   }

   self->vfd_fp = fopen(filename, "r");
   fread(&(self->vfd_version), sizeof(int), 1, self->vfd_fp);
   int package_string_len;
   fread(&package_string_len, sizeof(int), 1, self->vfd_fp);
   self->package_string = (char*)malloc(package_string_len * sizeof(char));
   fread(self->package_string, sizeof(char), package_string_len, self->vfd_fp);
   int datestr_len;
   fread(&datestr_len, sizeof(int), 1, self->vfd_fp);
   self->datestr_start = (char*)malloc(datestr_len * sizeof(char));
   self->datestr_end = (char*)malloc(datestr_len * sizeof(char));
   fread (self->datestr_start, sizeof(char), datestr_len, self->vfd_fp);
   fread (self->datestr_end, sizeof(char), datestr_len, self->vfd_fp);
   fread (&(self->interval), sizeof(long long), 1, self->vfd_fp);
   fread (&(self->nprocesses), sizeof(int), 1, self->vfd_fp);
   fread (&(self->processID), sizeof(int), 1, self->vfd_fp);
   fread (&(self->nthreads), sizeof(int), 1, self->vfd_fp);
   fread (&(self->runtime), sizeof(double), 1, self->vfd_fp);
   fread (&(self->function_samplecount), sizeof(unsigned int), 1, self->vfd_fp);
   fread (&(self->message_samplecount), sizeof(unsigned int), 1, self->vfd_fp);
   fread (&(self->nstacks), sizeof(unsigned int), 1, self->vfd_fp);
   fread (&(self->n_hw_counters), sizeof(unsigned int), 1, self->vfd_fp);
   fread (&(self->n_hw_observables), sizeof(unsigned int), 1, self->vfd_fp);
   fread (&(self->samples_offset), sizeof(long int), 1, self->vfd_fp);
   fread (&(self->stacks_offset), sizeof(long int), 1, self->vfd_fp);
   fread (&(self->threadtree_offset), sizeof(long int), 1, self->vfd_fp);
   return 0;
};

static PyObject *vfdParse_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwargs) {
   vfd_parse_t *self = (vfd_parse_t*)type->tp_alloc(type, 0);
   return (PyObject*)self;
};

static int vfdParse_tp_clear (vfd_parse_t *self) {
   if (self->vfd_fp != NULL) fclose(self->vfd_fp);
   if (self->package_string != NULL) free(self->package_string);
   if (self->datestr_start != NULL) free(self->datestr_start);
   if (self->datestr_end != NULL) free(self->datestr_end);
   return 0;
}

static void vfdParse_tp_dealloc (vfd_parse_t *self) {
   vfdParse_tp_clear (self);
}

static int vfdParse_tp_traverse (vfd_parse_t *self, visitproc visit, void *arg) {
   return 0;
}

static PyObject *get_timestamps (vfd_parse_t *self, PyObject *args, PyObject *kwargs);
static PyObject *get_stackstrings (vfd_parse_t *self, PyObject *args);
static PyObject *show_header (vfd_parse_t *self);

static PyMethodDef vfd_parse_methods[] = {
	{
	    "show_header", (PyCFunction)show_header, METH_NOARGS, "Show header"
	},
	{
	    "get_timestamps", (PyCFunction)get_timestamps, METH_VARARGS | METH_KEYWORDS,
	    "sample vfd",
	},
	{
	    "get_stackstrings", (PyCFunction)get_stackstrings, METH_VARARGS,
	    "get stack strings",
	},
	{NULL, NULL, 0, NULL}
};

static PyTypeObject vfdParseType = {
   PyVarObject_HEAD_INIT(NULL, 0)
   .tp_name = "vfdParser",
   .tp_basicsize = sizeof(vfd_parse_t),
   .tp_dealloc = (destructor)vfdParse_tp_dealloc,
   .tp_flags = Py_TPFLAGS_DEFAULT,
   .tp_traverse = (traverseproc)vfdParse_tp_traverse,
   .tp_clear = (inquiry)vfdParse_tp_clear,
   .tp_methods = vfd_parse_methods,
   .tp_init = (initproc)vfdParse_tp_init,
   .tp_new = vfdParse_tp_new
};

static PyObject *show_header (vfd_parse_t *self) {
   printf ("VFD version: %d\n", self->vfd_version);
   printf ("Package string: %s\n", self->package_string);
   printf ("Start date: %s\n", self->datestr_start);
   printf ("End date: %s\n", self->datestr_end);
   printf ("Sampling interval: %lld\n", self->interval);
   printf ("Nr. of processes: %d\n", self->nprocesses);
   printf ("Process ID: %d\n", self->processID);
   printf ("Nr. of threads: %d\n", self->nthreads);
   printf ("Nr. of sampled functions: %d\n", self->function_samplecount);
   printf ("Nr. of sampled messages: %d\n", self->message_samplecount);
   printf ("Nr. of stacks: %d\n", self->nstacks);
   printf ("Nr. of hardware counters: %d\n", self->n_hw_counters);
   printf ("Nr. of hardware observables: %d\n", self->n_hw_observables);
   Py_RETURN_NONE;
}

static PyObject *get_timestamps (vfd_parse_t *self, PyObject *args, PyObject *kwargs) {
   int n_readout = -1;
   int merge = 0;
   static char *keywords[] = {"merge", "n_readout", NULL};
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", keywords, &merge, &n_readout)) {
	   printf ("Error: Bad arguments!\n");
	   Py_RETURN_NONE;
   }
   
   int n_samples = self->function_samplecount + self->message_samplecount;
   int *stack_ids = (int*)malloc(n_samples * sizeof(int));
   long long *timestamps = (long long*)malloc(n_samples * sizeof(long long));
   long long **counters = (long long**)malloc(n_samples * sizeof(long long*));
   for (int i = 0; i < n_samples; i++) {
	   counters[i] = (long long*)malloc(self->n_hw_counters * sizeof(long long));
   }
   int i_sample = 0;
   int stack_id;
   fseek (self->vfd_fp, self->samples_offset, SEEK_SET);
   for (int i = 0; i < n_samples; i++) {
     sample_kind kind;
     fread (&kind, sizeof(sample_kind), 1, self->vfd_fp);
     switch (kind) {
       case samp_function_entry:
       case samp_function_exit:
	  fread(&(stack_ids[i_sample]), sizeof(int), 1, self->vfd_fp);
	  fread(&(timestamps[i_sample]), sizeof(long long), 1, self->vfd_fp);
	  for (int ic = 0; ic < self->n_hw_counters; ic++) {
		  fread(&(counters[i_sample][ic]), sizeof(long long), 1, self->vfd_fp);
	  }
	  i_sample++;
	  break;
       case samp_message:
	  break;
       default:
	  break;
     }
   }

   int n_return_samples = 0;
   long long *return_ts = NULL;
   long long *return_tf = NULL;
   int *return_stackids = NULL;
   long long **return_counters = NULL;
   if (merge) {
      int current_stackid = -1;
      for (int i = 0; i < n_samples; i++) {
        int this_stackid = stack_ids[i];
        if (this_stackid != current_stackid) {
           n_return_samples++;
           current_stackid = this_stackid;
        }
      }

      return_ts = (long long*)malloc(n_return_samples * sizeof(long long));
      return_tf = (long long*)malloc(n_return_samples * sizeof(long long));
      return_stackids = (int*)malloc(n_return_samples * sizeof(int));
      return_counters = (long long**)malloc(n_return_samples * sizeof(long long*));
      for (int i = 0; i < n_return_samples; i++) {
	      return_counters[i] = (long long*)malloc(n_return_samples * sizeof(long long));
      }

      current_stackid = stack_ids[0];
      return_ts[0] = timestamps[0];
      return_stackids[0] = stack_ids[0];
      i_sample = 0;
      for (int i = 0; i < n_samples; i++) {
         int this_stackid = stack_ids[i];
         if (this_stackid != current_stackid) {
            i_sample++;
            return_stackids[i_sample] = this_stackid;
            return_ts[i_sample] = timestamps[i];
            return_tf[i_sample] = timestamps[i];
	    for (int ic = 0; ic < self->n_hw_counters; ic++) {
	       return_counters[i_sample][ic] = counters[i_sample][ic];
	    }
            current_stackid = this_stackid;
         } else {
            return_tf[i_sample] = timestamps[i];
         }
      }
   } else {
      n_return_samples = n_samples;
      return_ts = (long long*)malloc(n_return_samples * sizeof(long long));
      return_tf = (long long*)malloc(n_return_samples * sizeof(long long));
      return_stackids = (int*)malloc(n_return_samples * sizeof(int));
      return_counters = (long long**)malloc(n_return_samples * sizeof(long long*));
      for (int i = 0; i < n_return_samples; i++) {
	      return_counters[i] = (long long*)malloc(n_return_samples * sizeof(long long));
      }
      for (int i_sample = 0; i_sample < n_samples; i_sample++) {
	      return_ts[i_sample] = timestamps[i_sample];
	      return_tf[i_sample] = timestamps[i_sample];
	      return_stackids[i_sample] = stack_ids[i_sample];
              for (int ic = 0; ic < self->n_hw_counters; ic++) {
                       return_counters[i_sample][ic] = counters[i_sample][ic];
              }
      }
   }


   // If no readout value is supplied, readout all values
   if (n_readout < 0) n_readout = n_return_samples;
   PyObject *ret = PyDict_New();

   PyObject *stackid_list = PyList_New (n_readout);
   PyObject *stackid_key = Py_BuildValue("s", "stackid");
   for (int i = 0; i < n_readout; i++) {
      PyObject *python_i = Py_BuildValue("i", return_stackids[i]);
      PyList_SetItem (stackid_list, i, python_i);
   }
   PyDict_SetItem (ret, stackid_key, stackid_list);

   PyObject *start_list = PyList_New (n_readout);
   PyObject *start_key = Py_BuildValue ("s", "start");
   for (int i = 0; i < n_readout; i++) {
      PyObject *python_ll = Py_BuildValue("L", return_ts[i]);
      PyList_SetItem (start_list, i, python_ll);
   }
   PyDict_SetItem (ret, start_key, start_list);

   PyObject *end_list = PyList_New (n_readout);
   PyObject *end_key = Py_BuildValue ("s", "end");
   for (int i = 0; i < n_readout; i++) {
      PyObject *python_ll = Py_BuildValue("L", return_tf[i]);
      PyList_SetItem (end_list, i, python_ll);
   }
   PyDict_SetItem (ret, end_key, end_list);

   PyObject *counter_list = PyList_New (n_readout);
   PyObject *counter_key = Py_BuildValue ("s", "counters");
   for (int i = 0; i < n_readout; i++) {
	   PyObject *tmp_list = PyList_New (self->n_hw_counters);
	   for (int ic = 0; ic < self->n_hw_counters; ic++) {
		   PyObject *python_ll = Py_BuildValue("L", counters[i][ic]);
		   PyList_SetItem (tmp_list, ic, python_ll);
	   }
	   PyList_SetItem (counter_list, i, tmp_list);
   }
   PyDict_SetItem (ret, counter_key, counter_list);

   free(timestamps);
   free(stack_ids);
   free(return_ts);
   free(return_tf);
   free(return_stackids);
   return ret;
}

int get_stack_length (unsigned int istack, vftr_stack_t *stacklist) {
	int n = strlen(stacklist[istack].name);
	if (stacklist[istack].caller >= 0) {
		n++; // Add "<"
		n += get_stack_length (stacklist[istack].caller, stacklist);	
	}
	return n;
}

void fill_stack_string (unsigned int istack, vftr_stack_t *stacklist, char *stackstr) {
	char *name = stacklist[istack].name;
	int n = strlen(name);
	snprintf (stackstr, n + 1, "%s", name);
	if (stacklist[istack].caller >= 0) {
		stackstr += n;
		snprintf (stackstr, 2, "%s", "<");
		stackstr++;
		fill_stack_string (stacklist[istack].caller, stacklist, stackstr);
	}
}

void print_stack (unsigned int istack, vftr_stack_t *stacklist) {
	printf ("%s", stacklist[istack].name);
	if (stacklist[istack].caller >= 0) {
		printf ("<");
		print_stack(stacklist[istack].caller, stacklist);
	}
}

static PyObject *get_stackstrings (vfd_parse_t *self, PyObject *args) {
	vftr_stack_t *stacklist = (vftr_stack_t*)malloc(self->nstacks * sizeof(vftr_stack_t));	
	fseek (self->vfd_fp, self->stacks_offset, SEEK_SET);

	int namelen;
	// First function is the init with caller id -1
	stacklist[0].ncallees = 0;

	size_t read_elems;
        read_elems = fread(&(stacklist[0].caller), sizeof(int), 1, self->vfd_fp);	
	read_elems = fread(&namelen, sizeof(int), 1, self->vfd_fp);
	stacklist[0].name = (char*)malloc(namelen * sizeof(char));
	read_elems = fread(stacklist[0].name, sizeof(char), namelen, self->vfd_fp);

	// all other stacks
	for (unsigned int istack = 1; istack < self->nstacks; istack++) {
		stacklist[istack].ncallees = 0;
		read_elems = fread(&(stacklist[istack].caller), sizeof(int), 1, self->vfd_fp);
		stacklist[stacklist[istack].caller].ncallees++;

		fread(&namelen, sizeof(int), 1, self->vfd_fp);
		stacklist[istack].name = (char*)malloc(namelen * sizeof(char));
		read_elems = fread(stacklist[istack].name, sizeof(char), namelen, self->vfd_fp);
	}	

	if (stacklist[0].ncallees > 0) {
		stacklist[0].callees = (int*)malloc(stacklist[0].ncallees * sizeof(int));
		stacklist[0].ncallees = 0;
		for (unsigned int istack = 1; istack < self->nstacks; istack++) {
			if (stacklist[istack].ncallees > 0) {
				stacklist[istack].callees = (int*)malloc(stacklist[istack].ncallees * sizeof(int));
				stacklist[istack].ncallees = 0;
			}
			vftr_stack_t *caller = stacklist + stacklist[istack].caller;
			caller->callees[caller->ncallees] = istack;
			caller->ncallees++;
		}
	}

	
	PyObject *ret = PyList_New(self->nstacks);
	for (unsigned int istack = 0; istack < self->nstacks; istack++) {
		int slen = get_stack_length(istack, stacklist) + 1;
		char *stackstring = (char*)malloc(slen * sizeof(char));
		fill_stack_string (istack, stacklist, stackstring);
		PyObject *python_s = Py_BuildValue("s", stackstring);
		PyList_SetItem (ret, istack, python_s);
		free(stackstring);
	}

	return ret;
}

static struct PyModuleDef vfd_parse_definition = {
	PyModuleDef_HEAD_INIT,
	"vfd_parse",
	"parse vfd files",
	-1,
	vfd_parse_methods
};

PyMODINIT_FUNC PyInit_vfd_parse (void) {
	Py_Initialize();
	PyObject *thisPy = PyModule_Create(&vfd_parse_definition);
	PyModule_AddType(thisPy, &vfdParseType);
	return thisPy;
}
