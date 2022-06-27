// Based on https://github.com/google/angle/blob/251ba5cb119ff2fed0e861cbc9b096c45004c1fa/util/EGLWindow.cpp#L108

#include <Python.h>
#include <structmember.h>

#ifdef _WIN32
    #include <windows.h>
    // I know this is the weird
    #define dlsym GetProcAddress
#else
    #include <dlfcn.h>
#endif

#if defined(__cplusplus)
#define EGL_CAST(type, value) (static_cast<type>(value))
#else
#define EGL_CAST(type, value) ((type) (value))
#endif

struct Display;

typedef unsigned int EGLenum;
typedef int EGLint;
typedef unsigned int EGLBoolean;
typedef Display * EGLNativeDisplayType;
typedef void * EGLConfig;
typedef void * EGLSurface;
typedef void * EGLContext;
typedef void * EGLDeviceEXT;
typedef void * EGLDisplay;
typedef intptr_t EGLAttrib;

#define EGL_DEFAULT_DISPLAY 0
#define EGL_NO_CONTEXT 0
#define EGL_NO_SURFACE 0
#define EGL_NO_DISPLAY EGL_CAST(EGLDisplay, 0)
#define EGL_PBUFFER_BIT 0x0001
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_NONE 0x3038
#define EGL_OPENGL_BIT 0x0008
#define EGL_BLUE_SIZE 0x3022
#define EGL_DEPTH_SIZE 0x3025
#define EGL_RED_SIZE 0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_SURFACE_TYPE 0x3033
#define EGL_OPENGL_API 0x30A2
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3056
#define EGL_SUCCESS 0x3000
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT 0x00000001
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE 0x31B1
#define EGL_PLATFORM_DEVICE_EXT 0x313F
#define EGL_PLATFORM_WAYLAND_EXT 0x31D8
#define EGL_PLATFORM_X11_EXT 0x31D5
#define EGL_EXTENSIONS 0x3055
#define EGL_PLATFORM_ANGLE_ANGLE          0x3202
#define EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE 0x320D
#define EGL_OPENGL_ES_API                 0x30A0

typedef EGLint (* m_eglGetErrorProc)();
typedef EGLDisplay (* m_eglGetDisplayProc)(EGLNativeDisplayType);
typedef EGLBoolean (* m_eglInitializeProc)(EGLDisplay, EGLint *, EGLint *);
typedef EGLBoolean (* m_eglChooseConfigProc)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *);
typedef EGLBoolean (* m_eglBindAPIProc)(EGLenum);
typedef EGLContext (* m_eglCreateContextProc)(EGLDisplay, EGLConfig, EGLContext, const EGLint *);
typedef EGLBoolean (* m_eglDestroyContextProc)(EGLDisplay, EGLContext);
typedef EGLBoolean (* m_eglMakeCurrentProc)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
typedef void (* (* m_eglGetProcAddressProc)(const char *))();
typedef EGLBoolean (* m_eglQueryDevicesEXTProc)(EGLint, EGLDeviceEXT *, EGLint *);
typedef EGLDisplay (* m_eglGetPlatformDisplayProc) (EGLenum, void *, const EGLint *);
typedef const char* (* m_eglQueryStringProc)(EGLDisplay, EGLint);

struct GLContext {
    PyObject_HEAD

#ifndef _WIN32
    void * libgl;
    void * libegl;
#else
    HMODULE libgl;
    HMODULE libegl;
#endif
    EGLContext ctx;
    EGLDisplay dpy;
    EGLConfig cfg;

    int standalone;
    char* extensionString;

    m_eglGetErrorProc m_eglGetError;
    m_eglGetDisplayProc m_eglGetDisplay;
    m_eglInitializeProc m_eglInitialize;
    m_eglChooseConfigProc m_eglChooseConfig;
    m_eglBindAPIProc m_eglBindAPI;
    m_eglCreateContextProc m_eglCreateContext;
    m_eglDestroyContextProc m_eglDestroyContext;
    m_eglMakeCurrentProc m_eglMakeCurrent;
    m_eglGetProcAddressProc m_eglGetProcAddress;
    m_eglQueryDevicesEXTProc m_eglQueryDevicesEXT;
    m_eglGetPlatformDisplayProc m_eglGetPlatformDisplay;
    m_eglQueryStringProc m_eglQueryString;
};

PyTypeObject * GLContext_type;

GLContext * meth_create_context(PyObject * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"mode", "libgl", "libegl", "glversion", "device_index", NULL};

    const char * mode = "standalone";

#ifndef _WIN32
    const char * libgl = "libGL.so";
    const char * libegl = "libEGL.so";
#else
    const char * libgl = "opengl32";
    const char * libegl = "libEGL";
#endif

    int glversion = 330;
    int device_index = 0;
    int load_mode;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sssii", keywords, &mode, &libgl, &libegl, &glversion, &device_index)) {
        return NULL;
    }

    GLContext * res = PyObject_New(GLContext, GLContext_type);

#ifndef _WIN32
    res->libgl = dlopen(libgl, RTLD_LAZY);
    if (!res->libgl) {
        PyErr_Format(PyExc_Exception, "%s not loaded", libgl);
        return NULL;
    }
#else
    // The mode parameter is required for dll's specifed as libgl
    // to load successfully along with its dependencies on Python 3.8+.
    // This is due to the introduction of using a short dll load path
    // inside of Python 3.8+.

    load_mode = LOAD_LIBRARY_SEARCH_DEFAULT_DIRS; // Search default dirs first

    // Check whether libgl contains `/` or `\\`.
    // We can treat that `libgl` would be absolute in that case.
    if (strchr(libgl, '/') || strchr(libgl, '\\'))
    {
        // Search the dirs in which the dll is located
        load_mode |= LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;
    }
    res->libgl = LoadLibraryEx(libgl, NULL, (DWORD)load_mode);
    if (!res->libgl) {
        DWORD last_error = GetLastError();
        PyErr_Format(PyExc_Exception, "%s not loaded. Error code: %ld.", libgl, last_error);
        return NULL;
    }
#endif

#ifndef _WIN32
    res->libegl = dlopen(libegl, RTLD_LAZY);
    if (!res->libegl) {
        PyErr_Format(PyExc_Exception, "%s not loaded", libegl);
        return NULL;
    }
#else
    // The mode parameter is required for dll's specifed as libgl
    // to load successfully along with its dependencies on Python 3.8+.
    // This is due to the introduction of using a short dll load path
    // inside of Python 3.8+.

    load_mode = LOAD_LIBRARY_SEARCH_DEFAULT_DIRS; // Search default dirs first

    // Check whether libgl contains `/` or `\\`.
    // We can treat that `libgl` would be absolute in that case.
    if (strchr(libegl, '/') || strchr(libegl, '\\'))
    {
        // Search the dirs in which the dll is located
        load_mode |= LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;
    }
    res->libegl = LoadLibraryEx(libegl, NULL, (DWORD)load_mode);
    if (!res->libegl) {
        DWORD last_error = GetLastError();
        PyErr_Format(PyExc_Exception, "%s not loaded. Error code: %ld.", libgl, last_error);
        return NULL;
    }
#endif

    res->m_eglQueryString = (m_eglQueryStringProc)dlsym(res->libegl, "eglQueryString");
    if (!res->m_eglGetError) {
        PyErr_Format(PyExc_Exception, "eglQueryString not found");
        return NULL;
    }

    const char *extensionString =
        (const char *)(res->m_eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));

    res->m_eglGetError = (m_eglGetErrorProc)dlsym(res->libegl, "eglGetError");
    if (!res->m_eglGetError) {
        PyErr_Format(PyExc_Exception, "eglGetError not found");
        return NULL;
    }

    res->m_eglGetDisplay = (m_eglGetDisplayProc)dlsym(res->libegl, "eglGetDisplay");
    if (!res->m_eglGetDisplay) {
        PyErr_Format(PyExc_Exception, "eglGetDisplay not found");
        return NULL;
    }

    res->m_eglInitialize = (m_eglInitializeProc)dlsym(res->libegl, "eglInitialize");
    if (!res->m_eglInitialize) {
        PyErr_Format(PyExc_Exception, "eglInitialize not found");
        return NULL;
    }

    res->m_eglChooseConfig = (m_eglChooseConfigProc)dlsym(res->libegl, "eglChooseConfig");
    if (!res->m_eglChooseConfig) {
        PyErr_Format(PyExc_Exception, "eglChooseConfig not found");
        return NULL;
    }

    res->m_eglBindAPI = (m_eglBindAPIProc)dlsym(res->libegl, "eglBindAPI");
    if (!res->m_eglBindAPI) {
        PyErr_Format(PyExc_Exception, "eglBindAPI not found");
        return NULL;
    }

    res->m_eglCreateContext = (m_eglCreateContextProc)dlsym(res->libegl, "eglCreateContext");
    if (!res->m_eglCreateContext) {
        PyErr_Format(PyExc_Exception, "eglCreateContext not found");
        return NULL;
    }

    res->m_eglDestroyContext = (m_eglDestroyContextProc)dlsym(res->libegl, "eglDestroyContext");
    if (!res->m_eglDestroyContext) {
        PyErr_Format(PyExc_Exception, "eglDestroyContext not found");
        return NULL;
    }

    res->m_eglMakeCurrent = (m_eglMakeCurrentProc)dlsym(res->libegl, "eglMakeCurrent");
    if (!res->m_eglMakeCurrent) {
        PyErr_Format(PyExc_Exception, "eglMakeCurrent not found");
        return NULL;
    }

    res->m_eglGetProcAddress = (m_eglGetProcAddressProc)dlsym(res->libegl, "eglGetProcAddress");
    if (!res->m_eglGetProcAddress) {
        PyErr_Format(PyExc_Exception, "eglGetProcAddress not found");
        return NULL;
    }

    res->m_eglGetPlatformDisplay = (m_eglGetPlatformDisplayProc)res->m_eglGetProcAddress("eglGetPlatformDisplay");
    if (!res->m_eglGetPlatformDisplay) {
        PyErr_Format(PyExc_Exception, "eglGetPlatformDisplayEXT not found");
        return NULL;
    }

    if (!strcmp(mode, "standalone")) {
        res->standalone = true;
        EGLDeviceEXT device;

        // res->dpy = 

        if (strstr(extensionString, "EGL_ANGLE_platform_angle"))
        {
            res->dpy = res->m_eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE,
                                            (void *)(0),
                                            NULL);
        }
        // else
        // {
        //     mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        // }
        // res->dpy = res->m_eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, device, 0);
        if (res->dpy == EGL_NO_DISPLAY) {
            PyErr_Format(PyExc_Exception, "eglGetPlatformDisplayEXT failed (0x%x)", res->m_eglGetError());
            return NULL;
        }

        EGLint major, minor;
        if (!res->m_eglInitialize(res->dpy, &major, &minor)) {
            PyErr_Format(PyExc_Exception, "eglInitialize failed (0x%x)", res->m_eglGetError());
            return NULL;
        }

        EGLint config_attribs[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_NONE
        };

        EGLint num_configs = 0;
        if (!res->m_eglChooseConfig(res->dpy, config_attribs, &res->cfg, 1, &num_configs)) {
            PyErr_Format(PyExc_Exception, "eglChooseConfig failed (0x%x)", res->m_eglGetError());
            return NULL;
        }

        if (!res->m_eglBindAPI(EGL_OPENGL_ES_API)) {
            PyErr_Format(PyExc_Exception, "eglBindAPI failed (0x%x)", res->m_eglGetError());
            return NULL;
        }

        int ctxattribs[] = {
            EGL_CONTEXT_MAJOR_VERSION, glversion / 100 % 10,
            EGL_CONTEXT_MINOR_VERSION, glversion / 10 % 10,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
            // EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE, 1,
            EGL_NONE,
        };
        res->cfg = {EGL_NONE};
        res->ctx = res->m_eglCreateContext(res->dpy, res->cfg, EGL_NO_CONTEXT, ctxattribs);
        if (!res->ctx) {
            PyErr_Format(PyExc_Exception, "eglCreateContext failed (0x%x)", res->m_eglGetError());
            return NULL;
        }

        res->m_eglMakeCurrent(res->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, res->ctx);
        return res;
    }

    PyErr_Format(PyExc_Exception, "unknown mode");
    return NULL;
}

PyObject * GLContext_meth_load(GLContext * self, PyObject * arg) {
    const char * method = PyUnicode_AsUTF8(arg);
    void * proc = (void *)dlsym(self->libgl, method);
    if (!proc) {
        proc = (void *)self->m_eglGetProcAddress(method);
    }
    return PyLong_FromVoidPtr(proc);
}

PyObject * GLContext_meth_enter(GLContext * self) {
    self->m_eglMakeCurrent(self->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, self->ctx);
    Py_RETURN_NONE;
}

PyObject * GLContext_meth_exit(GLContext * self) {
    self->m_eglMakeCurrent(self->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    Py_RETURN_NONE;
}

PyObject * GLContext_meth_release(GLContext * self) {
    self->m_eglDestroyContext(self->dpy, self->ctx);
    Py_RETURN_NONE;
}

void GLContext_dealloc(GLContext * self) {
    Py_TYPE(self)->tp_free(self);
}

PyMethodDef GLContext_methods[] = {
    {"load", (PyCFunction)GLContext_meth_load, METH_O, NULL},
    {"release", (PyCFunction)GLContext_meth_release, METH_NOARGS, NULL},
    {"__enter__", (PyCFunction)GLContext_meth_enter, METH_NOARGS, NULL},
    {"__exit__", (PyCFunction)GLContext_meth_exit, METH_VARARGS, NULL},
    {},
};

PyMemberDef GLContext_members[] = {
    {"standalone", T_BOOL, offsetof(GLContext, standalone), READONLY, NULL},
    {},
};

PyType_Slot GLContext_slots[] = {
    {Py_tp_methods, GLContext_methods},
    {Py_tp_members, GLContext_members},
    {Py_tp_dealloc, (void *)GLContext_dealloc},
    {},
};

PyType_Spec GLContext_spec = {"egl.GLContext", sizeof(GLContext), 0, Py_TPFLAGS_DEFAULT, GLContext_slots};

PyMethodDef module_methods[] = {
    {"create_context", (PyCFunction)meth_create_context, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "angle", NULL, -1, module_methods};

extern "C" PyObject * PyInit_angle() {
    PyObject * module = PyModule_Create(&module_def);
    GLContext_type = (PyTypeObject *)PyType_FromSpec(&GLContext_spec);
    PyModule_AddObject(module, "GLContext", (PyObject *)GLContext_type);
    return module;
}
