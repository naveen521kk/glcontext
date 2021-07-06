# import glcontext

# backend = glcontext.default_backend()
# ctx = backend(mode='standalone', glversion=330)  #, libgl='libGL.so.1')
# print(ctx)
# print(ctx.load('glViewport'))
import glcontext
glcontext._osmesa()(width=40000,height=4,format='RGB')
#glcontext._wgl()(mode='standalone')
