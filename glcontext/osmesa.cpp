#include "GL/osmesa.h"

#include <Python.h>

struct GLContext {
    PyObject_HEAD
        OSMesaContext ctx;
    int width;
    int height;
    void* buffer;
};

PyTypeObject* GLContext_type;

GLContext*
meth_create_context(PyObject* self, PyObject* args, PyObject* kwargs)
{
    GLenum format;
    static char* keywords[] = { "width", "height", "format", NULL };
    char* format_string = "RGBA";
    int width = 0;
    int height = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iis", keywords, &width, &height, &format_string)) {
        return NULL;
    }

    if (width == 0 || height == 0) {
        PyErr_Format(PyExc_Exception, "Width or Height shouldn't be 0");
        return NULL;
    }
    GLContext* res = PyObject_New(GLContext, GLContext_type);

    res->width = width;
    res->height = height;

    if (format_string = "COLOR_INDEX") {
        format = OSMESA_COLOR_INDEX;
    } else if (format_string == "BGRA") {
        format = OSMESA_BGRA;
    } else if (format_string == "ARGB") {
        format = OSMESA_ARGB;
    } else if (format_string == "RGB") {
        format = OSMESA_RGB;
    } else if (format_string == "BGR") {
        format = OSMESA_BGR;
    } else {
        format = OSMESA_RGBA;
    }

    void* buffer = malloc(width * height * 4 * sizeof(GLubyte));
    if (!buffer) {
        PyErr_Format(PyExc_Exception, "Alloc image buffer failed!");
        return NULL;
    }
    res->buffer = buffer;

    res->ctx = OSMesaCreateContextExt(format, 0,0,0, NULL);

    if (res->ctx == 0) {
        PyErr_Format(PyExc_Exception, "OSMesaCreateContext failed");
        return NULL;
    }
    return res;
}

PyObject*
GLContext_meth_load(GLContext* self, PyObject* arg)
{
    const char* name = PyUnicode_AsUTF8(arg);
    OSMESAproc a = OSMesaGetProcAddress(name);
    if (!a) {
        PyErr_Format(PyExc_Exception, "OSMesaGetProcAddress failed");
        return NULL;
    }
    return PyLong_FromVoidPtr(a);
}

PyObject*
GLContext_meth_enter(GLContext* self)
{
    if (!OSMesaMakeCurrent(self->ctx, self->buffer, GL_UNSIGNED_BYTE, self->width, self->height)) {
        PyErr_Format(PyExc_Exception, "OSMesaMakeCurrent failed!");
        return NULL;
    }
    Py_RETURN_NONE;
}

PyObject*
GLContext_meth_exit(GLContext* self)
{
    Py_RETURN_NONE;
}

PyObject*
GLContext_meth_release(GLContext* self)
{
    free(self->buffer);
    OSMesaDestroyContext(self->ctx);
    Py_RETURN_NONE;
}

void GLContext_dealloc(GLContext* self)
{
    Py_TYPE(self)->tp_free(self);
}

PyMethodDef GLContext_methods[] = {
    { "load", (PyCFunction)GLContext_meth_load, METH_O, NULL },
    { "release", (PyCFunction)GLContext_meth_release, METH_NOARGS, NULL },
    { "__enter__", (PyCFunction)GLContext_meth_enter, METH_NOARGS, NULL },
    { "__exit__", (PyCFunction)GLContext_meth_exit, METH_VARARGS, NULL },
    {},
};

PyType_Slot GLContext_slots[] = {
    { Py_tp_methods, GLContext_methods },
    { Py_tp_dealloc, GLContext_dealloc },
    {},
};

PyType_Spec GLContext_spec = { "osmesa.GLContext",
    sizeof(GLContext),
    0,
    Py_TPFLAGS_DEFAULT,
    GLContext_slots };

PyMethodDef module_methods[] = {
    { "create_context",
        (PyCFunction)meth_create_context,
        METH_VARARGS | METH_KEYWORDS,
        NULL },
    {},
};

PyModuleDef module_def = { PyModuleDef_HEAD_INIT,
    "osmesa",
    NULL,
    -1,
    module_methods };

extern "C" PyObject*
PyInit_osmesa()
{
    PyObject* module = PyModule_Create(&module_def);
    GLContext_type = (PyTypeObject*)PyType_FromSpec(&GLContext_spec);
    PyModule_AddObject(module, "GLContext", (PyObject*)GLContext_type);
    return module;
}
